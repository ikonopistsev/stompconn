#pragma once

#include "stompconn/fnv1a.hpp"

#include <string>
#include <unordered_map>
#include <cassert>

namespace stompconn {

class header_store
{
    using version_type = std::size_t;
    using hash_type = fnv1a::type;
    using value_type = std::tuple<std::string, std::string, version_type>;
    using header_type = std::pair<std::string_view, std::string_view>;
    using storage_type = std::unordered_map<hash_type, value_type>;

    version_type version_{1};
    storage_type storage_{};

public:
    header_store() = default;

    void set(hash_type num_id, std::string_view key, std::string_view value)
    {
        auto f = storage_.find(num_id);
        if (f != storage_.end())
        {
            auto& t = std::get<1>(*f);
            
            auto& storage_key = std::get<0>(t);
            assert(storage_key == key);

            std::get<1>(t) = value;
            std::get<2>(t) = version_;
        }
        else
        {
            storage_[num_id] = std::make_tuple(std::string{key}, 
                std::string{value}, version_);
        }
    }

    void set(std::string_view key, std::string_view value)
    {
       fnv1a h;
       set(h(key.begin(), key.end()), key, value);
    }

    void clear() noexcept
    {
        ++version_;
    }

    void reset()
    {
        storage_.clear();
        version_ = 1;
    }

    template<class F>
    std::string_view find(hash_type num_id, F fn) const
    {
        auto f = storage_.find(num_id);
        if (f != storage_.end())
        {
            const auto& t = std::get<1>(*f);
            if (std::get<2>(t) == version_)
            {
                return fn(std::make_pair(std::string_view{std::get<0>(t)}, 
                    std::string_view{std::get<1>(t)}));
            }
        }
        return {};
    }

    std::string_view get(std::string_view key) const noexcept
    {
        fnv1a h;
        return find(h(key.begin(), key.end()), [&](auto hdr) {
            assert(std::get<0>(hdr) == key);
            return std::get<1>(hdr);
        });
    }

    std::string_view get(hash_type num_id) const noexcept
    {
        return find(num_id, [&](auto hdr) {
            return std::get<1>(hdr);
        });
    }

    std::string dump() const
    {
       std::string rc;
       rc.reserve(320);

       for (auto& h : storage_)
       {
            const auto& t = std::get<1>(h);
            if (std::get<2>(t) == version_)
            {
                if (!rc.empty())
                    rc += ' ';

                rc += '\"';
                rc += std::get<0>(t);
                rc += '\"';
                rc += ':';
                rc += '\"';
                rc += std::get<1>(t);
                rc += '\"';
            }
       }

       return rc;
    }
};

} // namespace stompconn
