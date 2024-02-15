#pragma once

#include "stompconn/stomplay/frame.hpp"
#include "stompconn/stomplay/protocol.hpp"
#include "stompconn/libevent/event.hpp"
#include "stompconn/libevent/buffer_event.hpp"
#include <functional>
#include <array>

namespace stompconn {

using event_fun = std::function<void(short)>;
using timer_fun = libevent::timer_fun;
using error_fun = std::function<void(std::exception_ptr)>;
using frame_fun = stomplay::protocol::fun_type;

class connection final
{   
public:
    struct status
    {
        enum type : std::size_t 
        {
            ready,      // готово к использованию
            connecting, // подключение
            running,    // подключено
            error       // ошибка
        };

        static auto sv(type s) noexcept
        {
            using namespace std::literals;
            static constexpr auto text = std::array{
                "ready"sv, "connecting"sv, "running"sv, "error"sv
            };
            return text[s];
        }

        static auto error_sv(type s) noexcept
        {
            using namespace std::literals;
            static constexpr auto text = std::array{
                "connection not ready"sv, 
                "connection is connecting"sv, 
                "connection is running"sv, 
                "connection error"sv
            };
            return text[s];
        }
    };

private:
    libevent::queue_handle_type queue_{};
    libevent::buffer_event bev_{};
    libevent::ev timeout_{};
    std::size_t write_timeout_{};
    std::size_t read_timeout_{};
    std::size_t bytes_writed_{};
    std::size_t bytes_readed_{};

    // события от libevent
    // подклчюение, отключение, ошибка
    event_fun on_event_{};
    error_fun on_error_{};
    
    stomplay::protocol stomplay_{};

    std::size_t connection_seq_id_{};
    std::string connection_id_{};

    std::size_t message_seq_id_{};
    status::type status_{status::ready};

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
    void create(struct ssl_st *ssl)
    {
        assert(ssl);

        bev_.destroy();
        timeout_.destroy();

        bev_.create(queue_, -1, ssl);

        bev_.set(&proxy<connection>::recvcb,
            nullptr, &proxy<connection>::evcb, this);

        write_timeout_ = 0;
        read_timeout_ = 0;
    }
#endif

    void exec_logon(const frame_fun& fn, stomplay::frame p) noexcept;

    void exec_unsubscribe(const frame_fun& fn,
                          const std::string& id, stomplay::frame p) noexcept;

    void exec_event_fun(short ef) noexcept;

    void update_connection_id() noexcept;

    std::string message_seq_id() noexcept
    {
        return std::to_string(++message_seq_id_);
    }

    std::string create_id(char ch) noexcept;

    void setup_heart_beat(const stomplay::frame& logon);

    void send_heart_beat() noexcept;

    void exec_error(std::exception_ptr ex) noexcept;
    // отправка подготовленного буфера в буфферэвент
    void send_command(stomplay::command& cmd);
    void send_command(stomplay::command& cmd, frame_fun fn);

    // отправка подготовленного буфера в буфферэвент
    void send_command(stomplay::body_command& cmd);
    void send_command(stomplay::body_command& cmd, frame_fun fn);

    void start_send_command(stomplay::body_command& cmd, std::size_t content_length);

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

    connection(libevent::queue_handle_type queue, event_fun fn) noexcept
        : queue_(queue)
        , on_event_(fn)
    {
        assert(queue);
        assert(fn);
    }

    ~connection();

    void connect(libevent::dns_handle_type dns, 
        const std::string& host, int port);

    void connect(libevent::dns_handle_type dns, 
        const std::string& host, int port, timeval timeout);

    void connect(const std::string& host, int port)
    {
        connect(nullptr, host, port);
    }

