#pragma once

#include "stompconn/header_store.hpp"
#include "btpro/buffer.hpp"

namespace stompconn {

class packet
{
protected:
    const header_store& header_;
    stomptalk::method::generic method_{};
    btpro::buffer data_{};

public:
    packet(packet&&) = default;

    packet(const header_store& header,
        stomptalk::method::generic method, btpro::buffer data)
        : header_(header)
        , method_(method)
        , data_(std::move(data))
    {   }

    virtual ~packet() = default;

    bool error() const noexcept
    {
        return method_.num_id() == stomptalk::method::num_id::error;
    }

    operator bool() const noexcept
    {
        return !error();
    }

    template<class T>
    std::string_view get(stomptalk::header::basic<T>) const noexcept
    {
        return header_.get(stomptalk::header::basic<T>());
    }

    std::string_view get(std::string_view key) const noexcept
    {
        return header_.get(key);
    }

    stomptalk::method::generic method() const noexcept
    {
        return method_;
    }

    btpro::buffer_ref data() const noexcept
    {
        return data_;
    }

    std::string dump() const noexcept
    {
        std::string rc;
        rc += method_.str();
        rc += '\n';
        rc += header_.dump();
        rc += '\n';
        if (!data_.empty())
            rc += data_.str();
        return rc;
    }
};

} // namespace stompconn
