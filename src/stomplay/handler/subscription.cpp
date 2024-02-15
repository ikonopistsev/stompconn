#include "stompconn/stomplay/handler/subscription.hpp"

namespace stompconn {
namespace stomplay {
namespace handler {

void subscription::exec(iterator i, frame p) noexcept
{
    try
    {
        auto& fn = std::get<1>(*i);
        assert(fn);

        fn(std::move(p));
    }
    catch (...)
    {   }
}

template<class S, class T>
auto try_emplace(S& storage, T&& id, frame_fun fn)
{
    if (!fn)
        throw std::runtime_error("subs handler empty");

    auto [ ptr, rc ] = storage.try_emplace(std::forward<T>(id), std::move(fn));
    if (!rc)
        throw std::runtime_error("subscription exist");

    return ptr->first;
}

std::string_view subscription::create_subscription(std::size_t id, frame_fun fn)
{
    return try_emplace(storage_, id, std::move(fn));
}

std::string_view subscription::create_subscription(std::string_view id, frame_fun fn)
{
    if (id.empty())
        throw std::runtime_error("subs id empty");

    return try_emplace(storage_, id, std::move(fn));
}

std::string_view subscription::create_subscription(frame_fun fn)
{
    return create_subscription(++subscription_seq_id_, std::move(fn));
}

bool subscription::call(std::string_view id, frame p) noexcept
{
    auto f = storage_.find(id);
    if (f != storage_.end())
    {
        exec(f, std::move(p));
        return true;
    }

    return false;
}

} // namespace handler
} // namespace stomplay
} // namespace stompconn