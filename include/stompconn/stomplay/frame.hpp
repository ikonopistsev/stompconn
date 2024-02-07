#pragma once

#include "stompconn/buffer.hpp"
#include "stompconn/buffer_event.hpp"
#include "stompconn/stomplay/header.hpp"
#include "stompconn/stomplay/header_store.hpp"
#include <functional>   

namespace stompconn {
namespace stomplay {

class packet;
class subscription_handler;

class frame
{
protected:
    buffer data_{};

public:
    frame() = default;
    virtual ~frame() = default;

    frame(frame&&) = default;
    frame& operator=(frame&&) = default;

    void push(std::string_view key, std::string_view value);

    void push(header::known_ref hdr)
    {
        push(hdr.text);
    }

    template<class K, class V>
    void push(const header::known<K,V>& hdr)
    {
        push(hdr.prepared_key);
        push(hdr.value);
    }

protected:
    void push(std::string_view text);

public:
    // complete frame before write
    virtual void complete();

    virtual int write(evutil_socket_t sock);

    virtual int write(buffer_event& bev);

    virtual buffer data();

    virtual std::string str() const;
};

class logon final
    : public frame
{
public:
    logon(std::string_view host, std::string_view login,
        std::string_view passcode);

    logon(std::string_view host, std::string_view login);

    logon(std::string_view host);
};

class subscribe final
    : public frame
{
public:
    typedef std::function<void(packet)> fn_type;

private:
    fn_type fn_{};

public:
    subscribe(std::string_view destination, fn_type fn);

    // возвращает идентификатор подписки
    std::string add_subscribe(subscription_handler& handler);
};

class body_frame
    : public frame
{
protected:
    buffer payload_{};

public:
    body_frame() = default;
    body_frame(body_frame&&) = default;
    body_frame& operator=(body_frame&&) = default;
    virtual ~body_frame() = default;

    void payload(buffer payload);

    void push_payload(buffer payload);

    void push_payload(const char *data, std::size_t size);

    virtual void complete() override;

    virtual std::string str() const override;
};

// rabbitmq temp-queue feature
// https://www.rabbitmq.com/stomp.html#d.tqd
class send_temp final
    : public body_frame
{
public:
    typedef std::function<void(packet)> fn_type;
private:
    fn_type fn_{};
    std::string reply_to_{};

public:
    send_temp(std::string_view destination, std::string_view reply_to, fn_type fn);

    // возвращает идентификатор подписки
    std::string add_subscribe(subscription_handler& handler);
};

class ack
    : public frame
{
public:
    ack(std::string_view ack_id);
};

class nack
    : public frame
{
public:
    nack(std::string_view ack_id);
};

class begin
    : public frame
{
public:
    begin(std::string_view transaction_id);
    begin(std::size_t transaction_id);
};

class commit
    : public frame
{
public:
    commit(std::string_view transaction_id);
};

class abort
    : public frame
{
public:
    abort(std::string_view transaction_id);
};

class receipt
    : public frame
{
public:
    receipt(std::string_view receipt_id);
};

class connected
    : public frame
{
public:
    connected(std::string_view session, std::string_view server_version);
    connected(std::string_view session);
};

class send
    : public body_frame
{
public:
    send(std::string_view destination);
};

class error
    : public body_frame
{
public:
    error(std::string_view message, std::string_view receipt_id);
    error(std::string_view message);
};

class message
    : public body_frame
{
public:
    message(std::string_view destination,
            std::string_view subscrition, std::string_view message_id);
    message(std::string_view destination,
            std::string_view subscrition, std::size_t message_id);
};

} // namespace stompaly
} // namespace stompconn

