#pragma once

#include "stompconn/libevent/type.hpp"
#include <functional>

namespace stompconn {
namespace libevent {

template<class T>
struct timer_fn final
{
    using fn_type = void (T::*)();

    fn_type fn_{};
    T& self_;

    void call() noexcept
    {
        assert(fn_);
        try {
            (self_.*fn_)();
        } 
        catch (...)
        {   }
    }
};

template<class T>
struct generic_fn final
{
    using fn_type = void (T::*)(evutil_socket_t fd, short ef);

    fn_type fn_{};
    T& self_; 

    void call(evutil_socket_t fd, short ef) noexcept
    {
        assert(fn_);
        try {
            (self_.*fn_)(fd, ef);
        } 
        catch (...)
        {   }
    }
};

template<class T>
auto proxy_call(timer_fn<T>& fn)
{
    return std::make_pair(&fn,
        [](evutil_socket_t, short, void *arg){
            assert(arg);
            try {
                static_cast<timer_fn<T>*>(arg)->call();
            }
            catch (...)
            {   }
        });
}

template<class T>
auto proxy_call(generic_fn<T>& fn)
{
    return std::make_pair(&fn,
        [](evutil_socket_t fd, short ef, void *arg){
            assert(arg);
            try {
                static_cast<generic_fn<T>*>(arg)->call(fd, ef);
            }
            catch (...)
            {   }
        });
}

using timer_fun = std::function<void()>;
using generic_fun = 
    std::function<void(evutil_socket_t fd, short ef)>;

auto proxy_call(timer_fun& fn)
{
    return std::make_pair(&fn,
        [](evutil_socket_t, short, void *arg){
            assert(arg);
            auto fn = static_cast<timer_fun*>(arg);
            try {
                (*fn)();
            }
            catch (...)
            {   }
        });
}
 
auto proxy_call(generic_fun& fn)
{
    return std::make_pair(&fn,
        [](evutil_socket_t fd, short ef, void *arg){
            assert(arg);
            auto fn = static_cast<generic_fun*>(arg);
            try {
                (*fn)(fd, ef);
            }
            catch (...)
            {   }
        });
}

template<class F>
auto proxy_call(F&& fn)
{
    return std::make_pair(new generic_fun{std::forward<F>(fn)},
        [](evutil_socket_t fd, short ef, void *arg){
            assert(arg);
            auto fn = static_cast<F*>(arg);
            try {
                (*fn)(fd, ef);
            }
            catch (...)
            {   }
            delete fn;
        });
}

} // namespace libevent
} // namespace stompconn