#pragma once

#include "stompconn/fnv1a.hpp"

#include <string>
#include <unordered_map>
#include <cassert>

namespace stompconn {
namespace stomplay {

class header_store
{
    using version_type = std::size_t;
    using hash_type = fnv1a::type;
    using value_type = std::tuple<std::string, std::string, version_type>;
    using header_type = std::pair<std::string_view, std::string_view>;
    using storage_type = std::unordered_map<hash_type, value_type>;

    version_type version_{};
    storage_type storage_{};
    std::size_t size_{};

public:
    header_store() = default;

    void set(hash_type num_id, std::string_view key, std::string_view value)
    {
        auto f = storage_.find(num_id);
        if (f != storage_.end())
        {
            auto& [ storage_key, storage_val, 
                storage_version ] = std::get<1>(*f);
            assert(storage_key == key);
            
            // If a client or a server receives repeated frame header entries, 
            // only the first header entry SHOULD be used as the value of header entry. 
            // Subsequent values are only used to maintain a history of state changes of the header and MAY be ignored.
            if (storage_version != version_)
            {
                storage_val = value;
                storage_version = version_;
                ++size_;
            }
        }
        else
        {
            assert(stomptalk::fnv1a::calc_hash<decltype(key)>(key.begin(), 
                key.end()) == num_id);
            storage_[num_id] = std::make_tuple(std::string{key}, 
                std::string{value}, version_);
            ++size_;
        }
    }

    void set(std::string_view key, std::string_view value)
    {
       fnv1a h;
       set(h(key.data(), key.size()), key, value);
    }

    void clear() noexcept
    {
        ++version_;
        size_ = 0;
    }

    void reset()
    {
        storage_.clear();
        version_ = 0;
    }

    std::size_t size() const noexcept
    {
        return size_;
    }

    bool empty() const noexcept
    {
        return size_ == 0;
    }

    template<class F>
    std::string_view find(hash_type num_id, F fn) const
    {
        auto f = storage_.find(num_id);
        if (f != storage_.end())
        {
            const auto& [ storage_key, storage_val, 
                storage_version ] = std::get<1>(*f);
            if (storage_version == version_)
            {
                return fn(std::make_pair(std::string_view{storage_key}, 
                    std::string_view{storage_val}));
            } 
        }
        return {};
    }

    std::string_view get(std::string_view key) const noexcept
    {
        fnv1a h;
        return find(h(key.begin(), key.end()), [&](auto hdr) {
            auto& [ hdr_key, hdr_val ] = hdr;
            assert(hdr_key == key);
            return hdr_val;
        });
    }

    std::string_view get(hash_type num_id) const noexcept
    {
        return find(num_id, [&](auto hdr) {
            return std::get<1>(hdr);
        });
    }

    std::string dump(char sep = ' ') const
    {
       std::string rc;
       rc.reserve(320);

       for (auto&& h : storage_)
       {
            const auto& [ storage_key, storage_val, 
                storage_version ] = std::get<1>(h);
            if (storage_version == version_)
            {
                if (!rc.empty())
                    rc += sep;

                rc += storage_key;
                rc += ':';
                rc += storage_val;
            }
       }

       return rc;
    }
};

} // namespace stomplay
} // namespace stompconn
