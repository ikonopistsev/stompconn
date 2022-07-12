#pragma once

#include "stompconn/tag/method.hpp"

namespace stompconn {
namespace method {

template<class T>
class known_ref
{
public:
    using value_type = decltype (T::text);

public:
    constexpr known_ref() = default;

    constexpr const value_type& value() const noexcept
    {
        return T::text;
    }
};

constexpr static auto ack() noexcept
{
    return known_ref<tag::ack>();
}

constexpr static auto nack() noexcept
{
    return known_ref<tag::nack>();
}

constexpr static auto send() noexcept
{
    return known_ref<tag::send>();
}

constexpr static auto abort() noexcept
{
    return known_ref<tag::abort>();
}

constexpr static auto begin() noexcept
{
    return known_ref<tag::begin>();
}

constexpr static auto error() noexcept
{
    return known_ref<tag::error>();
}

constexpr static auto stomp() noexcept
{
    return known_ref<tag::stomp>();
}

constexpr static auto commit() noexcept
{
    return known_ref<tag::commit>();
}

constexpr static auto connect() noexcept
{
    return known_ref<tag::connect>();
}

constexpr static auto message() noexcept
{
    return known_ref<tag::message>();
}

constexpr static auto receipt() noexcept
{
    return known_ref<tag::receipt>();
}

constexpr static auto subscribe() noexcept
{
    return known_ref<tag::subscribe>();
}

constexpr static auto connected() noexcept
{
    return known_ref<tag::connected>();
}

constexpr static auto disconnect() noexcept
{
    return known_ref<tag::disconnect>();
}

constexpr static auto unsubscribe() noexcept
{
    return known_ref<tag::unsubscribe>();
}

} // namespace method
} // namespace stompconn
