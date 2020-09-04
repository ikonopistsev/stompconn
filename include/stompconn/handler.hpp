#pragma once

#include "stompconn/packet.hpp"
#include <functional>

namespace stompconn {

class handler
{
public:
    using fn_type = std::function<void(packet)>;
    using storage_type = std::unordered_map<std::string, fn_type>;
    using iterator = storage_type::iterator;

    handler() = default;

private:
    storage_type storage_{};

    void exec(iterator i, packet p) noexcept;

public:
    void create(const std::string& id, fn_type fn);

    void remove(const std::string& id);

    void on_recepit(const std::string& id, packet p) noexcept;

    void on_message(const std::string& id, packet p) noexcept;

    void on_error(const std::string& id, packet p) noexcept
    {
        on_message(id, std::move(p));
    }

    void clear()
    {
        storage_.clear();
    }
};

} // namespace stomptalk
