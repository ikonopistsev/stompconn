#include "stompconn/frame.hpp"
#include "stompconn/handler.hpp"
#include "stomptalk/header_store.hpp"
#include <stdexcept>
#include <iostream>

using namespace stompconn;
using namespace std::literals;

void frame::push_header(std::string_view key, std::string_view value)
{
    if (key.empty())
        throw std::logic_error("header key empty");

    if (value.empty())
        throw std::logic_error("frame header value empty");

    data_.append("\n"sv);
    data_.append(key);
    data_.append(":"sv);
    data_.append(value);
}

// all ref
void frame::push_header_ref(std::string_view prepared_key_value)
{
    assert(!prepared_key_value.empty());

    data_.append(prepared_key_value);
}

// key ref, val non ref
void frame::push_header_val(std::string_view prepared_key,
                            std::string_view value)
{
    assert(!prepared_key.empty());

    if (value.empty())
        throw std::logic_error("frame header value empty");

    data_.append(prepared_key);
    data_.append(value);
}

void frame::push_method(std::string_view method)
{
    if (method.empty())
        throw std::logic_error("frame method empty");

    data_.append(method.data(), method.size());
}

void frame::complete()
{
#ifndef NDEBUG
    std::cout << str() << std::endl << std::endl;
#endif
    data_.append("\n\n\0"sv);
}

int frame::write(btpro::socket sock)
{
    complete();

    return data_.write(sock.fd());
}

void frame::write(btpro::tcp::bev& bev)
{
    complete();

    bev.write(std::move(data_));
}

btpro::buffer frame::data()
{
    complete();

    return btpro::buffer(std::move(data_));
}

std::string frame::str() const
{
    return data_.str();
}

//void frame::write(btpro::socket sock)
//{
//    auto& frame = get();
//    frame.complete();

//#ifndef NDEBUG
//    std::cout << str() << std::endl << std::endl;
//#endif

//    auto& iv = frame.layout();

//    mmsghdr h = {};
//    h.msg_hdr.msg_iov = iv.begin();
//    h.msg_hdr.msg_iovlen= iv.size();

//    auto rc = sendmmsg(sock, &h, 1, 0);
//    if (-1 == rc)
//    {
//        throw std::system_error(
//            std::error_code(errno, std::generic_category()),
//                            "sendmmsg");
//    }

//    frame.drain(h.msg_len);
//}

logon::logon(std::string_view host,
    std::string_view login, std::string_view passcode)
{
    push(stomptalk::method::connect());
    push(stomptalk::header::accept_version_v12());

    if (host.empty())
        host = "/"sv;
    push(stomptalk::header::host(host));

    if (!login.empty())
        push(stomptalk::header::login(login));
    if (!passcode.empty())
        push(stomptalk::header::passcode(passcode));
}

logon::logon(std::string_view host, std::string_view login)
    : logon(host, login, std::string_view())
{   }

logon::logon(std::string_view host)
    : logon(host, std::string_view(), std::string_view())
{   }

subscribe::subscribe(std::string_view destination, fn_type fn)
    : fn_(std::move(fn))
{
    if (destination.empty())
        throw std::runtime_error("destination empty");

    push(stomptalk::method::subscribe());
    push(stomptalk::header::destination(destination));
}

void subscribe::add_subscribe(subscription_handler& handler)
{
    auto subs_id = handler.create(std::move(fn_));
    push(stomptalk::header::id(subs_id));
}

void body_frame::payload(btpro::buffer payload)
{
    payload_ = std::move(payload);
}

void body_frame::push_payload(btpro::buffer payload)
{
    payload_.append(std::move(payload));
}

void body_frame::push_payload(const char *data, std::size_t size)
{
    payload_.append(data, size);
}

void body_frame::complete()
{
    auto size = payload_.size();
    if (size)
    {
        // дописываем размер
        push(stomptalk::header::content_length(size));

#ifndef NDEBUG
        std::cout << str() << std::endl;
#endif
        // дописываем разделитель хидеров и боди
        data_.append("\n\n"sv);

        // дописываем протокольный ноль
        payload_.append("\0"sv);

        // добавляем боди
        data_.append(std::move(payload_));
    }
    else
        frame::complete();
}

std::string body_frame::str() const
{
    auto rc = data_.str();
    auto size = payload_.size();
    if (size)
    {
        rc += "\n\n< "sv;
        if (size > 32)
        {
            rc += payload_.str().substr(0, 8);
            rc += " size="sv;
            rc += std::to_string(size);
        }
        else
            rc += payload_.str();
        rc += " >\n"sv;
    }
    return rc;
}

send::send(std::string_view destination)
{
    if (destination.empty())
        throw std::runtime_error("destination empty");

    push(stomptalk::method::send());
    push(stomptalk::header::destination(destination));
}

ack::ack(std::string_view ack_id)
{
    push(stomptalk::method::ack());
    push(stomptalk::header::id(ack_id));
}

nack::nack(std::string_view ack_id)
{
    push(stomptalk::method::nack());
    push(stomptalk::header::id(ack_id));
}

begin::begin(std::string_view transaction_id)
{
    if (transaction_id.empty())
        throw std::runtime_error("transaction id empty");

    push(stomptalk::method::begin());
    push(stomptalk::header::transaction(transaction_id));
}

begin::begin(std::size_t transaction_id)
    : begin(std::to_string(transaction_id))
{   }

commit::commit(std::string_view transaction_id)
{
    if (transaction_id.empty())
        throw std::runtime_error("transaction id empty");

    push(stomptalk::method::commit());
    push(stomptalk::header::transaction(transaction_id));
}

abort::abort(std::string_view transaction_id)
{
    if (transaction_id.empty())
        throw std::runtime_error("transaction id empty");

    push(stomptalk::method::abort());
    push(stomptalk::header::transaction(transaction_id));
}

receipt::receipt(std::string_view receipt_id)
{
    if (receipt_id.empty())
        throw std::runtime_error("receipt id empty");

    push(stomptalk::method::receipt());
    push(stomptalk::header::receipt_id(receipt_id));
}

connected::connected(std::string_view session, std::string_view server_version)
{
    push(stomptalk::method::connected());
    push(stomptalk::header::version_v12());
    if (!server_version.empty())
        push(stomptalk::header::server(server_version));
    if (!session.empty())
        push(stomptalk::header::session(session));
}

connected::connected(std::string_view session)
    : connected(session, std::string_view())
{   }

error::error(std::string_view message, std::string_view receipt_id)
{
    if (message.empty())
        throw std::runtime_error("message empty");

    push(stomptalk::method::error());
    push(stomptalk::header::message(message));
    if (!receipt_id.empty())
        push(stomptalk::header::receipt_id(receipt_id));
}

error::error(std::string_view message)
    : error(message, std::string_view())
{   }

message::message(std::string_view destination,
    std::string_view subscrition, std::string_view message_id)
{
    if (destination.empty())
        throw std::runtime_error("destination empty");
    if (subscrition.empty())
        throw std::runtime_error("subscrition empty");
    if (message_id.empty())
        throw std::runtime_error("message_id empty");
    push(stomptalk::method::message());
    push(stomptalk::header::destination(destination));
    push(stomptalk::header::subscription(subscrition));
    push(stomptalk::header::message_id(message_id));
}

message::message(std::string_view destination,
    std::string_view subscrition, std::size_t message_id)
    : message(destination, subscrition, std::to_string(message_id))
{   }

