#include "stompconn/connection.hpp"
#include <random>
#include <iostream>

using namespace stompconn;

void connection::do_evcb(short what) noexcept
{
    if (what == BEV_EVENT_CONNECTED)
    {
        bev_.enable(EV_READ);
        update_connection_id();
        on_connect_fun_();
    }
    else
    {
        exec_event_fun(what);
        disconnect();
    }
}

void connection::setup_write_timeout(std::size_t timeout, double tolerant)
{
    if (timeout)
    {
        if (!timeout_.initialized())
        {
            timeout_.create(queue_, EV_PERSIST|EV_TIMEOUT,
                proxy<connection>::heart_beat, this);
        }

        // because of timing inaccuracies, the receiver
        // SHOULD be tolerant and take into account an error margin
        // они должны быть толерантными
        timeout *= tolerant;
        timeout_.add(std::chrono::milliseconds(timeout));
    }
}

void connection::setup_read_timeout(std::size_t timeout, double tolerant)
{
    if (timeout)
    {
        // because of timing inaccuracies, the receiver
        // SHOULD be tolerant and take into account an error margin
        // нужно быть толерантным
        timeout *= tolerant;
        auto tv = btpro::make_timeval(
            std::chrono::milliseconds(timeout));
        bev_.set_timeout(&tv, nullptr);
    }
}

void connection::do_recv(btpro::buffer_ref input) noexcept
{
    try
    {
        // такого типа быть не может
        // буферэвент должен отрабоать дисконнект
        assert(!input.empty());

        while (!input.empty())
        {
            // сколько непрерывных данных мы имеем
            auto needle = input.contiguous_space();
            // получаем указатель
            auto ptr = reinterpret_cast<const char*>(
                input.pullup(static_cast<ev_ssize_t>(needle)));

#ifdef DEBUG
            if ((needle < 2) && ((ptr[0] == '\n') || (ptr[0] == '\r')))
                std::cout << "recv ping" << std::endl;
#endif // DEBUG
            // парсим данные
            auto rc = stomplay_.parse(ptr, needle);

            // если не все пропарсилось
            // это ошибка
            // дисконнектимся
            if (rc < needle)
            {
#ifdef DEBUG
                std::cerr << "stomplay parse: "
                          << stomplay_.error_str() << std::endl;
#endif
                // очищаем весь входящий буфер
                input.drain(input.size());
                // вызываем ошибку
                do_evcb(BEV_EVENT_ERROR);
            }
            else
            {
                // очищаем input
                // сколько пропарсили
                input.drain(rc);
            }
        }

        setup_read_timeout(read_timeout_);

        return;
    }
    catch (...)
    {
        exec_error(std::current_exception());
    }

    do_evcb(BEV_EVENT_ERROR);
}

void connection::exec_logon(const stomplay::fun_type& fn, packet p) noexcept
{
    try
    {
        assert(fn);

        setup_heart_beat(p);

        fn(std::move(p));
    }
    catch (...)
    {
        exec_error(std::current_exception());
    }
}

void connection::exec_unsubscribe(const stomplay::fun_type& fn,
    const std::string& id, packet p) noexcept
{
    try
    {
        assert(fn);
        assert(!id.empty());
        // удаляем обработчик подписки
        stomplay_.unsubscribe(id);

        fn(std::move(p));
    }
    catch (...)
    {
        exec_error(std::current_exception());
    }
}

void connection::exec_event_fun(short what) noexcept
{
    try
    {
        event_fun_(what);
    }
    catch (...)
    {
        exec_error(std::current_exception());
    }
}

void connection::create()
{
    bev_.destroy();

    bev_.create(queue_, btpro::socket());

    bev_.set(&proxy<connection>::recvcb,
        nullptr, &proxy<connection>::evcb, this);

    write_timeout_ = 0;
    read_timeout_ = 0;
}

