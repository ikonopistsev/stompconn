#include "stompconn/frame.hpp"
#include <stdexcept>

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
#ifndef NDEBUG
    std::cout << data_.str() << std::endl << std::endl;
#endif
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

void subscribe::push(stomptalk::header::custom hdr)
{
    frame::push(hdr);
}

void subscribe::push(stomptalk::header::id hdr)
{
    // сохраняем кастомный id
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

subscribe::fn_type&& subscribe::fn() noexcept
{
    return std::move(fn_);
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
        // тут надо хранить объект строки до передачи его в хидер
        auto size_text = std::to_string(payload_size);
        // выставляем размер данных
        push(stomptalk::header::content_length(std::string_view(size_text)));

        // маркер конца хидеров
        append_ref(stomptalk::make_ref("\n"));

        // добавляем данные
        data_.append(std::move(payload_));

        // маркер конца пакета
        append_ref(stomptalk::make_ref("\0"));
    }
    else
        append_ref(stomptalk::make_ref("\n\0"));

#ifndef NDEBUG
    std::cout << data_.str() << std::endl << std::endl;
#endif

    output.write(std::move(data_));
}

ack::ack(std::string_view ack_id, std::size_t size_reserve)
{
    if (ack_id.empty())
        throw std::runtime_error("ack id empty");

    reserve(size_reserve);
    frame::push(stomptalk::method::tag::ack::name());
    push(stomptalk::header::id(ack_id));
}

ack::ack(const packet& p)
    : ack(p.get(stomptalk::header::ack()))
{
    auto t = p.get(stomptalk::header::transaction());
    if (!t.empty())
        push(stomptalk::header::transaction(t));
}

nack::nack(std::string_view ack_id, std::size_t size_reserve)
{
    if (ack_id.empty())
        throw std::runtime_error("nack id empty");

    reserve(size_reserve);
    frame::push(stomptalk::method::tag::nack::name());
    push(stomptalk::header::id(ack_id));
}

nack::nack(const packet& p)
    : nack(p.get(stomptalk::header::message_id()))
{
    auto t = p.get(stomptalk::header::transaction());
    if (!t.empty())
        push(stomptalk::header::transaction(t));
}
