#include "stompconn/handler.hpp"
#include <iostream>

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

void handler::create(std::string_view id, fn_type fn)
{
    assert(fn);

    auto hash = stomptalk::get_hash(id);
    storage_[hash] = std::move(fn);
}

void handler::remove(std::string_view id)
{
    storage_.erase(stomptalk::get_hash(id));
}

void handler::on_recepit(std::string_view id, packet p) noexcept
{
    auto hash = stomptalk::get_hash(id);
    auto f = storage_.find(hash);
    if (f != storage_.end())
    {
        exec(f, std::move(p));
        storage_.erase(f);
    }
}

void handler::on_message(std::string_view id, packet p) noexcept
{
    auto f = storage_.find(stomptalk::get_hash(id));
    if (f != storage_.end())
        exec(f, std::move(p));
}


