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
        stomplay_.logout();
        bev_.destroy();
        event_fun_(what);
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

void connection::exec_subscribe(const stomplay::fun_type& fn, packet p) noexcept
{
    try
    {
        assert(fn);
        fn(std::move(p));
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "exec_subscribe" << std::endl;
    }
}

void connection::exec_unsubscribe(const stomplay::fun_type& fn,
                                  const std::string& id, packet p) noexcept
{
    try
    {
        assert(fn);
        assert(!id.empty());

        fn(std::move(p));

        // удаляем обработчик подписки
        stomplay_.remove_handler(id);
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

void connection::unsubscribe(std::string_view id, stomplay::fun_type fn)
{
    assert(fn);

    frame frame;
    frame.push(stomptalk::method::unsubscribe());
    frame.push(stomptalk::header::id(id));

    auto rcpt_id = create_receipt_id();
    auto receipt_id = stomptalk::sv(rcpt_id);
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id,
        [this , id = std::string(id), receipt_fn = std::move(fn)] (packet p) {
          // вызываем клиентский обработчик подписки
          exec_unsubscribe(receipt_fn, id, std::move(p));
    });

    frame.write(bev_);
}

void connection::logout(stomplay::fun_type fn)
{
    assert(fn);

    frame frame;
    frame.push(stomptalk::method::disconnect());

    auto rcpt_id = create_receipt_id();
    auto receipt_id = stomptalk::sv(rcpt_id);
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

    frame.write(bev_);
}

void connection::send(stompconn::logon frame, stomplay::fun_type fn)
{
    assert(fn);

    stomplay_.on_logon(std::move(fn));

    frame.write(bev_);
}

void connection::send(stompconn::subscribe frame, stomplay::fun_type fn)
{
    assert(fn);

    // генерируем id квитанции
    auto rcpt_id = create_receipt_id();
    auto receipt_id = stomptalk::sv(rcpt_id);
    frame.push(stomptalk::header::receipt(receipt_id));

    auto subs_id = create_subs_id();
    frame.push(stomptalk::header::id(stomptalk::sv(subs_id)));

    // получаем обработчик подписки
    stomplay_.add_handler(receipt_id,
        [this, id = subs_id, frame_fn = std::move(frame.fn()),
            receipt_fn = std::move(fn)] (packet p) {
            // добавляем id подписки и ее обработчик
            stomplay_.add_handler(stomptalk::sv(id), frame_fn);
            // вызываем клиентский обработчик подписки
            exec_subscribe(receipt_fn, std::move(p));
    });

    frame.write(bev_);
}

void connection::send(stompconn::send frame, stomplay::fun_type fn)
{
    assert(fn);

//    if (frame.mask(stomptalk::header::tag::transaction()))
//        throw std::runtime_error("receipt for transaction");

    auto rcpt_id = create_receipt_id();
    auto receipt_id = stomptalk::sv(rcpt_id);
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::ack frame, stomplay::fun_type fn)
{
    assert(fn);

    auto rcpt_id = create_receipt_id();
    auto receipt_id = stomptalk::sv(rcpt_id);
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::nack frame, stomplay::fun_type fn)
{
    assert(fn);

    auto rcpt_id = create_receipt_id();
    auto receipt_id = stomptalk::sv(rcpt_id);
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::begin frame, stomplay::fun_type fn)
{
    assert(fn);

    auto rcpt_id = create_receipt_id();
    auto receipt_id = stomptalk::sv(rcpt_id);
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::commit frame, stomplay::fun_type fn)
{
    assert(fn);

    auto rcpt_id = create_receipt_id();
    auto receipt_id = stomptalk::sv(rcpt_id);
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::abort frame, stomplay::fun_type fn)
{
    assert(fn);

    auto rcpt_id = create_receipt_id();
    auto receipt_id = stomptalk::sv(rcpt_id);
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

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

connection::text_id_type connection::create_subs_id() noexcept
{
    return create_id('S');
}

connection::text_id_type connection::create_receipt_id() noexcept
{
    return create_id('R');
}

connection::text_id_type connection::create_message_id() noexcept
{
    return create_id('M');
}
