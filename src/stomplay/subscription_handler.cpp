#include "stompconn/stomplay/subscription_handler.hpp"

namespace stompconn {
namespace stomplay {


void subscription_handler::exec(iterator i, packet p) noexcept
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

void subscription_handler::create_subscription(const id_type& id, fn_type fn)
{
    if (!fn)
        throw std::runtime_error("handler empty");

    auto f = subscription_.try_emplace(id, std::move(fn));
    if (!f.second)
        throw std::runtime_error("subscription exist");
}

subscription_handler::id_type subscription_handler::create(fn_type fn)
{
    auto id = std::to_string(++subscription_seq_id_);
    create_subscription(id, std::move(fn));
    return id;
}

void subscription_handler::remove(const id_type& id) noexcept
{
    subscription_.erase(id);
}

bool subscription_handler::call(const id_type& id, packet p) noexcept
{
    auto f = subscription_.find(id);
    if (f != subscription_.end())
    {
        exec(f, std::move(p));
        return true;
    }

    return false;
}


void subscription_handler::clear()
{
    subscription_.clear();
}

} // namespace stomplay
} // namespace stompconn