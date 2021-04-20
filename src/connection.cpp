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
        write_timeout_ = 0;
        read_timeout_ = 0;
        timeout_.remove();
        stomplay_.logout();
        bev_.destroy();
        event_fun_(what);
    }
}

void connection::setup_write_timeout()
{
    if (write_timeout_)
        timeout_.add(std::chrono::milliseconds(write_timeout_));
}

void connection::setup_read_timeout()
{
    if (read_timeout_)
    {
        auto t = btpro::make_timeval(
            std::chrono::milliseconds(read_timeout_));
        bev_.set_timeout(&t, nullptr);
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

            // парсим данные
            auto rc = stomplay_.parse(ptr, needle);

            // если не все пропарсилось
            // это ошибка
            // дисконнектимся
            if (rc < needle)
            {
                std::cerr << "stomplay parse: "
                          << stomplay_.error_str() << std::endl;

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

        setup_read_timeout();

        return;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "receive hell" << std::endl;
    }

    do_evcb(BEV_EVENT_ERROR);
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
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "exec_unsubscribe" << std::endl;
    }
}

void connection::create()
{
    bev_.destroy();

    bev_.create(queue_, btpro::socket());

    bev_.set(&proxy<connection>::recvcb,
        nullptr, &proxy<connection>::evcb, this);
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

            // because of timing inaccuracies, the receiver
            // SHOULD be tolerant and take into account an error margin
            // нужно быть толерантным
            read_timeout_ *= 1.5;

            setup_read_timeout();

            write_timeout_ = static_cast<std::size_t>(
                std::atoll(h.substr(f + 1).data()));

            if (write_timeout_)
            {
                // because of timing inaccuracies, the receiver
                // SHOULD be tolerant and take into account an error margin
                // нужно быть толерантным
                write_timeout_ *= 0.9;

                if (!timeout_.initialized())
                {
                    timeout_.create(queue_, EV_PERSIST|EV_TIMEOUT,
                        proxy<connection>::heart_beat, this);
                }

                timeout_.add(std::chrono::milliseconds(write_timeout_));
            }
        }
    }
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

    setup_write_timeout();

    frame.write(bev_);
}

void connection::logout(stomplay::fun_type fn)
{
    assert(fn);

    frame frame;
    frame.push(stomptalk::method::disconnect());

    stomplay_.add_handler(frame, std::move(fn));

    frame.write(bev_);
}

void connection::send(stompconn::logon frame, stomplay::fun_type fn)
{
    assert(fn);

    stomplay_.on_logon(std::move(fn));

    setup_write_timeout();

    frame.write(bev_);
}

void connection::send(stompconn::subscribe frame, stomplay::fun_type fn)
{
    assert(fn);

    // получаем обработчик подписки
    stomplay_.add_handler(frame, std::move(fn));

    setup_write_timeout();

    frame.write(bev_);
}

void connection::send(stompconn::send frame, stomplay::fun_type fn)
{
    assert(fn);

//    if (frame.mask(stomptalk::header::tag::transaction()))
//        throw std::runtime_error("receipt for transaction");

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

void connection::send_heart_beat()
{
    try
    {
        // if the sender has no real STOMP frame to send,
        // it MUST send an end-of-line (EOL)
        auto buf = bev_.output();
        if (buf.empty())
            buf.append_ref(std::cref("\n"));
    }
    catch (...)
    {   }
}
