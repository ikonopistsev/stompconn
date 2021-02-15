#include "stompconn/stomplay.hpp"
#include "stomptalk/antoull.hpp"
#include "stomptalk/sv.hpp"
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

    hook.generic_error();
}

void stomplay::on_method(stomptalk::parser_hook& hook,
    std::string_view method) noexcept
{
    try
    {
#ifndef NDEBUG
        dump_ = method;
#endif
        using namespace stomptalk::method;

        method_.eval(method);

        if (!method_.valid())
            std::cerr << "stomplay method: " << method << " unknown" << std::endl;

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

    hook.generic_error();
}

void stomplay::on_hdr_key(stomptalk::parser_hook& hook,
    std::string_view text) noexcept
{
    try
    {
#ifndef NDEBUG
        dump_ += '\n';
        dump_ += text;
#endif
        current_header_ = text;
        header_.eval(text);
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

    hook.generic_error();
}

void stomplay::on_hdr_val(stomptalk::parser_hook& hook,
    std::string_view val) noexcept
{
    try
    {
#ifndef NDEBUG
        dump_ += ':';
        dump_ += val;
#endif
        using namespace stomptalk::header;

        auto num_id = header_.num_id();
        if (num_id != num_id::none)
            header_store_.set(num_id, header_.hash(), current_header_, val);
        else
            header_store_.set(current_header_, val);

        if (num_id == tag::content_type::num)
            content_type_ = tag::content_type::eval_content_type(val);

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

    hook.generic_error();
}

void stomplay::on_body(stomptalk::parser_hook& hook,
    const void* data, std::size_t size) noexcept
{
    try
    {
#ifndef NDEBUG
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

    hook.generic_error();
}

void stomplay::on_frame_end(stomptalk::parser_hook&, const char*) noexcept
{
#ifndef NDEBUG
    std::cout << "<< " <<  dump_ << std::endl << std::endl;
#endif
    using namespace stomptalk::method;

    switch (method_.num_id())
    {
    case tag::error::num:
        exec_on_error();
        break;

    case tag::receipt::num: {
        auto id = header_store_.get(stomptalk::header::tag::receipt_id());
        if (!id.empty())
            exec_on_receipt(id);
        break;
    }

    case tag::message::num: {
        auto subs = header_store_.get(stomptalk::header::tag::subscription());
        if (!subs.empty())
            exec_on_message(subs);
        break;
    }

    case tag::connected::num:
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

        auto subs = header_store_.get(stomptalk::header::tag::subscription());
        if (!subs.empty())
            exec_on_receipt(subs);

        auto id = header_store_.get(stomptalk::header::tag::receipt_id());
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
        session_ = header_store_.get(stomptalk::header::tag::session());
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
        auto id = stomptalk::antoull(text_id);
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
    method_.reset();
    header_.reset();
    content_type_ = content_type_id::none;
    current_header_.clear();
    header_store_.clear();
    recv_.reset(btpro::buffer());
}

void stomplay::logout()
{
    session_.clear();
}

std::string_view stomplay::add_receipt(frame &frame, fun_type fn)
{
    auto receipt = receipt_.create(std::move(fn));
    frame.push(stomptalk::header::receipt(receipt));
    return receipt;
}

void stomplay::unsubscribe(std::string_view text_id)
{
    auto id = stomptalk::antoull(text_id);
    if (id > 0)
        subscription_.remove(id);
}
