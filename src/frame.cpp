#include "stompconn/frame.hpp"
#include <stdexcept>

using namespace stompconn;
using stomptalk::sv;

void frame::append(std::string_view text)
{
    data_.append(text.data(), text.size());
}

void frame::append_ref(std::string_view text)
{
    data_.append_ref(text.data(), text.size());
}

void frame::append(btpro::buffer buf)
{
    data_.append(std::move(buf));
}

void frame::reserve(std::size_t len)
{
    data_.expand(len);
}

void frame::write(btpro::tcp::bev& output)
{
#ifndef NDEBUG
    std::cout << data_.str() << std::endl << std::endl;
#endif
    append_ref(sv("\n\0"));
    output.write(std::move(data_));
}

btpro::buffer frame::data()
{
#ifndef NDEBUG
    std::cout << data_.str() << std::endl << std::endl;
#endif
    append_ref(sv("\n\0"));

    btpro::buffer rc;
    rc.append(std::move(data_));
    return rc;
}

std::size_t frame::write_all(btpro::socket sock)
{
#ifndef NDEBUG
    std::cout << data_.str() << std::endl << std::endl;
#endif
    append_ref(sv("\n\0"));

    std::size_t result = 0;
    do {
        auto rc = data_.write(sock);
        if (btpro::code::fail == rc)
            throw std::runtime_error("frame write");
        result += static_cast<std::size_t>(rc);
    } while (!data_.empty());

    return result;
}

std::string frame::str() const
{
    return data_.str();
}

logon::logon(std::string_view host, std::size_t size_reserve)
{
    reserve(size_reserve);
    push(stomptalk::method::tag::connect());
    push(stomptalk::header::ver12());
    if (!host.empty())
        push(stomptalk::header::host(host));
    else
        push(stomptalk::header::host(sv("/")));
}

logon::logon(std::string_view host, std::string_view login,
    std::size_t size_reserve)
{
    reserve(size_reserve);
    push(stomptalk::method::tag::connect());
    push(stomptalk::header::ver12());
    if (!host.empty())
        push(stomptalk::header::host(host));
    else
    {
        using namespace stomptalk::header;
        push(known_ref<tag::host, std::string_view>(sv("/")));
    }
    if (!login.empty())
        push(stomptalk::header::login(login));
}

logon::logon(std::string_view host, std::string_view login,
    std::string_view passcode, std::size_t size_reserve)
{
    reserve(size_reserve);
    push(stomptalk::method::tag::connect());
    push(stomptalk::header::ver12());
    if (!host.empty())
        push(stomptalk::header::host(host));
    else
    {
        using namespace stomptalk::header;
        push(known_ref<tag::host, std::string_view>(sv("/")));
    }
    if (!login.empty())
        push(stomptalk::header::login(login));
    if (!passcode.empty())
        push(stomptalk::header::passcode(passcode));
}

subscribe::subscribe(std::string_view destination,
          std::size_t size_reserve)
{
    reserve(size_reserve);
    frame::push(stomptalk::method::tag::subscribe());
    push(stomptalk::header::destination(destination));
}

subscribe::subscribe(std::string_view destination,
    fn_type fn, std::size_t size_reserve)
    : fn_(std::move(fn))
{
    reserve(size_reserve);
    frame::push(stomptalk::method::tag::subscribe());
    push(stomptalk::header::destination(destination));
}

subscribe::subscribe(std::string_view destination,
          std::string_view id, fn_type fn, std::size_t size_reserve)
    : id_(id)
    , fn_(std::move(fn))
{
    reserve(size_reserve);
    frame::push(stomptalk::method::tag::subscribe());
    push(stomptalk::header::destination(destination));
}

void subscribe::set(fn_type fn)
{
    fn_ = std::move(fn);
}

const subscribe::fn_type& subscribe::fn() const noexcept
{
    return fn_;
}

subscribe::fn_type&& subscribe::fn() noexcept
{
    return std::move(fn_);
}

