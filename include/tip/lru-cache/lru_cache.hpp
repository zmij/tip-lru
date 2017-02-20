/*
 * lru_cache.hpp
 *
 *  Created on: Aug 30, 2015
 *      Author: zmij
 */

#ifndef TIP_LRU_CACHE_LRU_CACHE_HPP_
#define TIP_LRU_CACHE_LRU_CACHE_HPP_

#include <unordered_map>
#include <functional>
#include <list>
#include <mutex>
#include <memory>
#include <chrono>

#include <iostream>
#include <sstream>
#include <atomic>

namespace tip {
namespace util {

namespace detail {

//@{
/** @name Implementation selection flags */
struct non_intrusive {
    enum {
        value = false
    };
};
struct intrusive {
    enum {
        value = true
    };
};
//@}

template < typename Clock >
struct clock_traits;
template < typename TimeType >
struct time_traits;


template < >
struct clock_traits< ::std::chrono::high_resolution_clock > {
    using clock_type        = ::std::chrono::high_resolution_clock;
    using time_type         = clock_type::time_point;
    using duration_type     = clock_type::duration;

    static time_type
    now()
    {
        return clock_type::now();
    }
};
template < >
struct time_traits< ::std::chrono::high_resolution_clock::time_point > {
    using clock_type = ::std::chrono::high_resolution_clock;
};


#ifdef POSIX_TIME_TYPES_HPP___
template < >
struct clock_traits< boost::posix_time::microsec_clock > {
    using clock_type        = boost::posix_time::microsec_clock;
    using time_type         = boost::posix_time::ptime;
    using duration_type     = boost::posix_time::time_duration;

    static time_type
    now()
    {
        return clock_type::universal_time();
    }
};
template < >
struct time_traits< boost::posix_time::ptime > {
    using clock_type = boost::posix_time::microsec_clock;
};
#endif

template < typename Value, typename Key >
struct key_extraction_traits {
    using type                  = non_intrusive;
    using key_type              = Key;
    using value_type            = Value;
};

template < typename Value, typename Key >
struct key_extraction_traits< Value, ::std::function< Key(Value) > > {
    using type                  = intrusive;
    using key_type              = Key;
    using value_type            = Value;
    using get_key_function      = ::std::function< key_type(value_type) >;
};

template < typename Value, typename Key >
struct key_extraction_traits< Value, ::std::function< Key(Value const&) > > {
    using type                  = intrusive;
    using key_type              = Key;
    using value_type            = Value;
    using get_key_function      = ::std::function< key_type(value_type const&) >;
};

template < typename Value, typename TimeGet, typename TimeSet >
struct time_handling_traits;

template < typename Value, typename TimeType >
struct time_handling_traits< Value, TimeType, void > {
    using type                  = non_intrusive;

