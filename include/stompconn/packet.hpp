#pragma once

#include "stompconn/libevent.hpp"
#include "stomptalk/header_store.hpp"

namespace stompconn {

class packet
{
public:
    using header_store = stomptalk::header_store;

protected:
    const header_store& header_;
    std::string_view session_{};
    std::string_view subscription_id_{};
    stomptalk::method::generic method_{};
    buffer payload_{};

public:
    packet(packet&&) = default;

    packet(const header_store& header, std::string_view session,
        stomptalk::method::generic method, buffer payload)
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
        using namespace stomptalk::method;
        return method_.num_id() == num_id::error;
    }

    operator bool() const noexcept
    {
        return !error();
    }

    template<class T>
    auto get(T) const noexcept
    {
        return header_.get(T());
    }

    auto get(std::string_view key) const noexcept
    {
        return header_.get(key);
    }

    auto get_content_type() const noexcept
    {
        return get(stomptalk::header::tag::content_type());
    }

    auto get_content_encoding() const noexcept
    {
        return get(stomptalk::header::tag::content_encoding());
    }

    auto get_correlation_id() const noexcept
    {
        return get(stomptalk::header::tag::correlation_id());
    }

    auto get_reply_to() const noexcept
    {
        return get(stomptalk::header::tag::reply_to());
    }

    auto get_expires() const noexcept
    {
        return get(stomptalk::header::tag::expires());
    }

    auto get_message_id() const noexcept
    {
        return get(stomptalk::header::tag::message_id());
    }

    auto get_amqp_type() const noexcept
    {
        return get(stomptalk::header::tag::amqp_type());
    }

    auto get_amqp_message_id() const noexcept
    {
        return get(stomptalk::header::tag::amqp_message_id());
    }

    auto get_timestamp() const noexcept
    {
        return get(stomptalk::header::tag::timestamp());
    }

    auto get_user_id() const noexcept
    {
        return get(stomptalk::header::tag::user_id());
    }

    auto get_app_id() const noexcept
    {
        return get(stomptalk::header::tag::app_id());
    }

    auto get_cluster_id() const noexcept
    {
        return get(stomptalk::header::tag::cluster_id());
    }

    auto get_ack() const noexcept
    {
        return get(stomptalk::header::tag::ack());
    }

    auto get_subscription() const noexcept
    {
        auto rc = get(stomptalk::header::tag::subscription());
        if (rc.empty())
        {
            // used for get id from subscribe receipt only!!
            rc = subscription_id_;
        }
        return rc;
    }

    auto get_destination() const noexcept
    {
        return get(stomptalk::header::tag::destination());
    }

    auto get_id() const noexcept
    {
        return get(stomptalk::header::tag::id());
    }

    auto get_transaction() const noexcept
    {
        return get(stomptalk::header::tag::transaction());
    }

    auto get_receipt() const noexcept
    {
        return get(stomptalk::header::tag::receipt());
    }

    auto get_receipt_id() const noexcept
    {
        return get(stomptalk::header::tag::receipt_id());
    }

    bool must_ack() const noexcept
    {
        return !get_ack().empty();
    }

    std::string_view session() const noexcept
    {
        return session_;
    }

    stomptalk::method::generic method() const noexcept
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
        auto method = method_.str();
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

} // namespace stompconn
