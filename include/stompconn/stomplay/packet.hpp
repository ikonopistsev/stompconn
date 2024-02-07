#pragma once

#include "stompconn/buffer.hpp"
#include "stompconn/stomplay/header_store.hpp"
#include "stomptalk/parser.h"

namespace stompconn {
namespace stomplay {

class packet
{
protected:
    const stomplay::header_store& header_;
    std::string_view session_{};
    std::string_view subscription_id_{};
    std::uint64_t method_{};
    buffer payload_{};

public:
    packet(packet&&) = default;

    packet(const stomplay::header_store& header, std::string_view session,
        std::uint64_t method, buffer payload)
        : header_(header)
        , session_(session)
        , method_(method)
        , payload_(std::move(payload))
    {   }

    void set_subscription_id(std::string_view subscription_id) noexcept
    {
        subscription_id_ = subscription_id;
    }

    bool error() const noexcept
    {
        return method_ == st_method_error;
    }

    operator bool() const noexcept
    {
        return !error();
    }

    auto get(std::string_view key) const noexcept
    {
        return header_.get(key);
    }

    auto get_content_type() const noexcept
    {
        return header_.get(st_header_content_type);
    }

    auto get_content_encoding() const noexcept
    {
        return header_.get(st_header_content_encoding);
    }

    auto get_correlation_id() const noexcept
    {
        return header_.get(st_header_correlation_id);
    }

    auto get_reply_to() const noexcept
    {
        return header_.get(st_header_reply_to);
    }

    auto get_expires() const noexcept
    {
        return header_.get(st_header_expires);
    }

    auto get_message_id() const noexcept
    {
        return header_.get(st_header_message_id);
    }

    auto get_amqp_type() const noexcept
    {
        return header_.get(st_header_amqp_type);
    }

    auto get_amqp_message_id() const noexcept
    {
        return header_.get(st_header_amqp_message_id);
    }

    auto get_timestamp() const noexcept
    {
        return header_.get(st_header_timestamp);
    }

    auto get_user_id() const noexcept
    {
        return header_.get(st_header_user_id);
    }

    auto get_app_id() const noexcept
    {
        return header_.get(st_header_app_id);
    }

    auto get_cluster_id() const noexcept
    {
        return header_.get(st_header_cluster_id);
    }

    auto get_ack() const noexcept
    {
        return header_.get(st_header_ack);
    }

    auto get_subscription() const noexcept
    {
        auto rc = header_.get(st_header_subscription);
        if (rc.empty())
        {
            // used for get id from subscribe receipt only!!
            rc = subscription_id_;
        }
        return rc;
    }

    auto get_destination() const noexcept
    {
        return header_.get(st_header_destination);
    }

    auto get_id() const noexcept
    {
        return header_.get(st_header_id);
    }

    auto get_transaction() const noexcept
    {
        return header_.get(st_header_transaction);
    }

    auto get_receipt() const noexcept
    {
        return header_.get(st_header_receipt);
    }

    auto get_receipt_id() const noexcept
    {
        return header_.get(st_header_receipt_id);
    }

    auto get_heart_beat() const noexcept
    {
        return header_.get(st_header_heart_beat);
    }

    bool must_ack() const noexcept
    {
        return !get_ack().empty();
    }

    std::string_view session() const noexcept
    {
        return session_;
    }

    auto method() const noexcept
    {
        return method_;
    }

    buffer_ref payload() const noexcept
    {
        return buffer_ref(payload_.handle());
    }

    void copyout(buffer& other)
    {
        other.append(std::move(payload_));
    }

    buffer_ref data() const noexcept
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

private:    
    static void replace_all(std::string &str, const std::string& from, const std::string& to)
    {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) 
        {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }

public:
    std::string dump(char m = ' ', char p = ' ', char h = ';') const
    {
        std::string rc;
        std::string_view method{stomptalk_method_str(method_)};
        auto header_dump = header_.dump(h);
        rc.reserve(method.size() + header_dump.size() + size() + 2);
        rc += method;
        rc += m;
        rc += header_dump;
        rc += p;
        if (!payload_.empty())
        {
            auto str = payload_.str();
            // rabbitmq issue
            replace_all(str, "\n", " ");
            replace_all(str, "\r", " ");
            replace_all(str, "\t", " ");
            std::size_t sz = 0;
            do {
                sz = str.length();
                replace_all(str, "  ", " ");
            } while (sz != str.length());

            rc += str;
        }
        return rc;
    }
};

} // namespace stomplay
} // namespace stompconn
