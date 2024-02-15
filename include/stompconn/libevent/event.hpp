#pragma once

#include "stompconn/libevent/functional.hpp"

namespace stompconn {
namespace libevent {

// хэндл динамического эвента
class ev final
{
private:
    struct deallocate
    {
        void operator()(event_handle ptr) noexcept 
        { 
            event_free(ptr); 
        };
    };
    std::unique_ptr<event, deallocate> handle_{};

    auto assert_handle() const noexcept
    {
        assert(!empty());
        return handle();
    }

public:
    ev() = default;
    ev(ev&& other) = default;
    ev& operator=(ev&& other) = default;
    ~ev() = default;

    // захват хэндла
    explicit ev(event_handle handle) noexcept
        : handle_{handle}
    {
        assert(handle);
    }

    // generic
    ev(queue_handle_type queue, evutil_socket_t fd, 
        short ef, event_callback_fn fn, void *arg)
        : ev{detail::check_pointer("event_new", 
            event_new(queue, fd, ef, fn, arg))}
    {
        assert(queue && fn);
    }

    ev(queue_handle_type queue, evutil_socket_t fd, short ef,
        const timeval& tv, event_callback_fn fn, void *arg)
        : ev{queue, fd, ef, fn, arg}
    {   
        add(tv);
    }

    template<class F, class P>
    ev(queue_handle_type queue, evutil_socket_t fd, 
        short ef, const std::pair<F, P>& p)
        : ev{queue, fd, ef, p.second, p.first}
    {   }

    template<class F, class P>
    ev(queue_handle_type queue, evutil_socket_t fd, 
        short ef, const timeval& tv, const std::pair<F, P>& p)
        : ev{queue, fd, ef, tv, p.second, p.first}
    {   }

    template<class F>
    ev(queue_handle_type queue, evutil_socket_t fd, 
        short ef, F& fn)
        : ev{queue, fd, ef, proxy_call(fn)}
    {   }  

    template<class F>
    ev(queue_handle_type queue, evutil_socket_t fd, 
        short ef, const timeval& tv, F& fn)
        : ev{queue, fd, ef, tv, proxy_call(fn)}
    {   }  

    template<class F, class Rep, class Period>
    ev(queue_handle_type queue, evutil_socket_t fd, 
        short ef, std::chrono::duration<Rep, Period> timeout, F& fn)
        : ev{queue, fd, ef, detail::make_timeval(timeout), fn}
    {   } 

    template<class F>
    ev(queue_handle_type queue, short ef, F& fn)
        : ev{queue, -1, ef, fn}
    {   }  

    template<class F>
    ev(queue_handle_type queue, short ef, const timeval& tv, F& fn)
        : ev{queue, -1, ef, tv, fn}
    {   }

    template<class F, class Rep, class Period>
    ev(queue_handle_type queue, short ef, 
        std::chrono::duration<Rep, Period> timeout, F& fn)
        : ev{queue, ef, detail::make_timeval(timeout), fn}
    {   } 

    template<class F>
    ev(queue_handle_type queue, F& fn)
        : ev{queue, -1, EV_TIMEOUT, fn}
    {   }  

    template<class F>
    ev(queue_handle_type queue, const timeval& tv, F& fn)
        : ev{queue, -1, EV_TIMEOUT, tv, fn}
    {   }

    template<class F, class Rep, class Period>
    ev(queue_handle_type queue,
        std::chrono::duration<Rep, Period> timeout, F& fn)
        : ev{queue, -1, EV_TIMEOUT, timeout, fn}
    {   }    

    // создание объекта
    void create(queue_handle_type queue, evutil_socket_t fd, 
        short ef, event_callback_fn fn, void *arg)
    {
        // если создаем повторно поверх
        // значит что-то пошло не так
        assert(queue && fn && !handle_);

        // создаем объект эвента
        handle_.reset(detail::check_pointer("event_new",
            event_new(queue, fd, ef, fn, arg)));
    }

    template<class F, class P>
    void create(queue_handle_type queue, 
        evutil_socket_t fd, short ef, const std::pair<F, P>& p)
    {   
        create(queue, fd, ef, p.second, p.first);
    }

