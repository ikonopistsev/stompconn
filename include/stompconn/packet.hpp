#pragma once

#include "stomptalk/header_store.hpp"
#include "btpro/buffer.hpp"

namespace stompconn {

class packet
{
public:
    using header_store = stomptalk::header_store;

protected:
    const header_store& header_;
    std::string_view session_{};
    stomptalk::method::generic method_{};
    btpro::buffer payload_{};

public:
    packet(packet&&) = default;

    packet(const header_store& header, std::string_view session,
        stomptalk::method::generic method, btpro::buffer_ref payload)
        : header_(header)
        , session_(session)
        , method_(method)
    {
        payload.copyout(payload_);
    }

    bool error() const noexcept
    {
        using namespace stomptalk::method;
        return method_.num_id() == num_id::error;
    }

    operator bool() const noexcept
    {
        return !error();
    }

    bool must_ack() const noexcept
    {
        return !get(stomptalk::header::tag::ack()).empty();
    }

    template<class T>
    std::string_view get(T) const noexcept
    {
        return header_.get(T());
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

    void copyout(btpro::buffer& other)
    {
        other.append(std::move(payload_));
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
        auto method = method_.str();
        auto header_dump = header_.dump();
        rc.reserve(method.size() + header_dump.size() + size() + 2);
        rc += method;
        rc += '\n';
        rc += header_dump;
        rc += '\n';
        if (!payload_.empty())
            rc += payload_.str();
        return rc;
    }
};

} // namespace stompconn
