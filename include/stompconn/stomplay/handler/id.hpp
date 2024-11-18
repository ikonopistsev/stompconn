#pragma once

#include <array>
#include <string>
#include <charconv>
#include <system_error>

namespace stompconn {
namespace stomplay {
namespace handler {

struct id
{
    std::string value;

    id() = default;
    id(const id&) = delete;
    id& operator=(const id&) = delete;
    id(id&&) = default;
    id& operator=(id&&) = default;

    id(std::string_view val)
        : value{val}
    {   }

    static inline std::string to_string(std::size_t id, std::size_t base = 16)
    {
        // подходит для 64-битных чисел
        constexpr auto max_digits = 16;
        std::array<char, max_digits> buf;
        auto [ last, ec ] = std::to_chars(buf.begin(), buf.end(), id, base);
        if (ec != std::errc{})
            throw std::runtime_error(std::make_error_code(ec).message());
        return { buf.begin(), last };
    }

    id(std::size_t val)
        : value{to_string(val)}
    {  }

    // приведение к строке
    operator std::string_view() const noexcept
    {
        return value;
    }

    // оператор сравнения для хэш-табилцы
    bool operator==(std::string_view other) const noexcept
    {
        return value == other;
    }
};  

} // namespace handler
} // namespace stomplay
} // namespace stompconn

namespace std {

template<>
struct hash<stompconn::stomplay::handler::id>
{
    std::size_t operator()(const stompconn::stomplay::handler::id& key) const noexcept
    {
        return std::hash<std::string>{}(key.value);
    }
};

} // namespace std
