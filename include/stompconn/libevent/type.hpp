#pragma once

#include <mutex>
#include <memory>
#include <chrono>
#include <cassert>
#include <stdexcept>

#include "event2/dns.h"
#include "event2/event.h"

namespace stompconn {
namespace libevent {

using event_handle = event*;
using dns_handle_type = evdns_base*;
using queue_handle_type = event_base*;

template<class T>
T assert_handle(T handle) noexcept
{
    assert(handle);
    return handle;
}

namespace detail {

static inline void check_result(const char* what, int result)
{
    assert(what);

    if (-1 == result)
        throw std::runtime_error(what);
}

template<class T>
std::size_t check_size(const char* what, T result)
{
    assert(what);

    if (result == static_cast<T>(-1))
        throw std::runtime_error(what);

    return static_cast<std::size_t>(result);
}

template<class P>
P* check_pointer(const char* what, P* value)
{
    assert(what);
    if (!value)
        throw std::runtime_error(what);
    return value;
}

template<class Rep, class Period>
timeval make_timeval(std::chrono::duration<Rep, Period> timeout) noexcept
{
    const auto sec = std::chrono::duration_cast<
        std::chrono::seconds>(timeout);
    const auto usec = std::chrono::duration_cast<
        std::chrono::microseconds>(timeout) - sec;

    // FIX: decltype fix for clang macosx
    return {
        static_cast<decltype(timeval::tv_sec)>(sec.count()),
        static_cast<decltype(timeval::tv_usec)>(usec.count())
    };
}

} // namespace detail

} // namespace libevent
} // namespace stompconn
