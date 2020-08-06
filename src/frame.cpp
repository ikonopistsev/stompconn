#include "stompconn/frame.hpp"

using namespace stompconn;

void frame::append(std::string_view text)
{
    data_.append(text.data(), text.size());
}

void frame::append_ref(std::string_view text)
{
    data_.append_ref(text.data(), text.size());
}

void frame::append(btpro::buffer buf)
{
    data_.append(std::move(buf));
}

void frame::reserve(std::size_t len)
{
    data_.expand(len);
}

void frame::write(btpro::tcp::bev& output)
{
    std::cout << data_.str() << std::endl << std::endl;
    append_ref(stomptalk::make_ref("\n\0"));
    output.write(std::move(data_));
}

std::string frame::str() const
{
    return data_.str();
}

logon::logon(std::string_view host, std::size_t size_reserve)
{
    reserve(size_reserve);
    push(stomptalk::method::tag::connect::name());
    push(stomptalk::header::ver12());
    push(stomptalk::header::host(host));
}

logon::logon(std::string_view host, std::string_view login,
    std::size_t size_reserve)
{
    reserve(size_reserve);
    push(stomptalk::method::tag::connect::name());
    push(stomptalk::header::ver12());
    push(stomptalk::header::host(host));
    push(stomptalk::header::login(login));
}

logon::logon(std::string_view host, std::string_view login,
    std::string_view passcode, std::size_t size_reserve)
{
    reserve(size_reserve);
    push(stomptalk::method::tag::connect::name());
    push(stomptalk::header::ver12());
    push(stomptalk::header::host(host));
    push(stomptalk::header::login(login));
    push(stomptalk::header::passcode(passcode));
}

subscribe::subscribe(std::string_view destination,
          std::size_t size_reserve)
{
    reserve(size_reserve);
    frame::push(stomptalk::method::tag::subscribe::name());
    push(stomptalk::header::destination(destination));
}

subscribe::subscribe(std::string_view destination,
    fn_type fn, std::size_t size_reserve)
    : fn_(std::move(fn))
{
    reserve(size_reserve);
    frame::push(stomptalk::method::tag::subscribe::name());
    push(stomptalk::header::destination(destination));
}

subscribe::subscribe(std::string_view destination,
          std::string_view id, fn_type fn, std::size_t size_reserve)
    : id_(id)
    , fn_(std::move(fn))
{
    reserve(size_reserve);
    frame::push(stomptalk::method::tag::subscribe::name());
    push(stomptalk::header::destination(destination));
}

// выставить хидер
void subscribe::push(stomptalk::header::fixed hdr)
{
    frame::push(hdr);
}

void subscribe::push(stomptalk::header::id hdr)
{
    id_ = hdr.value();
    frame::push(hdr);
}

void subscribe::set(fn_type fn)
{
    fn_ = std::move(fn);
}

const subscribe::fn_type& subscribe::fn() const noexcept
{
    return fn_;
}

const std::string& subscribe::id() const noexcept
{
    return id_;
}

std::string&& subscribe::id() noexcept
{
    return std::move(id_);
}

send::send(std::string_view destination, std::size_t size_reserve)
{
    data_.expand(size_reserve);
    push(stomptalk::method::tag::send::name());
    push(stomptalk::header::destination(destination));
}

void send::payload(btpro::buffer payload)
{
    payload_.append(std::move(payload));
}

void send::write(bt::bev& output)
{
    auto payload_size = payload_.size();
    if (payload_size)
    {
        // печатаем размер контента
        auto size_text = std::to_string(payload_size);
        std::string_view size(size_text.data(), size_text.size());
        push(stomptalk::header::content_length(size));

        // маркер конца хидеров
        append_ref(stomptalk::make_ref("\n"));

        // добавляем данные
        data_.append(std::move(payload_));

        // маркер конца пакета
        append_ref(stomptalk::make_ref("\0"));
    }
    else
        append_ref(stomptalk::make_ref("\n\0"));

    //std::cout << data_.str() << std::endl << std::endl;
    output.write(std::move(data_));
}

