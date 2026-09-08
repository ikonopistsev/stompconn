#include "stompconn/handler.hpp"
#include <charconv>
#include <limits>
#include <stdexcept>

using namespace stompconn;

std::string_view receipt_handler::create(fn_type fn)
{
    if (receipt_seq_id_ == std::numeric_limits<std::size_t>::max())
        throw std::overflow_error("receipt sequence exhausted");
    char hex_id[2 * sizeof(std::size_t)];
    const auto end = std::to_chars(hex_id, hex_id + sizeof(hex_id),
        ++receipt_seq_id_, 16).ptr;
    auto i = receipt_.emplace(std::string{hex_id, end}, std::move(fn));
    return i.first->first;
}

bool receipt_handler::call(std::string_view id, packet p) noexcept
{
    try
    {
        auto i = receipt_.find(std::string{id});
        if (i == receipt_.end())
            return false;
        // Remove before invoking user code: it may clear the dispatcher,
        // re-enter call(), or destroy its owning connection.
        auto fn = std::move(i->second);
        receipt_.erase(i);
        try
        {
            if (fn)
                fn(std::move(p));
        }
        catch (...)
        {   }
        return true;
    }
    catch (...)
    {   }
    return false;
}

void receipt_handler::clear()
{
    receipt_.clear();
}

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
