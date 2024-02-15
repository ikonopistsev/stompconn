#include "stompconn/connection.hpp"
#include <random>
#ifdef STOMPCONN_DEBUG
#include <iostream>
#endif

using namespace stompconn;

void connection::do_evcb(short what) noexcept
{
    /// FIXME: стаусы в нужных местах
    // status_ = status::connecting;

    if (what & BEV_EVENT_CONNECTED)
    {
        try
        {
            update_connection_id();

            bev_.enable(EV_READ);

            status_ = status::running;
        }
        catch(...)
        {
            exec_error(std::current_exception());
        }        
    }
    else
        destroy();

    exec_event_fun(what);
}

void connection::setup_write_timeout(std::size_t timeout, double tolerant)
{
    if (timeout)
    {
        if (timeout_.empty())
        {
            timeout_.create(queue_, -1, EV_PERSIST|EV_TIMEOUT,
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

void connection::exec_logon(const frame_fun& fn, stomplay::frame p) noexcept
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

void connection::exec_unsubscribe(const frame_fun& fn,
    const std::string& id, stomplay::frame p) noexcept
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
        on_event_(what);
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

    bev_.create(queue_);

    bev_.set(&proxy<connection>::recvcb,
        nullptr, &proxy<connection>::evcb, this);

    write_timeout_ = 0;
    read_timeout_ = 0;
    status_ = status::ready;
}

void connection::setup_heart_beat(const stomplay::frame& logon)
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
    destroy();
}

void connection::connect(evdns_base* dns, 
    const std::string& host, int port)
{
    create();
    // при работе с bev этот вызов должен быть посленим 
    // тк при ошибке коннетка bev будет удален в каллбеке
    bev_.connect(dns, host, port);
    // перводим статус
    status_ = status::connecting;
}

void connection::connect(evdns_base* dns, 
    const std::string& host, int port, timeval timeout)
{
    create();

    bev_.set_timeout(nullptr, &timeout);
    // при работе с bev этот вызов должен быть посленим 
    // тк при ошибке коннетка bev будет удален в каллбеке
    bev_.connect(dns, host, port);
    // перводим статус
    status_ = status::connecting;
}

void connection::unsubscribe(std::string_view id, frame_fun real_fn)
{
    assert(real_fn);

    /// FIXME: тут явно ересь какая-то
    stomplay::unsubscribe frame{id, std::move(real_fn)};
    send_command(frame, [& , id = std::string(id), fn = std::move(real_fn)](stomplay::frame p) {
          // вызываем клиентский обработчик подписки
          exec_unsubscribe(fn, id, std::move(p));
    });
}

void connection::destroy() noexcept
{
    try
    {
        bytes_readed_ = 0;
        bytes_writed_ = 0;

        timeout_.destroy();
        bev_.destroy();
        stomplay_.reset();

        status_ = status::ready;
    }
    catch (...)
    {
        exec_error(std::current_exception());
    }
}

// some helpers
static inline auto add_tranaction_id(stomplay::command& cmd, const stomplay::frame& frame)
{
    auto transaction_id = frame.get_transaction();
    auto rc = !transaction_id.empty();
    if (rc)
        cmd.push(stomplay::header::transaction(transaction_id));
    return rc;
}

void connection::ack(const stomplay::frame& frame,
    bool with_transaction_id, frame_fun fn)
{
    assert(fn);

    stomplay::ack cmd(frame.get_id());
    if (with_transaction_id)
        add_tranaction_id(cmd, frame);

    send(std::move(cmd), std::move(fn));
}

void connection::nack(const stomplay::frame& frame,
    bool with_transaction_id, frame_fun fn)
{
    assert(fn);

    stomplay::nack cmd(frame.get_id());
    if (with_transaction_id)
        add_tranaction_id(cmd, frame);

    send(std::move(cmd), std::move(fn));
}

void connection::send(stomplay::logon cmd, frame_fun real_fn)
{
    assert(real_fn);

    stomplay_.on_logon([this, fn = std::move(real_fn)](stomplay::frame frame) {
        exec_logon(fn, std::move(frame));
    });
    
    send_command(cmd);
}

void connection::send(stomplay::subscribe cmd, frame_fun fn)
{
    assert(fn);
    stomplay_.add_subscribe(cmd, std::move(fn));
    send_command(cmd);
}

/// FIXME вернуть send_temp
// void connection::send(stompconn::send_temp frame, frame_fun fn)
// {
//     assert(fn);

//     // получаем обработчик подписки
//     stomplay_.add_subscribe(frame, std::move(fn));

//     stomplay_.add_handler(frame, std::move(fn));

//     send(std::move(frame));
// }

void connection::on_error(frame_fun fn)
{
    stomplay_.on_error(std::move(fn));
}

void connection::on_except(error_fun fn)
{
    on_error_ = std::move(fn);
}

// minutes from 2020-01-01 as hex string
std::string startup_hex_minutes_20200101() noexcept
{
    struct steady {
        std::string value;
        steady() noexcept
        {
            using namespace std::chrono;
            auto t = steady_clock::now();
            auto c = duration_cast<minutes>(t.time_since_epoch()).count();
            value = std::to_string(c);
        }
    };       

    static steady s;
    return s.value;
}

// последовательный номер сообщения
void connection::update_connection_id() noexcept
{
    connection_id_ = std::to_string(++connection_seq_id_);
    connection_id_ += '@';
    static const auto time = startup_hex_minutes_20200101();
    connection_id_ += time;
}

std::string connection::create_id(char ch) noexcept
{
    std::string rc;
    rc = message_seq_id();
    rc += ch;
    rc += connection_id_;
    return rc;
}

std::string connection::create_message_id() noexcept
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
        status_ = status::error;

        if (on_error_)
            on_error_(ex);
    }
    catch (...)
    {   }
}

void connection::send_command(stomplay::command& cmd)
{
    setup_write_timeout(write_timeout_);
    bytes_writed_ += cmd.write_cmd(bev_);
}

void connection::send_command(stomplay::command& cmd, frame_fun fn)
{
    assert(fn);
    stomplay_.add_receipt(cmd, std::move(fn));
    send_command(cmd);
}

void connection::send_command(stomplay::body_command& cmd)
{
    setup_write_timeout(write_timeout_);
    bytes_writed_ += cmd.write_body_cmd(bev_);
}

void connection::send_command(stomplay::body_command& cmd, frame_fun fn)
{
    assert(fn);
    stomplay_.add_receipt(cmd, std::move(fn));
    send_command(cmd);
}

void connection::once(timeval tv, timer_fun fn)
{
    make_once(queue_, -1, EV_TIMEOUT, 
        tv, proxy_call(std::move(fn)));
}
