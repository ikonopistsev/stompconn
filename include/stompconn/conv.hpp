#pragma once

#include <string_view>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif // _WIN32

namespace stompconn
{

static inline std::string_view to_hex(unsigned char val) noexcept
{
    constexpr static const std::string_view t[] = {
        "00", "01", "02", "03", "04", "05", "06", "07",
        "08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
        "10", "11", "12", "13", "14", "15", "16", "17",
        "18", "19", "1a", "1b", "1c", "1d", "1e", "1f",
        "20", "21", "22", "23", "24", "25", "26", "27",
        "28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
        "30", "31", "32", "33", "34", "35", "36", "37",
        "38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
        "40", "41", "42", "43", "44", "45", "46", "47",
        "48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
        "50", "51", "52", "53", "54", "55", "56", "57",
        "58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
        "60", "61", "62", "63", "64", "65", "66", "67",
        "68", "69", "6a", "6b", "6c", "6d", "6e", "6f",
        "70", "71", "72", "73", "74", "75", "76", "77",
        "78", "79", "7a", "7b", "7c", "7d", "7e", "7f",
        "80", "81", "82", "83", "84", "85", "86", "87",
        "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
        "90", "91", "92", "93", "94", "95", "96", "97",
        "98", "99", "9a", "9b", "9c", "9d", "9e", "9f",
        "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
        "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af",
        "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7",
        "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
        "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7",
        "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
        "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
        "d8", "d9", "da", "db", "dc", "dd", "de", "df",
        "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7",
        "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
        "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
        "f8", "f9", "fa", "fb", "fc", "fd", "fe", "ff"
    };

    return t[val];
}

static inline std::string_view to_hex(char val) noexcept
{
    return to_hex(static_cast<unsigned char>(val));
}

template<class T>
void to_hex_print(T& rc, const char *ptr, std::size_t len) noexcept
{
    assert(ptr);

    auto end = ptr + len;
    while (ptr < end)
        rc += to_hex(*ptr++);
}

template<class T>
void to_hex_print(T& rc, std::uint8_t val)
{
    rc += to_hex(static_cast<unsigned char>(val));
}

template<class, int>
struct cast_conv;

template<class T>
struct cast_conv<T, 1>
{
    static inline auto hton(T val)
    {
        return static_cast<std::uint8_t>(val);
    }
};

template<class T>
struct cast_conv<T, 2>
{
    static inline auto hton(T val)
    {
        return static_cast<std::uint16_t>(
            htons(static_cast<std::uint16_t>(val)));
    }
};

template<class T>
struct cast_conv<T, 4>
{
    static inline auto hton(T val)
    {
        return static_cast<std::uint32_t>(
            htonl(static_cast<std::uint32_t>(val)));
    }
};

template<class T>
struct cast_conv<T, 8>
{
    static inline auto hton(T val)
    {
#ifndef htonll
        val = (((static_cast<std::uint64_t>(htonl(val & 0xffffffff)) << 32) |
            htonl(static_cast<std::uint32_t>((val) >> 32))));
#undef htonll
#else
        val = htonll(val);
#endif // htonll
        return static_cast<std::uint64_t>(val);
    }
};

template<class T,
    typename std::enable_if_t<std::is_unsigned<T>::value, int> = 1>
auto cv_htount(T value)
{
    return cast_conv<T, sizeof(value)>::hton(value);
}

template<class T,
    typename std::enable_if_t<std::is_signed<T>::value, int> = 1>
auto cv_htount(T value)
{
    using type = typename std::make_unsigned<T>::type;
    return cv_htount(static_cast<type>(value));
}

template<class T, class V,
    typename std::enable_if_t<std::numeric_limits<V>::is_integer, int> = 1>
void to_hex_print(T& rc, V value)
{
    auto val = cv_htount(value);

    if (!val)
        rc += to_hex(static_cast<unsigned char>(val));
    else do {
        auto v = static_cast<unsigned char>(val & 0xff);
        if (v)
            rc += to_hex(v);
        val >>= 8;
    } while (val);
}

} // namespace name
