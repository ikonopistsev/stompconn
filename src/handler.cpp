#include "stompconn/handler.hpp"

using namespace stompconn;

void handler::exec(iterator i, packet p) noexcept
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

void handler::create(const std::string& id, fn_type fn)
{
    assert(fn);

    storage_[id] = std::move(fn);
}

void handler::remove(const std::string& id)
{
    storage_.erase(id);
}

void handler::on_recepit(const std::string& receipt_id, packet p) noexcept
{
    auto f = storage_.find(receipt_id);
    if (f != storage_.end())
    {
        exec(f, std::move(p));
        storage_.erase(f);
    }
}

void handler::on_message(const std::string &id, packet p) noexcept
{
    auto f = storage_.find(id);
    if (f != storage_.end())
        exec(f, std::move(p));
}


