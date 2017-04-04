/*
 * lru_cache_service.hpp
 *
 *  Created on: Aug 30, 2015
 *      Author: zmij
 */

#ifndef TIP_LRU_CACHE_LRU_CACHE_SERVICE_HPP_
#define TIP_LRU_CACHE_LRU_CACHE_SERVICE_HPP_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#pragma GCC diagnostic pop

#include <tip/lru-cache/lru_cache.hpp>

#include <functional>

namespace tip {
namespace lru {

template < typename Value,
        typename GetKey,
        typename GetTime = boost::posix_time::ptime,
        typename SetTime = void >
class lru_cache_service : public boost::asio::detail::service_base<
            lru_cache_service<Value, GetKey, GetTime, SetTime>>,
        public util::lru_cache< Value, GetKey, GetTime, SetTime > {
public:
    using this_type         =  lru_cache_service< Value, GetKey, GetTime, SetTime >;
    using io_service        =  boost::asio::io_service;
    using service_base      = boost::asio::detail::service_base<
                                    lru_cache_service<Value, GetKey, GetTime, SetTime>
                                >;
    using container_base    = util::lru_cache< Value, GetKey, GetTime, SetTime >;
    using key_intrusive     = typename container_base::key_intrusive;
    using time_intrusive    = typename container_base::time_intrusive;

    using duration_type     = typename container_base::duration_type;
    using deadline_timer    = boost::asio::deadline_timer;
    using timer_iterval_type= deadline_timer::duration_type;

    using key_type          = typename container_base::key_type;
    using value_type        = typename container_base::value_type;
    using erase_callback    = ::std::function< void(value_type const&) >;
public:
    //@{
    /** @name Constructors required for compiling use_service template function */
    lru_cache_service( io_service& owner ) :
            service_base(owner), container_base(),
            timer_interval_(), max_age_(),
            timer_(owner, timer_interval_)
    {
        throw std::logic_error("LRU Cache service must be added manually to "
                "io_service before it can be used");
    }
    //@}
    //@{
    /** @name Actual constructors */
    template < typename U = this_type,
        typename = typename std::enable_if< !U::key_intrusive::value && !U::time_intrusive::value >::type >
    lru_cache_service( io_service& owner,
                timer_iterval_type timer_interval,
                duration_type max_age ) :
            service_base(owner), container_base(),
            timer_interval_(timer_interval), max_age_(max_age),
            timer_(owner, timer_interval_)
    {
        start_timer();
    }
    template < typename U = this_type,
        typename = typename std::enable_if< U::key_intrusive::value && !U::time_intrusive::value >::type >
    lru_cache_service( io_service& owner,
                timer_iterval_type timer_interval,
                duration_type max_age,
                typename U::key_extraction_type::get_key_function get_key) :
            service_base(owner), container_base(get_key),
            timer_interval_(timer_interval), max_age_(max_age),
            timer_(owner, timer_interval_)
    {
        start_timer();
    }
    template < typename U = this_type,
        typename = typename std::enable_if< U::key_intrusive::value && U::time_intrusive::value >::type >
    lru_cache_service( io_service& owner,
                timer_iterval_type timer_interval,
                duration_type max_age,
                typename U::key_extraction_type::get_key_function get_key,
                typename U::time_handling_type::get_time_function get_time,
                typename U::time_handling_type::set_time_function set_time) :
            service_base(owner), container_base(get_key, get_time, set_time),
            timer_interval_(timer_interval), max_age_(max_age),
            timer_(owner, timer_interval_)
    {
        start_timer();
    }
    template < typename U = this_type,
        typename = typename std::enable_if< !U::key_intrusive::value && U::time_intrusive::value >::type >
    lru_cache_service( io_service& owner,
                timer_iterval_type timer_interval,
                duration_type max_age,
                typename U::time_handling_type::get_time_function get_time,
                typename U::time_handling_type::set_time_function set_time) :
            service_base(owner), container_base(get_time, set_time),
            timer_interval_(timer_interval), max_age_(max_age),
            timer_(owner, timer_interval_)
    {
        start_timer();
    }
    //@}

    virtual ~lru_cache_service() {}

    void
    set_on_erase(erase_callback on_erase)
    { on_erase_ = on_erase; }
private:
    virtual void
    shutdown_service()
    {
        timer_.cancel();
        container_base::clear();
    }
    void
    start_timer()
    {
        timer_.async_wait(
                std::bind(&lru_cache_service::timer_expired,
                        this, std::placeholders::_1));
    }
    void
    timer_expired( boost::system::error_code const& ec)
    {
        if (!ec) {
            container_base::expire(max_age_, on_erase_);
            timer_.expires_at(timer_.expires_at() + timer_interval_);
            start_timer();
        }
    }
private:
    timer_iterval_type  timer_interval_;
    duration_type       max_age_;
    deadline_timer      timer_;
    erase_callback      on_erase_;
};

}  // namespace lru
}  // namespace tip

#define LRU_CACHE_KEYINTRUSIVE_SERVICE(io_service, namespace_, class_, cleanup, timeout, get_key)\
boost::asio::add_service(io_service, \
        new namespace_::class_##_lru_service( \
            io_service, \
            boost::posix_time::seconds(cleanup), \
            boost::posix_time::minutes(timeout), \
            [](namespace_::class_##_ptr obj) { return get_key; }))

#define LRU_CACHE_TIMEKEYINTRUSIVE_SERVICE(io_service, namespace_, class_, cleanup, timeout, get_key, get_time, set_time) \
boost::asio::add_service(io_service, \
        new namespace_::class_##_lru_service( \
            io_service, \
            boost::posix_time::seconds(cleanup), \
            boost::posix_time::minutes(timeout), \
            [](namespace_::class_##_ptr obj) { return get_key; }, \
            [](namespace_::class_##_ptr obj) { return get_time; }, \
            [](namespace_::class_##_ptr obj, boost::posix_time::ptime tm) { set_time; }))


#endif /* TIP_LRU_CACHE_LRU_CACHE_SERVICE_HPP_ */
