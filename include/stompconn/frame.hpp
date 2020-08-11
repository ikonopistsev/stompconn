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

    template<class T>
    void push(stomptalk::header::basic<T> hdr)
    {
        frame::push(hdr);
    }

    // разрешаем кастомные хидера
    void push(stomptalk::header::custom hdr);

    void push(stomptalk::header::id hdr);

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

public:
    send(std::string_view destination, std::size_t size_reserve = 320);

    void payload(btpro::buffer payload);

    void write(bt::bev& output) override;
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

} // namespace stompconn