void connection::setup_heart_beat(const packet& logon)
{
    auto h = logon.get(stomptalk::header::tag::heart_beat());
    if (!h.empty())
    {
        using namespace std::literals;
        auto f = h.find(","sv);
        if (f != std::string_view::npos)
        {
            read_timeout_ = static_cast<std::size_t>(
                std::atoll(h.data()));

            setup_read_timeout(read_timeout_);

            write_timeout_ = static_cast<std::size_t>(
                std::atoll(h.substr(f + 1).data()));

            setup_write_timeout(write_timeout_);
        }
    }
}

connection::~connection()
{
    disconnect();
}

void connection::connect(const btpro::ip::addr& addr)
{
    create();

    bev_.connect(addr);
}

void connection::connect(btpro::dns_ref dns, const std::string& host, int port)
{
    create();

    bev_.connect(dns, host, port);
}

void connection::unsubscribe(std::string_view id, stomplay::fun_type real_fn)
{
    assert(real_fn);

    frame frame;
    frame.push(stomptalk::method::unsubscribe());
    frame.push(stomptalk::header::id(id));
    stomplay_.add_receipt(frame,
        [this , id = std::string(id), fn = std::move(real_fn)](packet p) {
          // вызываем клиентский обработчик подписки
          exec_unsubscribe(fn, id, std::move(p));
    });

    setup_write_timeout(write_timeout_);

    frame.write(bev_);
}

void connection::disconnect() noexcept
{
    try
    {
        if (timeout_.initialized())
            timeout_.remove();

        stomplay_.logout();

        bev_.destroy();
    }
    catch (...)
    {
        exec_error(std::current_exception());
    }
}

void connection::logout(stomplay::fun_type fn)
{
    assert(fn);

    frame frame;
    frame.push(stomptalk::method::disconnect());

    stomplay_.add_handler(frame, std::move(fn));

    frame.write(bev_);
}

// some helpers
static inline auto add_tranaction_id(stompconn::frame& frame, const packet& p)
{
    auto transaction_id = p.get(stomptalk::header::tag::transaction());
    auto rc = !transaction_id.empty();
    if (rc)
        frame.push(stomptalk::header::transaction(transaction_id));
    return rc;
}

void connection::ack(const packet& p,
    bool with_transaction_id, stomplay::fun_type fn)
{
    assert(fn);

    stompconn::ack frame(get_ack_id(p));
    if (with_transaction_id)
        add_tranaction_id(frame, p);

    send(std::move(frame), std::move(fn));
}

void connection::ack(const packet& p, stomplay::fun_type fn)
{
    assert(fn);

    send(stompconn::ack(get_ack_id(p)), std::move(fn));
}

void connection::ack(const packet& p, bool with_transaction_id)
{
    stompconn::ack frame(get_ack_id(p));
    if (with_transaction_id)
        add_tranaction_id(frame, p);

    send(std::move(frame));
}

void connection::nack(const packet& p,
    bool with_transaction_id, stomplay::fun_type fn)
{
    assert(fn);

    stompconn::nack frame(get_ack_id(p));
    if (with_transaction_id)
        add_tranaction_id(frame, p);

    send(std::move(frame), std::move(fn));
}

void connection::nack(const packet& p, stomplay::fun_type fn)
{
    assert(fn);

    send(stompconn::nack(get_ack_id(p)), std::move(fn));
}

void connection::nack(const packet& p, bool with_transaction_id)
{
    stompconn::nack frame(get_ack_id(p));
    if (with_transaction_id)
        add_tranaction_id(frame, p);

    send(std::move(frame));
}

void connection::send(stompconn::logon frame, stomplay::fun_type real_fn)
{
    assert(real_fn);

    stomplay_.on_logon([this, fn = std::move(real_fn)](packet p) {
        exec_logon(fn, std::move(p));
    });

    setup_write_timeout(write_timeout_);

    frame.write(bev_);
}

