#pragma once

#include "stomptalk/fnv1a.hpp"

namespace stompconn {

using stomptalk::fnv1a;

template<class T, fnv1a::type H>
struct static_hash
{
    constexpr static auto value = 
        fnv1a::calc_hash<decltype(T::text)>(std::begin(T::text), std::end(T::text));
    using enable_type = 
        typename std::enable_if<value == H, std::nullptr_t>::type;
};

} // namespace stompconn
