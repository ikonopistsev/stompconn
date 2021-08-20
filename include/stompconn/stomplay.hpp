#pragma once

#include "stompconn/frame.hpp"
#include "stompconn/handler.hpp"
#include "stomptalk/parser.hpp"
#include "stomptalk/hook_base.hpp"

#include <array>

namespace stompconn {

class stomplay final
    : public stomptalk::hook_base
{
public:
    using content_type_id =
        stomptalk::header::tag::content_type::content_type_id;
    using fun_type = std::function<void(packet)>;
    using header_store = stomptalk::header_store;
    using text_type = stomptalk::basic_text<char, 20>;
    using on_error_type = std::function<void(std::exception_ptr)>;

private:
    stomptalk::parser stomp_{};
    stomptalk::parser_hook hook_{*this};
    header_store header_store_{};

    stomptalk::method::generic method_{};
    stomptalk::header::generic header_{};
    std::string current_header_{};
    content_type_id::type content_type_{content_type_id::none};

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
        std::string_view method) noexcept override;

    virtual void on_hdr_key(stomptalk::parser_hook& hook,
        std::string_view text) noexcept override;

    virtual void on_hdr_val(stomptalk::parser_hook& hook,
        std::string_view val) noexcept override;

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

    std::string_view error_str() const noexcept
    {
        return hook_.error_str();
    }

    void logout();

    std::string_view add_receipt(frame& frame, fun_type fn);

    std::string_view add_handler(frame &frame, fun_type fn)
    {
        return add_receipt(frame, std::move(fn));
    }

    std::size_t add_subscribe(subscribe& frame, fun_type fn);

    void unsubscribe(std::string_view id);

    void unsubscribe(std::size_t id);

    auto& subscription() const noexcept
    {
        return subscription_;
    }
};

} // namespace stompconn

