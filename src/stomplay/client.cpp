#include "stompconn/stomplay/client.hpp"
#include "stomptalk/antoull.hpp"
#include "stomptalk/parser.h"
#include <iostream>

namespace stompconn {
namespace stomplay {

void client::on_frame(stomptalk::parser_hook& hook, const char*) noexcept
{
    try
    {
        clear();
        return;
    }
    catch (const std::exception& e)
    {
        std::cerr << "stomplay frame: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "stomplay frame" << std::endl;
    }

    hook.set(stomptalk_error_generic);
}

void client::on_method(stomptalk::parser_hook& hook, 
    std::uint64_t method_id, const char* ptr, std::size_t size) noexcept
{
    std::string_view method{ptr, size};
    try
    {
#ifdef STOMPCONN_DEBUG
        dump_ = method;
        if (!session_.empty()) 
        {
            dump_ += ' ';
            dump_ += '@';
            dump_ += session_;
        }
#endif
        method_ = method_id;
        return;
    }
    catch (const std::exception& e)
    {
        std::cerr << "stomplay method: " << method << ' '
                  << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "stomplay method: " << method << " error" << std::endl;
    }

    hook.set(stomptalk_error_generic);
}

void client::on_hdr_key(stomptalk::parser_hook& hook, 
    std::uint64_t header_id, const char* ptr, std::size_t size) noexcept
{
    std::string_view text{ptr, size};
    try
    {
#ifdef STOMPCONN_DEBUG
        dump_ += '\n';
        dump_ += text;
#endif
        header_ = header_id;
        current_header_ = text;
        return;
    }
    catch (const std::exception& e)
    {
        std::cerr << "stomplay header: " << text
                  << ' ' << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "stomplay header" << std::endl;
    }

    hook.set(stomptalk_error_generic);
}

void client::on_hdr_val(stomptalk::parser_hook& hook, 
    const char* ptr, std::size_t size) noexcept
{
    std::string_view val{ptr, size};
    try
    {
#ifdef STOMPCONN_DEBUG
        dump_ += ':';
        dump_ += val;
#endif
        //using namespace stompconn::header;

        header_store_.set(header_, current_header_, val);

        return;
    }
    catch (const std::exception& e)
    {
        std::cerr << "stomplay header val: " << val
                  << ' ' << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "stomplay header val" << std::endl;
    }

    hook.set(stomptalk_error_generic);
}

void client::on_body(stomptalk::parser_hook& hook,
    const void* data, std::size_t size) noexcept
{
    try
    {
#ifdef STOMPCONN_DEBUG
        dump_ += '\n';
        dump_ += std::string(reinterpret_cast<const char*>(data), size);
#endif
        recv_.append(data, size);
        return;
    }
    catch (const std::exception& e)
    {
        std::cerr << "stomplay body: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "stomplay body" << std::endl;
    }

    hook.set(stomptalk_error_generic);
}

void client::on_frame_end(stomptalk::parser_hook&, const char*) noexcept
{
#ifdef STOMPCONN_DEBUG
    std::cout << "<< " <<  dump_ << std::endl << std::endl;
#endif

    switch (method_)
    {
    case st_method_error:
        exec_on_error();
        break;

    case st_method_receipt: {
        auto id = header_store_.get(st_header_receipt_id);
        if (!id.empty())
            exec_on_receipt(id);
        break;
    }

    case st_method_message: {
        auto subs = header_store_.get(st_header_subscription);
        if (!subs.empty())
            exec_on_message(subs);
        break;
    }

    case st_method_connected:
        exec_on_logon();
        break;
    }
}

void client::exec_on_error() noexcept
{
    try
    {
        if (session_.empty())
        {
            on_logon_fn_(packet(header_store_, session_,
                                method_, std::move(recv_)));
            return;
        }

        auto subs = header_store_.get(st_header_subscription);
        if (!subs.empty())
            exec_on_receipt(subs);

        auto id = header_store_.get(st_header_receipt_id);
        if (!id.empty())
            exec_on_receipt(id);

        if (on_error_fn_)
            on_error_fn_(packet(header_store_, session_,
                                method_, std::move(recv_)));
    }
    catch (const std::exception& e)
    {
        std::cerr << "stomplay error: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "stomplay error" << std::endl;
    }
}

void client::exec_on_logon() noexcept
{
    try
    {
        // save session
        session_ = header_store_.get(st_header_session);
        on_logon_fn_(packet(header_store_, session_,
                            method_, std::move(recv_)));
    }
    catch (const std::exception& e)
    {
        std::cerr << "stomplay logon: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "stomplay logon" << std::endl;
    }
}

void client::exec_on_receipt(std::string_view text_id) noexcept
{
    try
    {
        receipt_.call(text_id,
            packet(header_store_, session_, method_, std::move(recv_)));
    }
    catch (const std::exception& e)
    {
        std::cerr << "stomplay receipt: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "stomplay receipt" << std::endl;
    }
}

void client::exec_on_message(std::string_view text_id) noexcept
{
    try
    {
        if (!text_id.empty())
        {
            subscription_.call(subscription_handler::id_type{text_id},
                packet(header_store_, session_, method_, std::move(recv_)));
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "stomplay message: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "stomplay message" << std::endl;
    }
}

void client::clear()
{
    method_ = st_method_none;
    header_ = st_header_none;
    current_header_.clear();
    header_store_.clear();
    recv_.reset(buffer());
}

void client::logout()
{
    session_.clear();
    subscription_.clear();
    receipt_.clear();
}

std::string_view client::add_receipt(frame &frame, fun_type fn)
{
    auto receipt = receipt_.create(std::move(fn));
    frame.push(header::receipt(receipt));
    return receipt;
}

std::string client::add_subscribe(subscribe& frame, fun_type fn)
{
    auto subscription_id = frame.add_subscribe(subscription_);
    add_receipt(frame, [this, subscription_id, fn](auto packet) {
        try
        {
            packet.set_subscription_id(subscription_id);

            if (!packet)
                subscription_.remove(subscription_id);

            fn(std::move(packet));
        }
        catch (const std::exception& e)
        {
            std::cerr << "stomplay subscribe receipt: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "stomplay subscribe receipt" << std::endl;
        }
    });
    return subscription_id;
}

std::string client::add_subscribe(send_temp& frame, fun_type fn)
{
    auto subscription_id = frame.add_subscribe(subscription_);
    add_receipt(frame, [this, subscription_id, fn](auto packet) {
        try
        {
            if (!packet)
                subscription_.remove(subscription_id);
                
            fn(std::move(packet));
        }
        catch (const std::exception& e)
        {
            std::cerr << "stomplay send_temp receipt: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "stomplay send_temp receipt" << std::endl;
        }
    });
    return subscription_id;
}

void client::unsubscribe(const std::string& text_id)
{
    subscription_.remove(text_id);
}

} // namspace stomplay
} // namespace stompconn