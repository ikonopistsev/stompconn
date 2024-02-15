#pragma once

#include "stompconn/buffer.hpp"
#include "stompconn/buffer_event.hpp"
#include "stompconn/stomplay/header.hpp"
//#include "stompconn/stomplay/header_store.hpp"
#include "stompconn/stomplay/handler/subscription.hpp"
#include <functional>
#include <stdexcept>

namespace stompconn {
namespace stomplay {

class command
{
protected:
    buffer data_{};

    constexpr command(std::string_view method)
    {   
        using namespace std::literals;
        value_not_empty(method, "method empty"sv);
        append(method);
    }
    virtual ~command() = default;

public:
    command(const command&) = delete;
    command& operator=(const command&) = delete;
    command(command&&) = default;
    command& operator=(command&&) = default;

    template<class V>
    constexpr static void value_not_empty(const V& value, std::string_view msg)
    {
        if (value.empty())
            throw std::logic_error(std::string{msg});
    }

    template<class K, class V>
    constexpr static void header_not_empty(const K& key, const V& value)
    {
        using namespace std::literals;
        if (key.empty())
            throw std::logic_error("header key empty");
        value_not_empty(value, "header value empty"sv);
    }

protected:
    constexpr void append(std::string_view text)
    {
        using namespace std::literals;
        value_not_empty(text, "text empty"sv);
        data_.append(text);        
    }

public:
    template<class B>
    std::size_t write_cmd(B& bev)
    {
        using namespace std::literals;
        data_.append("\n\n\0"sv);
        auto sz = data_.size();
        bev.write(std::move(data_));
        return sz;
    }

    void push(std::string_view key, std::string_view value)
    {
        header_not_empty(key, value);

        using namespace std::literals;

        data_.append("\n"sv);
        data_.append(key);
        data_.append(":"sv);
        data_.append(value);
    }

    void push(header::known_ref hdr)
    {
        append(hdr.text);
    }

    template<class K, class V>
    void push(const header::known<K,V>& hdr)
    {
        append(hdr.prepared_key);
        append(hdr.value);
    }

    std::string str() const
    {
        using namespace std::literals;
        auto rc = data_.str();
        rc += "\n\n"sv;
        return rc;
    }