    using time_traits_type      = time_traits< TimeType >;
    using clock_type            = typename time_traits_type::clock_type;
    using clock_traits_type     = clock_traits< clock_type >;
    using time_type             = typename clock_traits_type::time_type;
    using duration_type         = typename clock_traits_type::duration_type;
};

template < typename Value, typename TimeType >
struct time_handling_traits<
        Value,
        ::std::function< TimeType(Value const&) >,
        ::std::function< void (Value&, TimeType) >> {
    using type                  = intrusive;

    using time_traits_type      = time_traits< TimeType >;
    using clock_type            = typename time_traits_type::clock_type;
    using clock_traits_type     = clock_traits< clock_type >;
    using time_type             = typename clock_traits_type::time_type;
    using duration_type         = typename clock_traits_type::duration_type;
    using get_time_function     = ::std::function< TimeType(Value const&) >;
    using set_time_function     = ::std::function< void (Value&, TimeType) >;
};

template < typename Value, typename TimeType >
struct time_handling_traits<
        Value,
        ::std::function< TimeType(Value) >,
        ::std::function< void (Value, TimeType) >> {
    using type                  = intrusive;

    using time_traits_type      = time_traits< TimeType >;
    using clock_type            = typename time_traits_type::clock_type;
    using clock_traits_type     = clock_traits< clock_type >;
    using time_type             = typename clock_traits_type::time_type;
    using duration_type         = typename clock_traits_type::duration_type;
    using get_time_function     = ::std::function< TimeType(Value) >;
    using set_time_function     = ::std::function< void (Value, TimeType) >;
};

template < typename KeyExtraction, typename TimeHandling >
struct cache_types {
    using key_extraction_type   = KeyExtraction;
    using time_handling_type    = TimeHandling;
    using key_type              = typename key_extraction_type::key_type;
    using value_type            = typename key_extraction_type::value_type;
    using time_type             = typename time_handling_type::time_type;
    using duration_type         = typename time_handling_type::duration_type;
    using clock_traits_type     = typename time_handling_type::clock_traits_type;
};

template < typename CacheTypes, typename ValueHolder >
class cache_container {
public:
    using types             = CacheTypes;
    using value_holder      = ValueHolder;
    using value_type        = typename types::value_type;
    using key_type          = typename types::key_type;
    using time_type         = typename types::time_type;
    using duration_type     = typename types::duration_type;
    using clock_traits_type = typename types::clock_traits_type;
    using element_type      = ::std::shared_ptr< value_holder >;
    using erase_callback    = ::std::function< void(key_type const&) >;
protected:
    using lru_list_type     = ::std::list< element_type >;
    using list_iterator     = typename lru_list_type::iterator;
    using lru_map_type      = ::std::unordered_map< key_type, list_iterator>;
    using get_key_function  = ::std::function< key_type(element_type) >;
    using get_time_function = ::std::function< time_type(element_type) >;
    using set_time_function = ::std::function<void(element_type,time_type)>;
    using mutex_type        = ::std::mutex;
    using lock_type         = ::std::lock_guard<mutex_type>;
public:
    cache_container()
    {
        throw ::std::logic_error("Cache container should be constructed with "
                "data extraction functions");
    }
    cache_container(get_key_function key_fn, get_time_function get_time_fn,
            set_time_function set_time_fn) :
                get_key_(key_fn), get_time_(get_time_fn), set_time_(set_time_fn)
    {
    }
protected:
    void
    put( key_type const& key, element_type elem)
    {
        lock_type lock(mutex_);
        erase_no_lock(key);
        set_time_(elem, clock_traits_type::now());
        cache_list_.push_front(elem);
        cache_map_.insert(::std::make_pair(key, cache_list_.begin()));
        empty_ = false;
    }
public:
    void
    erase(key_type const& key)
    {
        lock_type lock(mutex_);
        erase_no_lock(key);
        empty_ = cache_list_.empty();
    }
    template < typename InputIterator, typename ExtractId >
    void
    erase(InputIterator first, InputIterator last, ExtractId func)
    {
        lock_type lock(mutex_);
        for (; first != last; ++first) {
            erase_no_lock(func(*first));
        }
        empty_ = cache_list_.empty();
    }
    bool
    get(key_type const& key, value_type& val) const
    {
        lock_type lock(mutex_);
        auto f = cache_map_.find(key);
        if (f == cache_map_.end()) {
            return false;
        }
        cache_list_.splice(cache_list_.begin(), cache_list_, f->second);
        set_time_(*f->second, clock_traits_type::now());
        val = (*f->second)->value_;
        return true;
    }
    void
    shrink(size_t max_size)
    {
        lock_type lock(mutex_);
        while (cache_list_.size() > max_size) {
            auto last = cache_list_.end();
            --last;
            cache_map_.erase(get_key_(*last));
            cache_list_.pop_back();
        }
    }
    void
    expire(duration_type age, erase_callback on_erase = nullptr)
    {
        lru_list_type to_destroy;
        {
            lock_type lock(mutex_);
            time_type now = clock_traits_type::now();
            time_type eldest = now - age;
            auto p = cache_list_.rbegin();
            for (; p != cache_list_.rend() && get_time_(*p) < eldest; ++p) {
                auto key = get_key_(*p);
                cache_map_.erase(key);
                if (on_erase) {
                    on_erase(key);
                }
            }
            if (p == cache_list_.rend()) {
                to_destroy.splice(to_destroy.end(), cache_list_);
            } else {
                if (get_time_(*p) >= eldest)
                    --p;
                to_destroy.splice(to_destroy.end(), cache_list_, p.base(), cache_list_.end());
            }
            empty_ = cache_list_.empty();
        }
    }
    void
    clear()
    {
        lock_type lock(mutex_);
        cache_list_.clear();
        cache_map_.clear();
        empty_ = true;
    }
    bool
    exists(key_type const& key) const
    {
        lock_type lock(mutex_);
        return cache_map_.find(key) != cache_map_.end();
    }
    bool
    empty() const
    {
        return empty_;
    }
    ::std::size_t
    size() const
    {
        lock_type lock(mutex_);
        return cache_list_.size();
    }
private:
    void
    erase_no_lock(key_type const& key)
    {
        auto f = cache_map_.find(key);
        if (f != cache_map_.end()) {
            cache_list_.erase(f->second);
            cache_map_.erase(f);
        }
    }
private:
    mutex_type mutable      mutex_;
    lru_list_type mutable   cache_list_;
    lru_map_type            cache_map_;
    ::std::atomic<bool>     empty_{true};

