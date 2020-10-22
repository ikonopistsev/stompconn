#pragma once

#include "stompconn/packet.hpp"
#include "stomptalk/basic_text.hpp"
#include <functional>

namespace stompconn {

class handler
{
public:
    using fn_type = std::function<void(packet)>;
    using storage_type = std::unordered_map<std::size_t, fn_type>;
    using iterator = storage_type::iterator;

    handler() = default;

private:
    storage_type storage_{};

    void exec(iterator i, packet p) noexcept;

public:
    void create(std::string_view id, fn_type fn);

    void remove(std::string_view id);

    void on_recepit(std::string_view id, packet p) noexcept;

    void on_message(std::string_view id, packet p) noexcept;

    void on_error(std::string_view id, packet p) noexcept
    {
        on_message(id, std::move(p));
    }

    void clear()
    {
        storage_.clear();
    }
};

} // namespace stomptalk
