#include "stompconn/connection.hpp"
#include "stompconn/conv.hpp"
#include <random>
#ifdef STOMPCONN_DEBUG
#include <iostream>
#endif

using namespace stompconn;

void connection::do_evcb(short what) noexcept
{
    connecting_ = false;

    if (what == BEV_EVENT_CONNECTED)
    {
        try
        {
            update_connection_id();
            on_connect_fun_();
            bev_.enable(EV_READ);
        }
        catch(...)
        {
            exec_error(std::current_exception());
        }        
    }
    else
    {
        disconnect();

        exec_event_fun(what);
    }
}

void connection::setup_write_timeout(std::size_t timeout, double tolerant)
{
    if (timeout)
    {
        if (timeout_.empty())
        {
            timeout_.create(queue_, EV_PERSIST|EV_TIMEOUT,
                proxy<connection>::heart_beat, this);
        }

        // because of timing inaccuracies, the receiver
        // SHOULD be tolerant and take into account an error margin
        // они должны быть толерантными
        timeout = static_cast<std::size_t>(timeout * tolerant);
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
        timeout = static_cast<std::size_t>(timeout * tolerant);
        auto tv = detail::make_timeval(
            std::chrono::milliseconds(timeout));
        bev_.set_timeout(&tv, nullptr);
    }
}

void connection::do_recv(buffer_ref input) noexcept
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
            bytes_readed_ += needle;
            // получаем указатель
            auto ptr = reinterpret_cast<const char*>(
                input.pullup(static_cast<ev_ssize_t>(needle)));

#ifdef STOMPCONN_DEBUG
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
#ifdef STOMPCONN_DEBUG
                std::cerr << "stomplay parse: "
                          << stomplay_.error_str() << std::endl;
#endif
                // очищаем весь входящий буфер
                input.drain(input.size());
                // вызываем ошибку
                do_evcb(BEV_EVENT_ERROR);
                return;
            }
            else
            {
                // очищаем input
                // сколько пропарсили
                input.drain(rc);

                // перезаводим таймер чтения
                setup_read_timeout(read_timeout_);
            }
        }

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
    timeout_.destroy();

    bev_.create(queue_, -1);

    bev_.set(&proxy<connection>::recvcb,
        nullptr, &proxy<connection>::evcb, this);

    write_timeout_ = 0;
    read_timeout_ = 0;
}

#ifdef STOMPCONN_OPENSSL
#ifdef EVENT__HAVE_OPENSSL
void connection::create(struct ssl_st *ssl)
{
    assert(ssl);

    bev_.destroy();
    timeout_.destroy();

    bev_.create(queue_, -1, ssl);

    bev_.set(&proxy<connection>::recvcb,
        nullptr, &proxy<connection>::evcb, this);

    write_timeout_ = 0;
    read_timeout_ = 0;
}
#endif
#endif

void connection::setup_heart_beat(const packet& logon)
{
    auto h = logon.get_heart_beat();
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

void connection::connect(evdns_base* dns, const std::string& host, int port)
{
    create();
    connecting_ = true;
    // при работе с bev этот вызов должен быть посленим 
    // тк при ошибке коннетка bev будет удалет в каллбеке
    bev_.connect(dns, host, port);
}

void connection::connect(evdns_base* dns, const std::string& host, int port, timeval timeout)
{
    create();
    bev_.set_timeout(nullptr, &timeout);
    connecting_ = true;
    // при работе с bev этот вызов должен быть посленим 
    // тк при ошибке коннетка bev будет удалет в каллбеке
    bev_.connect(dns, host, port);
}

void connection::unsubscribe(std::string_view id, stomplay::fun_type real_fn)
{
    assert(real_fn);

    frame frame;
    frame.push(stompconn::method::unsubscribe());
    frame.push(stompconn::header::id(id));
    stomplay_.add_receipt(frame,
        [this , id = std::string(id), fn = std::move(real_fn)](packet p) {
          // вызываем клиентский обработчик подписки
          exec_unsubscribe(fn, id, std::move(p));
    });

    setup_write_timeout(write_timeout_);

    bytes_writed_ += frame.write(bev_);
}

void connection::disconnect() noexcept
{
    try
    {
        connecting_ = false;
        bytes_readed_ = 0;
        bytes_writed_ = 0;

        timeout_.destroy();

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
    frame.push(stompconn::method::disconnect());

    stomplay_.add_handler(frame, std::move(fn));

    bytes_writed_ += frame.write(bev_);
}

// some helpers
static inline auto add_tranaction_id(stompconn::frame& frame, const packet& p)
{
    auto transaction_id = p.get_transaction();
    auto rc = !transaction_id.empty();
    if (rc)
        frame.push(stompconn::header::transaction(transaction_id));
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

void connection::nack(const packet& p,
    bool with_transaction_id, stomplay::fun_type fn)
{
    assert(fn);

    stompconn::nack frame(get_ack_id(p));
    if (with_transaction_id)
        add_tranaction_id(frame, p);

    send(std::move(frame), std::move(fn));
}

void connection::send(stompconn::logon frame, stomplay::fun_type real_fn)
{
    assert(real_fn);

    stomplay_.on_logon([this, fn = std::move(real_fn)](packet p) {
        exec_logon(fn, std::move(p));
    });

    setup_write_timeout(write_timeout_);

    bytes_writed_ += frame.write(bev_);
}

void connection::send(stompconn::subscribe frame, stomplay::fun_type fn)
{
    assert(fn);

    // получаем обработчик подписки
    stomplay_.add_subscribe(frame, std::move(fn));

    setup_write_timeout(write_timeout_);

    bytes_writed_ += frame.write(bev_);
}

void connection::send(stompconn::send frame, stomplay::fun_type fn)
{
    assert(fn);

    stomplay_.add_handler(frame, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::send_temp frame, stomplay::fun_type fn)
{
    assert(fn);

    // получаем обработчик подписки
    stomplay_.add_subscribe(frame, std::move(fn));

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

// minutes from 2020-01-01 as hex string
connection::text_id_type startup_hex_minutes_20200101() noexcept
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
    to_hex_print(rc,
        static_cast<std::uint8_t>(distrib(gen)));

    to_hex_print(rc,
        static_cast<std::uint64_t>(val));
    return rc;
}

// последовательный номер сообщения
connection::hex_text_type connection::message_seq_id() noexcept
{
    hex_text_type rc;
    to_hex_print(rc,
        static_cast<std::uint64_t>(++message_seq_id_));
    return rc;
}

// последовательный номер сообщения
void connection::update_connection_id() noexcept
{
    connection_id_.clear();
    to_hex_print(connection_id_,
        static_cast<std::uint64_t>(++connection_seq_id_));
    connection_id_ += '@';
    static const auto time = startup_hex_minutes_20200101();
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
        using namespace std::literals;
        // if the sender has no real STOMP frame to send,
        // it MUST send an end-of-line (EOL)
        auto buf = bev_.output();
        if (buf.empty())
        {
            constexpr static auto nl = "\n"sv;
            bytes_writed_ += nl.size();
            buf.append_ref(nl);
        }

#ifdef STOMPCONN_DEBUG
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

void connection::once(timeval tv, callback_type fn)
{
    make_once(queue_, -1, EV_TIMEOUT, 
        tv, new callback_type(std::move(fn)));
}