    get_key_function        get_key_;
    get_time_function       get_time_;
    set_time_function       set_time_;
};

template < typename KeyTag, typename TimeTag, typename KeyExtraction, typename TimeHandling >
struct cache_value_holder;

template < typename KeyExtraction, typename TimeHandling >
struct cache_value_holder < non_intrusive, non_intrusive, KeyExtraction, TimeHandling > {
    using types = cache_types < KeyExtraction, TimeHandling >;
    typename types::key_type    key_;
    typename types::value_type  value_;
    typename types::time_type   access_time_;
};

template < typename KeyExtraction, typename TimeHandling >
struct cache_value_holder < intrusive, non_intrusive, KeyExtraction, TimeHandling > {
    using types = cache_types < KeyExtraction, TimeHandling >;
    typename types::value_type  value_;
    typename types::time_type   access_time_;
};

template < typename KeyExtraction, typename TimeHandling >
struct cache_value_holder < non_intrusive, intrusive, KeyExtraction, TimeHandling > {
    using types = cache_types < KeyExtraction, TimeHandling >;
    typename types::key_type    key_;
    typename types::value_type  value_;
};

template < typename KeyExtraction, typename TimeHandling >
struct cache_value_holder < intrusive, intrusive, KeyExtraction, TimeHandling > {
    using types = cache_types < KeyExtraction, TimeHandling >;
    typename types::value_type  value_;
};

template < typename KeyTag, typename TimeTag, typename KeyExtraction, typename TimeHandling >
class basic_cache;

template < typename KeyExtraction, typename TimeHandling >
class basic_cache <non_intrusive, non_intrusive, KeyExtraction, TimeHandling > :
        public cache_container<
                cache_types< KeyExtraction, TimeHandling >,
                cache_value_holder< non_intrusive, non_intrusive,
                        KeyExtraction, TimeHandling >
            > {
public:
    using types                 = cache_types < KeyExtraction, TimeHandling >;
    using value_holder_type     = cache_value_holder< non_intrusive, non_intrusive,
                                    KeyExtraction, TimeHandling >;
    using base_type             = cache_container< types, value_holder_type >;
    using element_type          = typename base_type::element_type;
public:
    basic_cache() :
        base_type(
            [](element_type elem)
            { return elem->key_; },
            [](element_type elem)
            { return elem->access_time_; },
            [](element_type elem, typename types::time_type tm)
            { elem->access_time_ = tm; }
        )
    {}

    void
    put(typename types::key_type const& key, typename types::value_type const& value)
    {
        base_type::put(key,
                element_type( new value_holder_type{ key, value, typename types::time_type{} }) );
    }
};

template < typename KeyExtraction, typename TimeHandling >
class basic_cache<intrusive, non_intrusive, KeyExtraction, TimeHandling > :
        public cache_container<
                cache_types< KeyExtraction, TimeHandling >,
                cache_value_holder< intrusive, non_intrusive,
                        KeyExtraction, TimeHandling >
            > {
public:
    using types                 = cache_types < KeyExtraction, TimeHandling >;
    using value_holder_type     = cache_value_holder< intrusive, non_intrusive,
                                    KeyExtraction, TimeHandling >;
    using base_type             = cache_container< types, value_holder_type >;
    using element_type          = typename base_type::element_type;
    using key_extraction_type   = typename types::key_extraction_type;
    using get_key_function      = typename key_extraction_type::get_key_function;
public:
    basic_cache() : base_type()
    {
    }
    basic_cache(get_key_function get_key) :
        base_type(
            [get_key](element_type elem)
            { return get_key( elem->value_ ); },
            [](element_type elem)
            { return elem->access_time_; },
            [](element_type elem, typename types::time_type tm)
            { elem->access_time_ = tm; }
        ), get_key_(get_key)
    {}

    void
    put(typename types::value_type const& value)
    {
        typename types::key_type const& key = get_key_(value);
        base_type::put(key,
                element_type( new value_holder_type{ value, typename types::time_type{} }) );
    }
private:
    get_key_function get_key_;
};

template < typename KeyExtraction, typename TimeHandling >
class basic_cache<intrusive, intrusive, KeyExtraction, TimeHandling > :
        public cache_container<
                cache_types< KeyExtraction, TimeHandling >,
                cache_value_holder< intrusive, intrusive,
                        KeyExtraction, TimeHandling >
            > {
public:
    using types                 = cache_types < KeyExtraction, TimeHandling >;
    using value_holder_type     = cache_value_holder< intrusive, intrusive,
                                    KeyExtraction, TimeHandling >;
    using base_type             = cache_container< types, value_holder_type >;
    using element_type          = typename base_type::element_type;
    using key_extraction_type   = typename types::key_extraction_type;
    using time_handling_type    = typename types::time_handling_type;
    using get_key_function      = typename key_extraction_type::get_key_function;
    using get_time_function     = typename time_handling_type::get_time_function;
    using set_time_function     = typename time_handling_type::set_time_function;
public:
    basic_cache() : base_type()
    {
    }
    basic_cache(
            get_key_function get_key,
            get_time_function get_time,
            set_time_function set_time) :
        base_type(
            [get_key](element_type elem)
            { return get_key( elem->value_ ); },
            [get_time](element_type elem)
            { return get_time(elem->value_); },
            [set_time](element_type elem, typename types::time_type tm)
            { set_time(elem->value_, tm); }
        ), get_key_(get_key)
    {}
    void
    put(typename types::value_type const& value)
    {
        typename types::key_type const& key = get_key_(value);
        base_type::put(key,
                element_type( new value_holder_type{ value }) );
    }
private:
    get_key_function get_key_;
};

template < typename KeyExtraction, typename TimeHandling >
class basic_cache<non_intrusive, intrusive, KeyExtraction, TimeHandling> :
        public cache_container<
                cache_types< KeyExtraction, TimeHandling >,
                cache_value_holder< non_intrusive, intrusive,
                        KeyExtraction, TimeHandling >
            > {
public:
    using types                 = cache_types < KeyExtraction, TimeHandling >;
    using value_holder_type     = cache_value_holder< non_intrusive, intrusive,
                                    KeyExtraction, TimeHandling >;
    using base_type             = cache_container< types, value_holder_type >;
    using element_type          = typename base_type::element_type;
    using time_handling_type    = typename types::time_handling_type;
    using get_time_function     = typename time_handling_type::get_time_function;
    using set_time_function     = typename time_handling_type::set_time_function;
public:
    basic_cache() : base_type()
    {
    }
    basic_cache(
            get_time_function get_time,
            set_time_function set_time) :
        base_type(
            [](element_type elem)
            { return elem->key_; },
            [get_time](element_type elem)
            { return get_time(elem->value_); },
            [set_time](element_type elem, typename types::time_type tm)
            { set_time(elem->value_, tm); }
        )
    {}
    void
    put(typename types::key_type const& key, typename types::value_type const& value)
    {
        base_type::put(key,
                element_type( new value_holder_type{ key, value }) );
    }
};

template < typename Value, typename Key,
        typename T0 = ::std::chrono::high_resolution_clock::time_point,
        typename T1 = void >
struct cache_traits {
    using value_type            = Value;
    using key_extraction_type   = key_extraction_traits< Value, Key >;
    using time_handling_type    = time_handling_traits< Value, T0, T1 >;
    using key_type              = typename key_extraction_type::key_type;
    using clock_type            = typename time_handling_type::clock_type;
    using time_type             = typename time_handling_type::time_type;
    using duration_type         = typename time_handling_type::duration_type;

