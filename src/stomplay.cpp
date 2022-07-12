#include "stompconn/stomplay.hpp"
#include "stomptalk/antoull.hpp"
#include "stomptalk/parser.h"
#include <iostream>

using namespace stompconn;

void stomplay::on_frame(stomptalk::parser_hook& hook, const char*) noexcept
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

void stomplay::on_method(stomptalk::parser_hook& hook, 
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

void stomplay::on_hdr_key(stomptalk::parser_hook& hook, 
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

void stomplay::on_hdr_val(stomptalk::parser_hook& hook, 
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

void stomplay::on_body(stomptalk::parser_hook& hook,
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

void stomplay::on_frame_end(stomptalk::parser_hook&, const char*) noexcept
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

void stomplay::exec_on_error() noexcept
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

void stomplay::exec_on_logon() noexcept
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

void stomplay::exec_on_receipt(std::string_view text_id) noexcept
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

void stomplay::exec_on_message(std::string_view text_id) noexcept
{
    try
    {
        auto id = stomptalk::antoull(text_id.data(), text_id.size());
        if (id > 0)
        {
            subscription_.call(static_cast<std::size_t>(id),
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

void stomplay::clear()
{
    method_ = st_method_none;
    header_ = st_header_none;
    current_header_.clear();
    header_store_.clear();
    recv_.reset(buffer());
}

void stomplay::logout()
{
    session_.clear();
    subscription_.clear();
    receipt_.clear();
}

std::string_view stomplay::add_receipt(frame &frame, fun_type fn)
{
    auto receipt = receipt_.create(std::move(fn));
    frame.push(stompconn::header::receipt(receipt));
    return receipt;
}

std::size_t stomplay::add_subscribe(subscribe& frame, fun_type fn)
{
    auto subs_id = frame.add_subscribe(subscription_);
    add_receipt(frame, [this, subs_id, fn](auto packet) {
        try
        {
            auto subscription_id = std::to_string(subs_id);
            packet.set_subscription_id(subscription_id);

            if (!packet)
                subscription_.remove(subs_id);

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
    return subs_id;
}

void stomplay::unsubscribe(std::string_view text_id)
{
    auto id = stomptalk::antoull(text_id.data(), text_id.size());
    if (id > 0)
        unsubscribe(static_cast<std::size_t>(id));
}

void stomplay::unsubscribe(std::size_t id)
{
    subscription_.remove(id);
}
