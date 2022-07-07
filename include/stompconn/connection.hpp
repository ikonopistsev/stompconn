#pragma once

#include "stompconn/stomplay.hpp"
#include "stompconn/libevent.hpp"
#include "stompconn/basic_text.hpp"

namespace stompconn {

class connection
{
public:
    using on_event_type = std::function<void(short)>;
    using text_id_type = stompconn::basic_text<char, 64>;
    using hex_text_type = stompconn::basic_text<char, 20>;
    using on_error_type = stomplay::on_error_type;
private:
    
    event_base* queue_{ nullptr };
    bev bev_{};
    ev timeout_{};
    std::size_t write_timeout_{};
    std::size_t read_timeout_{};
    std::size_t bytes_writed_{};
    std::size_t bytes_readed_{};

    on_event_type event_fun_{};
    callback_type on_connect_fun_{};
    on_error_type on_error_fun_{};

    stomplay stomplay_{};

    std::size_t connection_seq_id_{};
    text_id_type connection_id_{};

    std::size_t message_seq_id_{};
    bool connecting_{false};

    template<class A>
    struct proxy
    {
        static inline void evcb(bufferevent *, short what, void *self) noexcept
        {
            assert(self);
            static_cast<A*>(self)->do_evcb(what);
        }

        static inline void recvcb(bufferevent *hbev, void *self) noexcept
        {
            assert(self);
            buffer_ref input(bufferevent_get_input(hbev));
            static_cast<A*>(self)->do_recv(std::move(input));
        }

        static inline void heart_beat(evutil_socket_t, short, void* self)
        {
            assert(self);
            static_cast<A*>(self)->send_heart_beat();
        }
    };

    void do_evcb(short what) noexcept;

    void setup_write_timeout(std::size_t timeout, double tolerant = 0.9);

    void setup_read_timeout(std::size_t timeout, double tolerant = 1.3);

    void do_recv(buffer_ref input) noexcept;

    void create();

#ifdef EVENT__HAVE_OPENSSL
    void create(struct ssl_st *ssl);
#endif

    void exec_logon(const stomplay::fun_type& fn, packet p) noexcept;

    void exec_unsubscribe(const stomplay::fun_type& fn,
                          const std::string& id, packet p) noexcept;

    void exec_event_fun(short ef) noexcept;

    void update_connection_id() noexcept;

    hex_text_type message_seq_id() noexcept;

    text_id_type create_id(char ch) noexcept;

    void setup_heart_beat(const packet& logon);

    void send_heart_beat() noexcept;

    void exec_error(std::exception_ptr ex) noexcept;

    template<class F>
    void exec(F fn) noexcept
    {
        try
        {
            fn();
        }
        catch (...)
        {
            exec_error(std::current_exception());
        }
    }

public:
    struct async
    {
        constexpr void operator()() const noexcept
        {   }
    };

    connection(event_base* queue,
               on_event_type event_fn, callback_type conn_fn) noexcept
        : queue_(queue)
        , event_fun_(event_fn)
        , on_connect_fun_(conn_fn)
    {
        assert(queue);
        assert(event_fn);
        assert(conn_fn);
    }

    ~connection();

    void connect(evdns_base* dns, const std::string& host, int port);

    void connect(evdns_base* dns, const std::string& host, int port, timeval timeout);

    void connect(const std::string& host, int port)
    {
        connect(nullptr, host, port);
    }

    template<class Rep, class Period>
    void connect(evdns_base* dns, const std::string& host, int port,
                 std::chrono::duration<Rep, Period> timeout)
    {
        connect(dns, host, port, detail::make_timeval(timeout));
    }

    template<class Rep, class Period>
    void connect(const std::string& host, int port,
        std::chrono::duration<Rep, Period> timeout)
    {
        connect(nullptr, host, port, timeout);
    }

    void disconnect() noexcept;

    // асинхронное отключение
    // допустим из собственных калбеков
    // on_event не будет вызыван
    template<class F>
    void disconnect(F fn) noexcept
    {
        try
        {
            once([this, fn]{
                exec([this, fn]{
                    disconnect();
                    fn();
                });
            });
        }
        catch (...)
        {
            exec_error(std::current_exception());
        }
    }

