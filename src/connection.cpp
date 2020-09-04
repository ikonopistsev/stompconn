#include "stompconn/connection.hpp"

using namespace stompconn;

void connection::do_evcb(short what) noexcept
{
    if (what == BEV_EVENT_CONNECTED)
    {
        bev_.enable(EV_READ);
        ++connection_id_;
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
#ifdef DEBUG
            // это для тестов
            if (needle > 1)
                needle /= 2;
#endif // DEBUG
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

                // очищаем весь входящиц буфер
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

void connection::logon(stompconn::logon frame, stomplay::fun_type fn)
{
    send(std::move(frame), std::move(fn));
}

void connection::subscribe(stompconn::subscribe frame, stomplay::fun_type fn)
{
    send(std::move(frame), std::move(fn));
}

void connection::unsubscribe(std::string_view id, stomplay::fun_type fn)
{
    assert(fn);

    auto receipt_id = create_receipt_id();

    frame frame;
    frame.push(stomptalk::method::tag::unsubscribe());
    frame.push(stomptalk::header::id(id));
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

    auto receipt_id = create_receipt_id();

    frame frame;
    frame.push(stomptalk::method::tag::disconnect());
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
    auto receipt_id = create_receipt_id();
    frame.push(stomptalk::header::receipt(receipt_id));

    // получаем id подписки
    // если нет - создаем новый
    // и добавляем во фрейм
    auto subs_id = std::move(frame.id());
    if (subs_id.empty())
    {
        subs_id = create_subs_id();
        frame.push(stomptalk::header::id(subs_id));
    }

    // получаем обработчик подписки
    stomplay_.add_handler(receipt_id,
        [this, id = std::move(subs_id), frame_fn = frame.fn(),
            receipt_fn = std::move(fn)] (packet p) {
            // добавляем id подписки и ее обработчик
            stomplay_.add_handler(id, frame_fn);
            // вызываем клиентский обработчик подписки
            exec_subscribe(receipt_fn, std::move(p));
    });

    frame.write(bev_);
}

void connection::send(stompconn::send frame, stomplay::fun_type fn)
{
    assert(fn);

    if (frame.mask(stomptalk::header::tag::transaction()))
        throw std::runtime_error("receipt for transaction");

    auto receipt_id = create_receipt_id();
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::ack frame, stomplay::fun_type fn)
{
    assert(fn);

    auto receipt_id = create_receipt_id();
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::nack frame, stomplay::fun_type fn)
{
    assert(fn);

    auto receipt_id = create_receipt_id();
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::begin frame, stomplay::fun_type fn)
{
    assert(fn);

    auto receipt_id = create_receipt_id();
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::commit frame, stomplay::fun_type fn)
{
    assert(fn);

    auto receipt_id = create_receipt_id();
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

    send(std::move(frame));
}

void connection::send(stompconn::abort frame, stomplay::fun_type fn)
{
    assert(fn);

    auto receipt_id = create_receipt_id();
    frame.push(stomptalk::header::receipt(receipt_id));

    stomplay_.add_handler(receipt_id, std::move(fn));

    send(std::move(frame));
}

void connection::ack(stompconn::ack frame, stomplay::fun_type fn)
{
    send(std::move(frame), std::move(fn));
}

void connection::nack(stompconn::nack frame, stomplay::fun_type fn)
{
    send(std::move(frame), std::move(fn));
}

void connection::begin(stompconn::begin frame, stomplay::fun_type fn)
{
    send(std::move(frame), std::move(fn));
}

void connection::commit(std::string_view transaction_id, stomplay::fun_type fn)
{
    assert(fn);
    send(stompconn::commit(transaction_id), std::move(fn));
}

void connection::commit(stompconn::commit frame, stomplay::fun_type fn)
{
    send(std::move(frame), std::move(fn));
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

void connection::abort(stompconn::abort frame, stomplay::fun_type fn)
{
    send(std::move(frame), std::move(fn));
}

void connection::abort(std::string_view transaction_id)
{
    send(stompconn::abort(transaction_id));
}

void connection::on_error(stomplay::fun_type fn)
{
    stomplay_.on_error(std::move(fn));
}

btdef::text trim00(const btdef::text& text) noexcept
{
    auto b = text.begin();
    auto e = text.end();
    while ((*b == '0') && (b < e))
    { ++b; }
    return btdef::text(b, std::distance(b, e));
}

// число минут с 2020 года
std::string startup_hex_minutes_20201001() noexcept
{
    using namespace std::chrono;
    constexpr auto t0 = 1577836800u / 60u;
    auto t = system_clock::now();
    auto c = duration_cast<minutes>(t.time_since_epoch()).count();
    auto val = static_cast<std::uint64_t>(c - t0);
    auto ses = trim00(btdef::to_hex(val));
    return std::string(ses.data(), ses.size());
}

// последовательный номер сообщения
std::string connection::message_seq_id() noexcept
{
    auto text = trim00(btdef::to_hex(
        static_cast<std::uint64_t>(++message_seq_id_)));
    return std::string(text.data(), text.size());
}

// последовательный номер сообщения
std::string connection::connection_seq_id() noexcept
{
    auto text = trim00(btdef::to_hex(
        static_cast<std::uint64_t>(connection_id_)));
    return std::string(text.data(), text.size());
}

std::string connection::create_id(char L) noexcept
{
    static const auto time_const = startup_hex_minutes_20201001();
    auto msg_id = message_seq_id();
    auto conn_id = connection_seq_id();
    std::string rc;
    auto sz = msg_id.size() + conn_id.size() + time_const.size() + 2;
    if (sz >= rc.capacity())
        rc.reserve(sz);
    rc = msg_id;
    rc += L;
    rc += conn_id;
    rc += '@';
    rc += time_const;
    return rc;
}

std::string connection::create_subs_id() noexcept
{
    return create_id('S');
}

std::string connection::create_receipt_id() noexcept
{
    return create_id('R');
}

std::string connection::create_message_id() noexcept
{
    return create_id('M');
}
