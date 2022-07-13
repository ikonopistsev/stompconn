#pragma once

#include "stompconn/frame.hpp"
#include "stompconn/handler.hpp"
#include "stompconn/basic_text.hpp"
#include "stompconn/header_store.hpp"
#include "stomptalk/parser.hpp"
#include "stomptalk/hook_base.hpp"

#include <array>

namespace stompconn {

class stomplay final
    : public stomptalk::hook_base
{
public:
    using fun_type = std::function<void(packet)>;
    using text_type = basic_text<char, 20>;
    using on_error_type = std::function<void(std::exception_ptr)>;

private:
    stomptalk::parser stomp_{};
    stomptalk::parser_hook hook_{*this};
    header_store header_store_{};

    std::uint64_t method_{};
    std::uint64_t header_{};
    std::string current_header_{};

    buffer recv_{};
    fun_type on_logon_fn_{};
    fun_type on_error_fn_{};
    std::string session_{};

    receipt_handler receipt_{};
    subscription_handler subscription_{};

#ifdef STOMPCONN_DEBUG
    std::string dump_{};
#endif

    virtual void on_frame(stomptalk::parser_hook&,
                          const char*) noexcept override;

    virtual void on_method(stomptalk::parser_hook& hook, 
        std::uint64_t method_id, const char* ptr, std::size_t size) noexcept override;

    virtual void on_hdr_key(stomptalk::parser_hook& hook, 
        std::uint64_t header_id, const char* ptr, std::size_t size) noexcept override;

    virtual void on_hdr_val(stomptalk::parser_hook& hook,
        const char* ptr, std::size_t size) noexcept override;

    virtual void on_body(stomptalk::parser_hook& hook,
        const void* data, std::size_t size) noexcept override;

    virtual void on_frame_end(stomptalk::parser_hook&,
                              const char*) noexcept override;

    void exec_on_error() noexcept;
    void exec_on_logon() noexcept;

    void exec_on_receipt(std::string_view id) noexcept;
    void exec_on_message(std::string_view id) noexcept;

    void clear();

public:
    stomplay() = default;

    const std::string& session() const noexcept
    {
        return session_;
    }

    std::size_t parse(const char* ptr, std::size_t size)
    {
        return stomp_.run(hook_, ptr, size);
    }

    void on_logon(fun_type fn)
    {
        on_logon_fn_ = std::move(fn);
    }

    void on_error(fun_type fn)
    {
        on_error_fn_ = std::move(fn);
    }

    const char* error_str() const noexcept
    {
        return stomptalk_get_error_str(hook_.error());
    }

    void logout();

    std::string_view add_receipt(frame& frame, fun_type fn);

    std::string_view add_handler(frame &frame, fun_type fn)
    {
        return add_receipt(frame, std::move(fn));
    }

    std::string add_subscribe(subscribe& frame, fun_type fn);

    std::string add_subscribe(send_temp& frame, fun_type fn);

    void unsubscribe(const std::string& id);

    auto& subscription() const noexcept
    {
        return subscription_;
    }
};

} // namespace stompconn