    const std::string& session() const noexcept
    {
        return stomplay_.session();
    }

    void unsubscribe(std::string_view id, stomplay::fun_type fn);

    template<class F>
    void unsubscribe_all(F fn) noexcept
    {
        try
        {
            auto& subs = stomplay_.subscription();
            if (subs.empty())
            {
                exec(fn);
                return;
            }

            for (auto& s : subs)
            {
                unsubscribe(std::to_string(s.first), [&, fn](auto) {
                    if (subs.empty())
                        exec(fn);
                });
            }
        }
        catch (...)
        {
            exec_error(std::current_exception());
        }
    }

    template<class F>
    void for_subscription(F fn) {
        for (auto& s : stomplay_.subscription())
            fn(s.first);
    }

    void once(timeval tv, callback_type fn);

    void once(callback_type fn)
    {
        once(timeval{0, 0}, std::move(fn));
    }

    template<class Rep, class Period>
    void once(std::chrono::duration<Rep, Period> timeout, callback_type fn)
    {
        once(detail::make_timeval(timeout), std::move(fn));
    }

    template<class F>
    void unsubscribe_logout(F fn) noexcept
    {
        unsubscribe_all([this, fn]{
            logout([this, fn](auto) {
                exec(fn);
            });
        });
    }

    // stomp DISCONNECT
    void logout(stomplay::fun_type fn);

    static auto get_ack_id(const packet& p) noexcept
    {
        return p.get_id();
    }

    // result is was asked or nacked :)
    // with_transaction_id = false by default
    void ack(const packet& p, bool with_transaction_id, stomplay::fun_type fn);

    void ack(const packet& p, stomplay::fun_type fn)
    {
        ack(p, false, std::move(fn));
    }

    void nack(const packet& p, bool with_transaction_id, stomplay::fun_type fn);

    void nack(const packet& p, stomplay::fun_type fn)
    {
        nack(p, false, std::move(fn));
    }

    void begin(std::string_view transaction_id, stomplay::fun_type fn)
    {
        assert(fn);
        send(stompconn::begin(transaction_id), std::move(fn));
    }

    void begin(std::string_view transaction_id)
    {
        send(stompconn::begin(transaction_id));
    }

    void commit(std::string_view transaction_id, stomplay::fun_type fn);

    void commit(std::string_view transaction_id);

    void commit(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);
        auto transaction_id = p.get_transaction();
        send(stompconn::commit(transaction_id), std::move(fn));
    }

    void commit(const packet& p)
    {
        auto transaction_id = p.get_transaction();
        send(stompconn::commit(transaction_id));
    }

    void abort(std::string_view transaction_id, stomplay::fun_type fn);


    void abort(std::string_view transaction_id);

    void abort(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);
        auto transaction_id = p.get_transaction();
        send(stompconn::abort(transaction_id), std::move(fn));
    }

    void abort(const packet& p)
    {
        auto transaction_id = p.get_transaction();
        send(stompconn::abort(transaction_id));
    }

    void send(stompconn::logon frame, stomplay::fun_type fn);

    void send(stompconn::subscribe frame, stomplay::fun_type fn);

    void send(stompconn::ack frame, stomplay::fun_type fn);

    void send(stompconn::nack frame, stomplay::fun_type fn);

    void send(stompconn::begin frame, stomplay::fun_type fn);

    void send(stompconn::commit frame, stomplay::fun_type fn);

    void send(stompconn::abort frame, stomplay::fun_type fn);

    void send(stompconn::send frame, stomplay::fun_type fn);

    template<class F>
    void send(F frame)
    {
        setup_write_timeout(write_timeout_);

        bytes_writed_ += frame.write(bev_);
    }

    void on_error(stomplay::fun_type fn);

    void on_except(on_error_type fn);

    text_id_type create_message_id() noexcept;

    bool connecting() const noexcept
    {
        return connecting_;
    }

    std::size_t bytes_writed() const noexcept
    {
        return bytes_writed_;
    }

    std::size_t bytes_readed() const noexcept
    {
        return bytes_readed_;
    }
};

} // namespace stomptalk

