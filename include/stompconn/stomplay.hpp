#pragma once

#include "stompconn/frame.hpp"
#include "stompconn/handler.hpp"
#include "stompconn/header_store.hpp"
#include "stomptalk/parser.hpp"
#include "stomptalk/hook_base.hpp"

#include <array>

namespace stompconn {

class stomplay final
    : public stomptalk::hook_base
{
public:
    typedef stomptalk::header::tag::content_type::content_type_id
        content_type_id;
    typedef std::function<void(packet)> fun_type;

private:
    stomptalk::parser stomp_{};
    stomptalk::parser_hook hook_{*this};
    header_store header_store_{};

    stomptalk::method::generic method_{};
    stomptalk::header::generic header_{};
    std::string current_header_{};
    content_type_id::type content_type_{content_type_id::none};

    btpro::buffer recv_{};
    fun_type on_logon_fn_{};
    handler handler_{};
    std::size_t logon_{false};

    virtual void on_frame(stomptalk::parser_hook&) noexcept override;

    virtual void on_method(stomptalk::parser_hook& hook,
        std::string_view method) noexcept override;

    virtual void on_hdr_key(stomptalk::parser_hook& hook,
        std::string_view text) noexcept override;

    virtual void on_hdr_val(stomptalk::parser_hook& hook,
        std::string_view val) noexcept override;

    virtual void on_body(stomptalk::parser_hook& hook,
        const void* data, std::size_t size) noexcept override;

    virtual void on_frame_end(stomptalk::parser_hook&) noexcept override;

    void exec_on_error() noexcept;
    void exec_on_logon() noexcept;

    void exec_on_receipt(std::string_view id) noexcept;
    void exec_on_message(std::string_view id) noexcept;

    void clear();
public:
    stomplay() = default;

    std::size_t parse(const char* ptr, std::size_t size)
    {
        return stomp_.run(hook_, ptr, size);
    }

    void on_logon(fun_type fn)
    {
        on_logon_fn_ = std::move(fn);
    }

    void logout() noexcept
    {
        logon_ = 0;
    }

    void add_handler(const std::string& id, fun_type fn);
};

} // namespace stompconn

