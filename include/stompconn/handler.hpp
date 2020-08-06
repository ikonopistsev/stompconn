#pragma once

#include "stompconn/packet.hpp"
#include <functional>

namespace stompconn {

class handler
{
public:
    typedef std::function<void(packet)> fn_type;
    typedef std::unordered_map<std::string, fn_type> storage_type;
    typedef storage_type::iterator iterator;

    handler() = default;

private:
    storage_type storage_{};

    void exec(iterator i, packet p) noexcept;

public:
    void create(const std::string& receipt_id, fn_type fn);

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