    std::string dump() const
    {
        auto str = data_.str();
        if (!str.empty())
        {
            std::replace(std::begin(str), std::end(str), '\n', ' ');
            std::replace(std::begin(str), std::end(str), '\r', ' ');
            std::replace(std::begin(str), std::end(str), '\t', ' ');
            std::size_t sz = 0;
            do {
                sz = str.length();
                frame::replace_all(str, "  ", " ");
            } while (sz != str.length());
        }
        return str;
    }    
};

class logon final
    : public command
{  
public:
    logon(std::string_view host,
        std::string_view login, std::string_view passcode)
        : command{"CONNECT"}
    {
        using namespace std::literals;
        push(header::accept_version_v12());

        if (host.empty())
            host = "/"sv;
        push(header::host(host));

        if (!login.empty())
            push(header::login(login));
        if (!passcode.empty())
            push(header::passcode(passcode));
    }

    logon(std::string_view host, std::string_view login)
        : logon{host, login, std::string_view()}
    {   }

    logon(std::string_view host)
        : logon{host, std::string_view(), std::string_view()}
    {   }
};

class logout final
    : public command
{
public:
    logout()
        : command{"DISCONNECT"}
    {   }
};

class subscribe final
    : public command
{
    frame_fun fn_{};

public:
    subscribe(std::string_view destination, frame_fun fn)
        : command{"SUBSCRIBE"}
        , fn_{std::move(fn)}
    {
        using namespace std::literals;
        value_not_empty(destination, "destination empty"sv);
        push(header::destination(destination));
    }

    std::string_view add_subscribe(handler::subscription& subs)
    {
        auto subscription_id = subs.create_subscription(std::move(fn_));
        push(header::id(subscription_id));
        return subscription_id;
    }
};

class unsubscribe final
    : public command
{
    frame_fun fn_{};

public:
    unsubscribe(std::string_view subs_id, frame_fun fn)
        : command{"UNSUBSCRIBE"}
        , fn_{std::move(fn)}
    {
        using namespace std::literals;
        value_not_empty(subs_id, "subscription id empty"sv);
        push(header::id(subs_id));
    }
};

// команды с данными
class body_command
    : command
{
protected:
    buffer payload_{};

    using command::value_not_empty;
    using command::header_not_empty;
    using command::append;

    constexpr body_command(std::string_view method)
        : command{method}
    {   }
    virtual ~body_command() = default;

public:
    body_command(const body_command&) = delete;
    body_command& operator=(const body_command&) = delete;
    body_command(body_command&&) = default;
    body_command& operator=(body_command&&) = default;

    using command::push;
    // добавить данные к фрейму
    void push_payload(buffer payload)
    {
        payload_.append(std::move(payload));
    }

    void push(buffer payload)
    {
        push_payload(std::move(payload));
    }

    void push_payload(const char *data, std::size_t size)
    {
        payload_.append(data, size);
    }

    void push(const char *data, std::size_t size)
    {
        push_payload(data, size);
    }

    template<class B>
    std::size_t write_body_cmd(B& output, std::size_t payload_size)
    {
        using namespace std::literals;
        // дописываем размер
        push(header::content_length(payload_size));
        // дописываем разделитель хидеров и боди
        data_.append("\n\n"sv);
        // добавляем боди
        data_.append(std::move(payload_));

        return write_cmd(output, std::move(data_));
    }  

    template<class B>
    std::size_t write_body_cmd(B& output)
    {
        using namespace std::literals;
        auto payload_size = payload_.size();
        if (payload_size)
        {
            // дописываем размер
            push(header::content_length(payload_size));

            // дописываем разделитель хидеров и боди
            data_.append("\n\n"sv);

            // дописываем протокольный ноль
            payload_.append("\0"sv);

            // добавляем боди
            data_.append(std::move(payload_));
        }
        else
        {
            data_.append("\n\n\0"sv);
        }

        return write_cmd(output, std::move(data_));
    }   

private:
    template<class B>
    static std::size_t write_cmd(B& output, libevent::buffer data)
    {
        auto size = data.size();
        output.write(std::move(data));
        return size;
    }
};

class send
    : public body_command
{
public:
    send(std::string_view destination)
        : body_command{"SEND"}
    {
        using namespace std::literals;
        value_not_empty(destination, "destination empty"sv);
        push(header::destination(destination));
    }    
};


class ack
    : public command
{
public:
    ack(std::string_view ack_id)
        : command{"ACK"}
    {
        using namespace std::literals;
        value_not_empty(ack_id, "id empty"sv);
        push(header::id(ack_id));
    }
};

class nack
    : public command
{
public:
    nack(std::string_view ack_id)
        : command{"NACK"}
    {
        using namespace std::literals;
        value_not_empty(ack_id, "id empty"sv);
        push(header::id(ack_id));
    }
};

class begin
    : public command
{
public:
    begin(std::string_view transaction_id)
        : command{"BEGIN"}
    {
        using namespace std::literals;
        value_not_empty(transaction_id, "transaction id empty"sv);        
        push(header::transaction(transaction_id));
    }

    begin(std::size_t transaction_id)
        : begin{std::to_string(transaction_id)}
    {   }    
};

class commit
    : public command
{
public:
    commit(std::string_view transaction_id)
        : command{"COMMIT"}
    {
        using namespace std::literals;
        value_not_empty(transaction_id, "transaction id empty"sv);        
        push(header::transaction(transaction_id));
    }    
};

class abort
    : public command
{
public:
    abort(std::string_view transaction_id)
        : command{"ABORT"}
    {
        using namespace std::literals;
        value_not_empty(transaction_id, "transaction id empty"sv);
        push(header::transaction(transaction_id));
    }    
};

class receipt
    : public command
{
public:
    receipt(std::string_view receipt_id)
        : command{"RECEIPT"}
    {
        using namespace std::literals;
        value_not_empty(receipt_id, "receipt id empty"sv);
        push(header::receipt_id(receipt_id));
    }    
};

class connected
    : public command
{
public:
    connected(std::string_view session, std::string_view server_version)
        : command{"CONNECTED"}
    {
        using namespace std::literals;
        push(header::version_v12());
        if (!server_version.empty())
            push(header::server(server_version));
        if (!session.empty())
            push(header::session(session));
    }

    connected(std::string_view session)
        : connected{session, std::string_view()}
    {   }    
};

class error
    : public body_command
{
public:
    error(std::string_view message, std::string_view receipt_id)
        : body_command{"ERROR"}
    {
        using namespace std::literals;
        value_not_empty(message, "message empty"sv);
        push(header::message(message));
        if (!receipt_id.empty())
            push(header::receipt_id(receipt_id));
    }

    error(std::string_view message)
        : error{message, std::string_view()}
    {   }
};

class message
    : public body_command
{
public:
    message(std::string_view destination,
        std::string_view subscrition, std::string_view message_id)
        : body_command{"MESSAGE"}
    {
        using namespace std::literals;
        value_not_empty(destination, "destination empty"sv);
        value_not_empty(subscrition, "subscrition empty"sv);
        value_not_empty(message_id, "message id empty"sv);
        push(header::destination(destination));
        push(header::subscription(subscrition));
        push(header::message_id(message_id));
    }

    message(std::string_view destination,
        std::string_view subscrition, std::size_t message_id)
        : message{destination, subscrition, std::to_string(message_id)}
    {   }            
};

} // namespace stompaly
} // namespace stompconn

