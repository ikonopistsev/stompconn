#pragma once

#include "stompconn/header_store.hpp"
#include "btpro/buffer.hpp"

namespace stompconn {

class packet
{
protected:
    const header_store& header_;
    std::string_view session_{};
    stomptalk::method::generic method_{};
    btpro::buffer payload_{};

public:
    packet(packet&&) = default;

    packet(const header_store& header, std::string_view session,
        stomptalk::method::generic method, btpro::buffer data)
        : header_(header)
        , session_(session)
        , method_(method)
        , payload_(std::move(data))
    {   }

    packet(const header_store& header,
        stomptalk::method::generic method, btpro::buffer data)
        : header_(header)
        , method_(method)
        , payload_(std::move(data))
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

    std::string_view session() const noexcept
    {
        return session_;
    }

    stomptalk::method::generic method() const noexcept
    {
        return method_;
    }

    btpro::buffer_ref payload() const noexcept
    {
        return payload_;
    }

    btpro::buffer_ref data() const noexcept
    {
        return payload();
    }

    std::size_t size() const noexcept
    {
        return payload_.size();
    }

    std::size_t empty() const noexcept
    {
        return payload_.empty();
    }

    std::string dump() const
    {
        std::string rc;
        rc.resize(320);

        rc += method_.str();
        rc += '\n';
        rc += header_.dump();
        rc += '\n';
        if (!payload_.empty())
            rc += payload_.str();
        return rc;
    }
};

} // namespace stompconn
