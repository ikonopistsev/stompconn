#pragma once

#include "stomptalk/frame_base.hpp"
#include "stomptalk/header_store.hpp"
#include "btpro/buffer.hpp"
#include "btpro/tcp/bev.hpp"

namespace stompconn {

class packet;
class subscription_handler;
class frame
    : public stomptalk::frame_base
{
protected:
    btpro::buffer data_{};

public:
    frame() = default;
    virtual ~frame() = default;

    frame(frame&&) = default;
    frame& operator=(frame&&) = default;

    // all non ref
    void push_header(std::string_view key, std::string_view value) override;
    // all ref
    void push_header_ref(std::string_view prepared_key_value) override;
    // key ref, val non ref
    void push_header_val(std::string_view prepared_key,
                         std::string_view value) override;

    void push_method(std::string_view method) override;

    // complete frame before write
    virtual void complete();

    virtual int write(btpro::socket sock);

    virtual void write(btpro::tcp::bev& bev);

    virtual btpro::buffer data();

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
    std::size_t add_subscribe(subscription_handler& handler);
};

class body_frame
    : public frame
{
protected:
    btpro::buffer payload_{};

public:
    body_frame() = default;
    body_frame(body_frame&&) = default;
    body_frame& operator=(body_frame&&) = default;
    virtual ~body_frame() = default;

    void payload(btpro::buffer payload);

    void push_payload(btpro::buffer payload);

    void push_payload(const char *data, std::size_t size);

    virtual void complete() override;

    virtual std::string str() const override;
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

} // namespace stompconn

