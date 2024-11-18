#pragma once

#include "stompconn/stomplay/command.hpp"
#include "stompconn/stomplay/handler/receipt.hpp"
#include "stompconn/stomplay/handler/subscription.hpp"
#include "stompconn/stomplay/header_store.hpp"
#include "stomptalk/parser.hpp"
#include "stomptalk/hook_base.hpp"

#include <array>

namespace stompconn {
namespace stomplay {

// тип ошибки
using error_fn_type = std::function<void(std::size_t, std::exception_ptr)>;

class protocol final
    : public stomptalk::hook_base
{
public:
    using fun_type = std::function<void(frame)>;

    enum status_type 
        : std::size_t 
    {
        ready,      // готово к использованию
        logon,      // ожидание ответа на logon
        running,    // подключено
        error       // ошибка  
    };

private:
    stomptalk::parser stomp_{};
    stomptalk::parser_hook hook_{*this};
    header_store header_store_{};

    std::uint64_t method_{};
    std::uint64_t header_{};
    std::string current_header_{};

    buffer payload_{};
    fun_type on_logon_fn_{};
    fun_type on_error_fn_{};
    std::string session_{};

    handler::receipt receipt_{};
    handler::subscription subscription_{};

    status_type status_{ready};

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
    protocol() = default;

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

    void reset();

    template<class C>
    std::string_view add_receipt(C& cmd, fun_type fn)
    {
        // создаем новую квитанцию и сохраняем ее калбек
        auto receipt = receipt_.create(std::move(fn));

        // добавляем идентификатор квитанции к команде
        // в стомпе это будет заголовок receipt
        cmd.push(header::receipt(receipt));

        // возвращаем идентификатор квитанции
        return receipt;
    }

    std::string_view add_subscribe(subscribe& cmd, fun_type fn);

    //std::string add_subscribe(send_temp& frame, fun_type fn);

    void unsubscribe(std::string_view id);

    auto& subscription() const noexcept
    {
        return subscription_;
    }

    status_type status() const noexcept
    {
        return status_;
    }
};

} // namspace stomplay
} // namespace stompconn