void connection::send(stompconn::subscribe frame, stomplay::fun_type fn)
{
    assert(fn);

    // получаем обработчик подписки
    stomplay_.add_handler(frame, std::move(fn));

    setup_write_timeout(write_timeout_);

    frame.write(bev_);
}

void connection::send(stompconn::send frame, stomplay::fun_type fn)
{
    assert(fn);

    stomplay_.add_handler(frame, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::ack frame, stomplay::fun_type fn)
{
    assert(fn);

    stomplay_.add_handler(frame, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::nack frame, stomplay::fun_type fn)
{
    assert(fn);

    stomplay_.add_handler(frame, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::begin frame, stomplay::fun_type fn)
{
    assert(fn);

    stomplay_.add_handler(frame, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::commit frame, stomplay::fun_type fn)
{
    assert(fn);

    stomplay_.add_handler(frame, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::abort frame, stomplay::fun_type fn)
{
    assert(fn);

    stomplay_.add_handler(frame, std::move(fn));

    send(std::move(frame));
}

void connection::commit(std::string_view transaction_id, stomplay::fun_type fn)
{
    assert(fn);
    send(stompconn::commit(transaction_id), std::move(fn));
}

void connection::commit(std::string_view transaction_id)
{
    send(stompconn::commit(transaction_id));
}

void connection::abort(std::string_view transaction_id, stomplay::fun_type fn)
{
    assert(fn);

    send(stompconn::abort(transaction_id), std::move(fn));
}

void connection::abort(std::string_view transaction_id)
{
    send(stompconn::abort(transaction_id));
}

void connection::on_error(stomplay::fun_type fn)
{
    stomplay_.on_error(std::move(fn));
}

void connection::on_except(on_error_type fn)
{
    on_error_fun_ = std::move(fn);
}

// число минут с 2020 года
connection::text_id_type startup_hex_minutes_20201001() noexcept
{
    using namespace std::chrono;
    constexpr auto t0 = 1577836800u / 60u;
    auto t = system_clock::now();
    auto c = duration_cast<minutes>(t.time_since_epoch()).count();
    auto val = static_cast<std::uint64_t>(c - t0);

    connection::text_id_type rc;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 255);
    btdef::conv::to_hex_print(rc,
        static_cast<std::uint8_t>(distrib(gen)));

    btdef::conv::to_hex_print(rc,
        static_cast<std::uint64_t>(val));
    return rc;
}

// последовательный номер сообщения
connection::hex_text_type connection::message_seq_id() noexcept
{
    hex_text_type rc;
    btdef::conv::to_hex_print(rc,
        static_cast<std::uint64_t>(++message_seq_id_));
    return rc;
}

// последовательный номер сообщения
void connection::update_connection_id() noexcept
{
    connection_id_.clear();
    btdef::conv::to_hex_print(connection_id_,
        static_cast<std::uint64_t>(++connection_seq_id_));
    connection_id_ += '@';
    static const auto time = startup_hex_minutes_20201001();
    connection_id_ += time;
}

connection::text_id_type connection::create_id(char ch) noexcept
{
    text_id_type rc;
    rc = message_seq_id();
    rc += ch;
    rc += connection_id_;
    return rc;
}

connection::text_id_type connection::create_message_id() noexcept
{
    return create_id('M');
}

void connection::send_heart_beat() noexcept
{
    try
    {
        // if the sender has no real STOMP frame to send,
        // it MUST send an end-of-line (EOL)
        auto buf = bev_.output();
        if (buf.empty())
            buf.append_ref(std::cref("\n"));

#ifdef DEBUG
        std::cout << "send ping" << std::endl;
#endif
    }
    catch (...)
    {
        exec_error(std::current_exception());
    }
}

void connection::exec_error(std::exception_ptr ex) noexcept
{
    try
    {
        if (on_error_fun_)
            on_error_fun_(ex);
    }
    catch (...)
    {   }
}
