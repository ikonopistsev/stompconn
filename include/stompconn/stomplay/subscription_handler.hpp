#pragma once

#include "stompconn/stomplay/packet.hpp"

#include <functional>
#include <list>

namespace stompconn {
namespace stomplay {

class subscription_handler
{
public:
    using id_type = std::string;
    
private:
    using fn_type = std::function<void(packet)>;
    using storage_type = std::unordered_map<id_type, fn_type>;
    using iterator = storage_type::iterator;

    std::size_t subscription_seq_id_{};
    storage_type subscription_{};

    void exec(iterator i, packet p) noexcept;

public:
    subscription_handler() = default;

    void create_subscription(const id_type& id, fn_type fn);

    id_type create(fn_type fn);

    bool call(const id_type& id, packet p) noexcept;

    void remove(const id_type& id) noexcept;

    void clear();

    auto begin() const noexcept
    {
        return subscription_.begin();
    }

    auto end() const noexcept
    {
        return subscription_.end();
    }

    auto empty() const noexcept
    {
        return subscription_.empty();
    }

    auto size() const noexcept
    {
        return subscription_.size();
    }
};

} // namespace stomplay
} // namespace stomptalk
