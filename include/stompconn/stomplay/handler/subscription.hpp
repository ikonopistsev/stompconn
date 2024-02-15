#pragma once

#include "stompconn/stomplay/frame.hpp"
#include "stompconn/stomplay/handler/id.hpp"

#include <functional>
#include <charconv>
#include <map>

namespace stompconn {
namespace stomplay {
namespace handler {

class subscription
{
    using storage_type = std::unordered_map<id, frame_fun>;
    using iterator = storage_type::iterator;

private:
    std::size_t subscription_seq_id_{};
    storage_type storage_{};

    void exec(iterator i, frame p) noexcept;

public:
    subscription() = default;

    // создать новый идентификатор подписки
    // и сохранить обрабочик на данные подписки
    std::string_view create_subscription(frame_fun fn);

    // создать подписку с пользовательсикм иднетификатором
    // и сохранить обрабочик на данные подписки
    std::string_view create_subscription(std::size_t id, frame_fun fn);
    std::string_view create_subscription(std::string_view id, frame_fun fn);

    bool call(std::string_view id, frame p) noexcept;

    void erase(std::string_view id) noexcept
    {
        storage_.erase(id);
    }

    void clear() noexcept
    {
        storage_.clear();
    }

    auto begin() const noexcept
    {
        return storage_.begin();
    }

    auto end() const noexcept
    {
        return storage_.end();
    }

    auto empty() const noexcept
    {
        return storage_.empty();
    }

    auto size() const noexcept
    {
        return storage_.size();
    }
};

} // namespace handler
} // namespace stomplay
} // namespace stomptalk