    using key_intrusive         = typename key_extraction_type::type;
    using time_intrusive        = typename time_handling_type::type;

    using cache_base_type = basic_cache<
            key_intrusive,
            time_intrusive,
            key_extraction_type,
            time_handling_type
        >;
};
}  // namespace detail

template < typename ValueType,
    typename KeyType,
    typename GetTime = ::std::chrono::high_resolution_clock::time_point,
    typename SetTime = void >
class lru_cache :
        public detail::cache_traits< ValueType, KeyType, GetTime, SetTime >::cache_base_type {
public:
    using this_type         = lru_cache< ValueType, KeyType, GetTime, SetTime >;
    using traits_type       = detail::cache_traits< ValueType, KeyType, GetTime, SetTime >;
    using value_type        = typename traits_type::value_type;
    using key_type          = typename traits_type::key_type;
    using time_type         = typename traits_type::time_type;
    using base_type         = typename traits_type::cache_base_type;
    using key_intrusive     = typename traits_type::key_intrusive;
    using time_intrusive    = typename traits_type::time_intrusive;
public:
    lru_cache()
        : base_type()
    {
    }
    template < typename U = this_type,
        typename SFINAE = typename
            ::std::enable_if< U::key_intrusive::value && !U::time_intrusive::value >::type >
    lru_cache(typename U::key_extraction_type::get_key_function key_extract)
        : base_type(key_extract)
    {
    }
    template < typename U = this_type, typename SFINAE =
            typename ::std::enable_if< U::key_intrusive::value && U::time_intrusive::value >::type >
    lru_cache(typename U::key_extraction_type::get_key_function key_extract,
            typename U::time_handling_type::get_time_function get_time,
            typename U::time_handling_type::set_time_function set_time)
        : base_type(key_extract, get_time, set_time)
    {
    }
    template < typename U = this_type, typename SFINAE =
            typename ::std::enable_if< !U::key_intrusive::value && U::time_intrusive::value >::type >
    lru_cache(typename U::time_handling_type::get_time_function get_time,
            typename U::time_handling_type::set_time_function set_time)
        : base_type(get_time, set_time)
    {
    }
};

}  // namespace util
}  // namespace tip

#endif /* TIP_LRU_CACHE_LRU_CACHE_HPP_ */
