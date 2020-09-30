#pragma once

#include "stompconn/packet.hpp"
#include "stomptalk/frame.hpp"
#include "btpro/tcp/bev.hpp"

namespace stompconn {

class frame
    : public stomptalk::frame::base
{
protected:
    btpro::buffer data_{};

    virtual void append(std::string_view text) override;
    virtual void append_ref(std::string_view text) override;
    virtual void append(btpro::buffer buf);

public:
    frame() = default;
    frame(frame&&) = default;
    virtual ~frame() override = default;

    virtual void reserve(std::size_t len) override;
    virtual void write(btpro::tcp::bev& output);
    virtual btpro::buffer data();
    virtual std::size_t write_all(btpro::socket sock);

    std::string str() const;
};

class logon final
    : public frame
{
public:
    explicit logon(std::string_view host, std::size_t size_reserve = 320);

    logon(std::string_view host, std::string_view login,
        std::size_t size_reserve = 320);

    logon(std::string_view host, std::string_view login,
        std::string_view passcode, std::size_t size_reserve = 320);
};

class subscribe final
    : public frame
{
public:
    typedef std::function<void(packet)> fn_type;

private:
    std::string id_{};
    fn_type fn_{};

public:
    subscribe(std::string_view destination,
              std::size_t size_reserve = 320);

    subscribe(std::string_view destination,
              fn_type fn,
              std::size_t size_reserve = 320);

    subscribe(std::string_view destination,
              std::string_view id,
              fn_type fn,
              std::size_t size_reserve = 320);

    template<class K, class V>
    void push(stomptalk::header::known<K, V> hdr)
    {
        frame::push(hdr);
    }

    template<class K, class V>
    void push(stomptalk::header::known_ref<K, V> hdr)
    {
        frame::push(hdr);
    }

    template<class V>
    void push(stomptalk::header::known<stomptalk::header::tag::id, V> hdr)
    {
        // сохраняем кастомный id
        id_ = hdr.value();
        frame::push(hdr);
    }

    void set(fn_type fn);

    const fn_type& fn() const noexcept;

    fn_type&& fn() noexcept;

    const std::string& id() const noexcept;

    std::string&& id() noexcept;
};

class send final
    : public frame
{
private:
    btpro::buffer payload_{};

    void push_palyoad();

public:
    send(std::string_view destination, std::size_t size_reserve = 320);

    void payload(btpro::buffer payload);

    std::size_t payload_size() const noexcept
    {
        return payload_.size();
    }

    void write(bt::bev& output) override;

    btpro::buffer data() override;

    std::size_t write_all(btpro::socket sock) override;
};

class ack final
    : public frame
{
public:
    ack(std::string_view ack_id, std::size_t size_reserve = 64);
    ack(const packet& p);
};

class nack final
    : public frame
{
public:
    nack(std::string_view ack_id, std::size_t size_reserve = 64);
    nack(const packet& p);
};

class begin final
    : public frame
{
public:
    begin(std::string_view transaction_id, std::size_t size_reserve = 64);
    begin(std::size_t transaction_id, std::size_t size_reserve = 64);
};

class commit final
    : public frame
{
public:
    commit(std::string_view transaction_id, std::size_t size_reserve = 64);
    commit(const packet& p);
};

class abort final
    : public frame
{
public:
    abort(std::string_view transaction_id, std::size_t size_reserve = 64);
    abort(const packet& p);
};

} // namespace stompconn