const std::string& subscribe::id() const noexcept
{
    return id_;
}

std::string&& subscribe::id() noexcept
{
    return std::move(id_);
}

send::send(std::string_view destination, std::size_t size_reserve)
{
    data_.expand(size_reserve);
    push(stomptalk::method::tag::send());
    push(stomptalk::header::destination(destination));
}

void send::payload(btpro::buffer payload)
{
    payload_.append(std::move(payload));
}

void send::push_palyoad()
{
    auto payload_size = payload_.size();
    if (payload_size)
    {
        // выставляем размер данных
        push(stomptalk::header::content_length(payload_size));

        // маркер конца хидеров
        append_ref(sv("\n"));

        // добавляем данные
        data_.append(std::move(payload_));

        // маркер конца пакета
        append_ref(sv("\0"));
    }
    else
        append_ref(sv("\n\0"));

#ifndef NDEBUG
    std::cout << data_.str() << std::endl << std::endl;
#endif

}

void send::write(bt::bev& output)
{
    push_palyoad();
    output.write(std::move(data_));
}

btpro::buffer send::data()
{
    push_palyoad();

    btpro::buffer rc;
    rc.append(std::move(data_));
    return rc;
}

std::size_t send::write_all(btpro::socket sock)
{
    push_palyoad();

    std::size_t result = 0;
    do {
        auto rc = data_.write(sock);
        if (btpro::code::fail == rc)
            throw std::runtime_error("frame write");
        result += static_cast<std::size_t>(rc);
    } while (!data_.empty());

    return result;
}

ack::ack(std::string_view ack_id, std::size_t size_reserve)
{
    if (ack_id.empty())
        throw std::runtime_error("ack id empty");

    reserve(size_reserve);
    frame::push(stomptalk::method::tag::ack());
    push(stomptalk::header::id(ack_id));
}

ack::ack(const packet& p)
    : ack(p.get(stomptalk::header::tag::ack()))
{
// not working with rabbitmq
//    auto t = p.get(stomptalk::header::tag::transaction());
//    if (!t.empty())
//        push(stomptalk::header::transaction(t));
}

nack::nack(std::string_view ack_id, std::size_t size_reserve)
{
    if (ack_id.empty())
        throw std::runtime_error("nack id empty");

    reserve(size_reserve);
    frame::push(stomptalk::method::tag::nack());
    push(stomptalk::header::id(ack_id));
}

nack::nack(const packet& p)
    : nack(p.get(stomptalk::header::tag::message_id()))
{
// not working with rabbitmq
//    auto t = p.get(stomptalk::header::tag::transaction());
//    if (!t.empty())
//        push(stomptalk::header::transaction(t));
}

begin::begin(std::string_view transaction_id, std::size_t size_reserve)
{
    if (transaction_id.empty())
        throw std::runtime_error("transaction id empty");

    reserve(size_reserve);
    frame::push(stomptalk::method::tag::begin());
    push(stomptalk::header::transaction(transaction_id));
}

begin::begin(std::size_t transaction_id, std::size_t size_reserve)
{
    reserve(size_reserve);
    frame::push(stomptalk::method::tag::begin());
    push(stomptalk::header::transaction(transaction_id));
}

commit::commit(std::string_view transaction_id, std::size_t size_reserve)
{
    if (transaction_id.empty())
        throw std::runtime_error("transaction id empty");

    reserve(size_reserve);
    frame::push(stomptalk::method::tag::commit());
    push(stomptalk::header::transaction(transaction_id));
}

commit::commit(const packet& p)
    : commit(p.get(stomptalk::header::tag::transaction()))
{   }

abort::abort(std::string_view transaction_id, std::size_t size_reserve)
{
    if (transaction_id.empty())
        throw std::runtime_error("transaction id empty");

    reserve(size_reserve);
    frame::push(stomptalk::method::tag::abort());
    push(stomptalk::header::transaction(transaction_id));
}

abort::abort(const packet& p)
    : abort(p.get(stomptalk::header::tag::transaction()))
{   }
