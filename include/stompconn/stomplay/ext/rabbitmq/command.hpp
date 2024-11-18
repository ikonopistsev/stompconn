#pragma once

#include "stompconn/stomplay/command.hpp"

namespace stompconn {
namespace stomplay {
namespace rabbitmq {

// rabbitmq temp-queue feature
// https://www.rabbitmq.com/stomp.html#d.tqd
class send_temp final
    : public stomplay::body_command
{
public:
    typedef std::function<void(packet)> fn_type;
    
private:
    fn_type fn_{};
    std::string reply_to_{};

public:
    send_temp(std::string_view destination, std::string_view reply_to, fn_type fn)
        : body_command{"SEND"}
        : fn_{std::move(fn)}
        , reply_to_{reply_to}
    {
        value_not_empty(destination, "destination empty"sv);
        value_not_empty(reply_to, "reply_to empty"sv);
        push(header::destination(destination));
        push(header::reply_to(reply_to));
    }

    // возвращает идентификатор подписки
    std::string add_subscribe(subscription_handler& handler)
    {
        handler.create_subscription(reply_to_, std::move(fn_));
        return reply_to_;
    }
};

} // namespace rabbitmq
} // namespace stomplay
} // namespace stompconn