    template<class F>
    void create(queue_handle_type queue, 
        evutil_socket_t fd, short ef, F& fn)
    {
        create(queue, fd, ef, proxy_call(fn));
    }  

    template<class F, int V>
    void create(queue_handle_type queue, short ft, F& fn)
    {   
        create(queue, -1, {ft|EV_TIMEOUT}, fn);
    }  

    template<class F>
    void create(queue_handle_type queue, F& fn)
    {   
        create(queue, -1, EV_TIMEOUT, fn);
    } 

    void destroy() noexcept
    {
        handle_.reset();
    }

    event_handle handle() const noexcept
    {
        return handle_.get();
    }

    operator event_handle() const noexcept
    {
        return handle();
    }

    bool empty() const noexcept
    {
        return handle_ == nullptr;
    }

    // !!! это не выполнить на следующем цикле очереди
    // это добавить без таймаута
    // допустим вечное ожидание EV_READ или сигнала
    void add(const timeval* tv = nullptr)
    {
        detail::check_result("event_add",
            event_add(assert_handle(), tv));
    }

    void add(const timeval& tv)
    {
        add(&tv);
    }

    template<class Rep, class Period>
    void add(std::chrono::duration<Rep, Period> timeout)
    {
        add(detail::make_timeval(timeout));
    }

    void remove()
    {
        detail::check_result("event_del",
            event_del(assert_handle()));
    }

    // метод запускает эвент напрямую
    // те может привести к бесконечному вызову калбеков activate
    // можно испольновать из разных потоков (при use_threads)
    void active(int res) noexcept
    {
        event_active(assert_handle(), res, 0);
    }

    void set_priority(int priority)
    {
        detail::check_result("event_priority_set",
            event_priority_set(assert_handle(), priority));
    }

    //    Checks if a specific event is pending or scheduled
    //    @param tv if this field is not NULL, and the event has a timeout,
    //           this field is set to hold the time at which the timeout will
    //       expire.
    bool pending(timeval& tv, short events) const noexcept
    {
        return event_pending(assert_handle(), events, &tv) != 0;
    }

    // Checks if a specific event is pending or scheduled
    bool pending(short events) const noexcept
    {
        return event_pending(assert_handle(), events, nullptr) != 0;
    }

    short events() const noexcept
    {
        return event_get_events(assert_handle());
    }

    evutil_socket_t fd() const noexcept
    {
        return event_get_fd(assert_handle());
    }

    // хэндл очереди
    queue_handle_type queue_handle() const noexcept
    {
        return event_get_base(assert_handle());
    }

    operator queue_handle_type() const noexcept
    {
        return queue_handle();
    }

    event_callback_fn callback() const noexcept
    {
        return event_get_callback(assert_handle());
    }

    void* callback_arg() const noexcept
    {
        return event_get_callback_arg(assert_handle());
    }

    // выполнить обратный вызов напрямую
    void exec(short ef)
    {
        auto handle = assert_handle();
        auto fn = event_get_callback(handle);
        // должен быть каллбек
        assert(fn);
        // вызываем обработчик
        (*fn)(event_get_fd(handle), ef, event_get_callback_arg(handle));
    }    
};

timeval gettimeofday_cached(queue_handle_type queue);

static inline void make_once(queue_handle_type queue, evutil_socket_t fd, 
    short ef, const timeval& tv, event_callback_fn fn, void *arg)
{
    assert(fn);
    assert(queue);
    detail::check_result("event_base_once", 
        event_base_once(queue, fd, ef, fn, arg, &tv));    
}

template<class F, class P>
void make_once(queue_handle_type queue, evutil_socket_t fd, 
    short ef, const timeval& tv, const std::pair<F, P>& p)
{
    make_once(queue, fd, ef, tv, p.second, p.first);
}

template<class F>
void make_once(queue_handle_type queue, evutil_socket_t fd, 
    short ef, const timeval& tv, F& fn)
{
    make_once(queue, fd, ef, tv, proxy_call(fn));    
}

} // namespace libevent
} // namespace stompconn