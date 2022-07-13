#include "stompconn/frame.hpp"
#include "stompconn/handler.hpp"
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
#ifdef STOMPCONN_DEBUG
    std::cout << str() << std::endl << std::endl;
#endif
    data_.append("\n\n\0"sv);
}

int frame::write(evutil_socket_t sock)
{
    complete();

    return data_.write(sock);
}

int frame::write(bev& bev)
{
    complete();

    auto sz = data_.size();
    bev.write(std::move(data_));
    
    return static_cast<int>(sz);
}

buffer frame::data()
{
    complete();

    return buffer(std::move(data_));
}

std::string frame::str() const
{
    return data_.str();
}

logon::logon(std::string_view host,
    std::string_view login, std::string_view passcode)
{
    push(method::connect());
    push(header::accept_version_v12());

    if (host.empty())
        host = "/"sv;
    push(header::host(host));

    if (!login.empty())
        push(header::login(login));
    if (!passcode.empty())
        push(header::passcode(passcode));
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

    push(method::subscribe());
    push(header::destination(destination));
}

std::string subscribe::add_subscribe(subscription_handler& handler)
{
    auto subs_id = handler.create(std::move(fn_));
    push(header::id(subs_id));
    return subs_id;
}

send_temp::send_temp(std::string_view destination, std::string_view reply_to, fn_type fn)
    : fn_(std::move(fn))
    , reply_to_{reply_to}
{
    if (destination.empty())
        throw std::runtime_error("destination empty");

    if (reply_to.empty())
        throw std::runtime_error("reply_to empty");

    push(method::send());
    push(header::destination(destination));
    push(header::reply_to(reply_to));
}

// возвращает идентификатор подписки
std::string send_temp::add_subscribe(subscription_handler& handler)
{
    handler.create_subscription(reply_to_, std::move(fn_));
    return reply_to_;
}

void body_frame::payload(buffer payload)
{
    payload_ = std::move(payload);
}

void body_frame::push_payload(buffer payload)
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
        push(header::content_length(size));

#ifdef STOMPCONN_DEBUG
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
            rc += payload_.str().substr(0, 32);
            rc += "... size="sv;
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

    push(method::send());
    push(header::destination(destination));
}

ack::ack(std::string_view ack_id)
{
    if (ack_id.empty())
        throw std::runtime_error("ack id empty");

    push(method::ack());
    push(header::id(ack_id));
}

nack::nack(std::string_view ack_id)
{
    if (ack_id.empty())
        throw std::runtime_error("ack id empty");

    push(method::nack());
    push(header::id(ack_id));
}

begin::begin(std::string_view transaction_id)
{
    if (transaction_id.empty())
        throw std::runtime_error("transaction id empty");

    push(method::begin());
    push(header::transaction(transaction_id));
}

begin::begin(std::size_t transaction_id)
    : begin(std::to_string(transaction_id))
{   }

commit::commit(std::string_view transaction_id)
{
    if (transaction_id.empty())
        throw std::runtime_error("transaction id empty");

    push(method::commit());
    push(header::transaction(transaction_id));
}

abort::abort(std::string_view transaction_id)
{
    if (transaction_id.empty())
        throw std::runtime_error("transaction id empty");

    push(method::abort());
    push(header::transaction(transaction_id));
}

receipt::receipt(std::string_view receipt_id)
{
    if (receipt_id.empty())
        throw std::runtime_error("receipt id empty");

    push(method::receipt());
    push(header::receipt_id(receipt_id));
}

connected::connected(std::string_view session, std::string_view server_version)
{
    push(method::connected());
    push(header::version_v12());
    if (!server_version.empty())
        push(header::server(server_version));
    if (!session.empty())
        push(header::session(session));
}

connected::connected(std::string_view session)
    : connected(session, std::string_view())
{   }

error::error(std::string_view message, std::string_view receipt_id)
{
    if (message.empty())
        throw std::runtime_error("message empty");

    push(method::error());
    push(header::message(message));
    if (!receipt_id.empty())
        push(header::receipt_id(receipt_id));
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
    push(method::message());
    push(header::destination(destination));
    push(header::subscription(subscrition));
    push(header::message_id(message_id));
}

message::message(std::string_view destination,
    std::string_view subscrition, std::size_t message_id)
    : message(destination, subscrition, std::to_string(message_id))
{   }