    template<class Rep, class Period>
    void connect(libevent::dns_handle_type dns, const std::string& host, int port,
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

    void destroy() noexcept;

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
                    destroy();
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

    void unsubscribe(std::string_view id, frame_fun fn);

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
                unsubscribe(s.first, [&, fn](auto) {
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

    void once(timeval tv, timer_fun fn);

    void once(timer_fun fn)
    {
        once(timeval{0, 0}, std::move(fn));
    }

    template<class Rep, class Period>
    void once(std::chrono::duration<Rep, Period> timeout, timer_fun fn)
    {
        once(detail::make_timeval(timeout), std::move(fn));
    }

    template<class F>
    void unsubscribe_logout(F fn) noexcept
    {
        unsubscribe_all([&, fn]{
            logout([&, fn](auto) {
                exec(fn);
            });
        });
    }

    // stomp DISCONNECT
    void logout(frame_fun fn)
    {               
        if (status_ != status::running)
            throw std::runtime_error(status::error_sv(status_).data());

        stomplay::logout cmd;
        send_command(cmd, std::move(fn));
    }

    // stomp DISCONNECT
    void logout()
    {
        only_running(*this, [&]{
            // сервер отключит нас
            // когда принимает команду DISCONNECT
            stomplay::logout cmd;
            send_command(cmd);
        });
    }

    static auto get_ack_id(const stomplay::frame& p) noexcept
    {
        return p.get_id();
    }

    // result is was asked or nacked :)
    // with_transaction_id = false by default
    void ack(const stomplay::frame& p, bool with_transaction_id, frame_fun fn);

    void ack(const stomplay::frame& p, frame_fun fn)
    {
        ack(p, false, std::move(fn));
    }

    void nack(const stomplay::frame& p, bool with_transaction_id, frame_fun fn);

    void nack(const stomplay::frame& p, frame_fun fn)
    {
        nack(p, false, std::move(fn));
    }

    void send(stomplay::logon cmd, frame_fun fn);

    void send(stomplay::subscribe cmd, frame_fun fn);

    // ACK
    void send(stomplay::ack cmd, frame_fun fn)
    {
        send_command(cmd, std::move(fn));
    }

    // NACK
    void send(stomplay::nack cmd, frame_fun fn)
    {
        send_command(cmd, std::move(fn));
    }

    // BEGIN
    void send(stomplay::begin cmd, frame_fun fn)
    {
        send_command(cmd, std::move(fn));
    }

    void send(stomplay::begin cmd)
    {
        send_command(cmd);
    }

    void begin(std::string_view transaction_id, frame_fun fn)
    {
        send(stomplay::begin(transaction_id), std::move(fn));
    }

    void begin(std::string_view transaction_id)
    {
        send(stomplay::begin{transaction_id});
    }

    // COMMIT
    void send(stomplay::commit cmd, frame_fun fn)
    {
        send_command(cmd, std::move(fn));
    }

    void send(stomplay::commit cmd)
    {
        send_command(cmd);
    }

    void commit(std::string_view transaction_id, frame_fun fn)
    {
        send(stomplay::commit{transaction_id}, std::move(fn));
    }

    void commit(std::string_view transaction_id)
    {
        send(stomplay::commit{transaction_id});
    }

    void commit(const stomplay::frame& p, frame_fun fn)
    {
        auto transaction_id = p.get_transaction();
        commit(transaction_id, std::move(fn));
    }

    void commit(const stomplay::frame& p)
    {
        auto transaction_id = p.get_transaction();
        commit(transaction_id);
    }

    // ABORT
    void send(stomplay::abort cmd, frame_fun fn)
    {
        send_command(cmd, std::move(fn));
    }

    void send(stomplay::abort cmd)
    {
        send_command(cmd);
    }

    void abort(std::string_view transaction_id, frame_fun fn)
    {
        send(stomplay::abort{transaction_id}, std::move(fn));
    }

    void abort(std::string_view transaction_id)
    {
        send(stomplay::abort{transaction_id});
    }

    void abort(const stomplay::frame& p, frame_fun fn)
    {
        auto transaction_id = p.get_transaction();
        abort(transaction_id, std::move(fn));
    }

    void abort(const stomplay::frame& p)
    {
        auto transaction_id = p.get_transaction();
        abort(transaction_id);
    }

    void send(stomplay::send cmd, frame_fun fn)
    {
        send_command(cmd, std::move(fn));
    }

    //void send(stomplay::send_temp frame, frame_fun fn);

    void on_error(frame_fun fn);

    void on_except(error_fun fn);

    std::string create_message_id() noexcept;

    bool state(status::type s) const noexcept
    {
        return status_ == s;
    }

    template<class F>
    void at_running(F fn) const
    {
        if (state(status::running))
            fn();
    }

    template<class F>
    void at_ready(F fn) const
    {
        if (state(status::ready))
            fn();
    }

    template<class F>
    static void only_running(const connection& conn, F fn)
    {
        auto st = status::running;
        if (conn.state(st))
            fn();
        else
            throw std::runtime_error(status::error_sv(st).data());
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

