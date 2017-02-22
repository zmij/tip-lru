/*
 * lru_container_test.cpp
 *
 *  Created on: Aug 30, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <thread>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <tip/lru-cache/lru_cache.hpp>
#include <chrono>

TEST(LruContainer, KeyValue)
{
//    typedef concept_check::cache_traits< std::string, int,
//            std::chrono::high_resolution_clock::time_point, void >::cache_base_type
    typedef tip::util::lru_cache<std::string, int>
                str_cache_type;
    str_cache_type cache;
    cache.put(0, "zero");
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    cache.put(4, "four");
    cache.put(5, "five");
    cache.put(6, "six");
    cache.put(7, "seven");
    cache.put(8, "eight");
    cache.put(9, "nine");
    EXPECT_EQ(10, cache.size());
    cache.shrink(5);
    EXPECT_TRUE(cache.exists(9));
    EXPECT_TRUE(cache.exists(8));
    EXPECT_TRUE(cache.exists(7));
    EXPECT_TRUE(cache.exists(6));
    EXPECT_TRUE(cache.exists(5));
    EXPECT_FALSE(cache.exists(4));
    EXPECT_FALSE(cache.exists(3));
    EXPECT_FALSE(cache.exists(2));
    EXPECT_FALSE(cache.exists(1));
    EXPECT_FALSE(cache.exists(0));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ::std::string val;
    EXPECT_TRUE(cache.get(9, val));
    EXPECT_TRUE(cache.get(6, val));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    cache.expire(std::chrono::milliseconds(150));
    EXPECT_TRUE(cache.exists(9));
    EXPECT_TRUE(cache.exists(6));

    EXPECT_FALSE(cache.exists(8));
    EXPECT_FALSE(cache.exists(7));
    EXPECT_FALSE(cache.exists(5));

    cache.expire(std::chrono::milliseconds(10));
    EXPECT_TRUE(cache.empty());
}

TEST(LruContainer, KeyExtractor)
{
//    typedef tip::util::lru_cache< std::function< int(int) > >
    typedef tip::util::lru_cache<int, std::function< int(int) >>
            int_cache_type;
    int_cache_type cache([](int a) { return a; } );
    cache.put(0);
    cache.put(1);
    cache.put(2);
    cache.put(3);
    cache.put(4);
    cache.put(5);
    cache.put(6);
    cache.put(7);
    cache.put(8);
    cache.put(9);
    EXPECT_EQ(10, cache.size());
    cache.shrink(5);
    EXPECT_TRUE(cache.exists(9));
    EXPECT_TRUE(cache.exists(8));
    EXPECT_TRUE(cache.exists(7));
    EXPECT_TRUE(cache.exists(6));
    EXPECT_TRUE(cache.exists(5));
    EXPECT_FALSE(cache.exists(4));
    EXPECT_FALSE(cache.exists(3));
    EXPECT_FALSE(cache.exists(2));
    EXPECT_FALSE(cache.exists(1));
    EXPECT_FALSE(cache.exists(0));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int val;
    EXPECT_TRUE(cache.get(9, val));
    EXPECT_TRUE(cache.get(6, val));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    cache.expire(std::chrono::milliseconds(150));
    EXPECT_TRUE(cache.exists(9));
    EXPECT_TRUE(cache.exists(6));

    EXPECT_FALSE(cache.exists(8));
    EXPECT_FALSE(cache.exists(7));
    EXPECT_FALSE(cache.exists(5));

    cache.expire(std::chrono::milliseconds(10));
    EXPECT_TRUE(cache.empty());
}

struct test_struct {
    using on_destroy = ::std::function<void()>;
    using time_point = ::std::chrono::system_clock::time_point;
    using get_time   = ::std::function< time_point(test_struct const&) >;
    using set_time   = ::std::function< void(test_struct&, time_point) >;

    on_destroy on_destroy_;
    time_point mtime_;

    ~test_struct()
    {
        if (on_destroy_) on_destroy_();
    }
};

TEST(LruContainer, ItemDestroy)
{
    using test_lru = tip::util::lru_cache<test_struct, int>;

    int count = 0;
    auto on_destroy = [&](){--count;};

    test_lru lru;

    lru.put(0, test_struct{ on_destroy });
    lru.put(1, test_struct{ on_destroy });
    lru.put(2, test_struct{ on_destroy });
    lru.put(3, test_struct{ on_destroy });
    lru.put(4, test_struct{ on_destroy });
    lru.put(5, test_struct{ on_destroy });
    lru.put(6, test_struct{ on_destroy });
    lru.put(7, test_struct{ on_destroy });
    lru.put(8, test_struct{ on_destroy });
    lru.put(9, test_struct{ on_destroy });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    lru.put(10, test_struct{ on_destroy });
    lru.put(11, test_struct{ on_destroy });
    lru.put(12, test_struct{ on_destroy });
    lru.put(13, test_struct{ on_destroy });
    lru.put(14, test_struct{ on_destroy });
    lru.put(15, test_struct{ on_destroy });
    lru.put(16, test_struct{ on_destroy });
    lru.put(17, test_struct{ on_destroy });
    lru.put(18, test_struct{ on_destroy });
    lru.put(19, test_struct{ on_destroy });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    count = lru.size();
    EXPECT_EQ(20, lru.size());

    lru.expire(std::chrono::milliseconds(150));
    EXPECT_EQ(10, count);
    EXPECT_EQ(10, lru.size());
    lru.expire(std::chrono::milliseconds(50));
    EXPECT_EQ(0, count);
    EXPECT_EQ(0, lru.size());
}

TEST(LruContainer, IntrusiveTime)
{
    using test_lru = tip::util::lru_cache<test_struct, int,
            test_struct::get_time, test_struct::set_time>;
    using clock = ::std::chrono::system_clock;

    int count = 0;
    auto on_destroy = [&](){--count;};

    test_lru lru{
        [](test_struct const& v){ return v.mtime_; },
        [](test_struct& v, test_struct::time_point tm) { v.mtime_ = tm; }
    };

    lru.put(0, test_struct{ on_destroy, clock::now() });
    lru.put(1, test_struct{ on_destroy, clock::now() });
    lru.put(2, test_struct{ on_destroy, clock::now() });
    lru.put(3, test_struct{ on_destroy, clock::now() });
    lru.put(4, test_struct{ on_destroy, clock::now() });
    lru.put(5, test_struct{ on_destroy, clock::now() });
    lru.put(6, test_struct{ on_destroy, clock::now() });
    lru.put(7, test_struct{ on_destroy, clock::now() });
    lru.put(8, test_struct{ on_destroy, clock::now() });
    lru.put(9, test_struct{ on_destroy, clock::now() });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    lru.put(10, test_struct{ on_destroy, clock::now() });
    lru.put(11, test_struct{ on_destroy, clock::now() });
    lru.put(12, test_struct{ on_destroy, clock::now() });
    lru.put(13, test_struct{ on_destroy, clock::now() });
    lru.put(14, test_struct{ on_destroy, clock::now() });
    lru.put(15, test_struct{ on_destroy, clock::now() });
    lru.put(16, test_struct{ on_destroy, clock::now() });
    lru.put(17, test_struct{ on_destroy, clock::now() });
    lru.put(18, test_struct{ on_destroy, clock::now() });
    lru.put(19, test_struct{ on_destroy, clock::now() });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    count = lru.size();
    EXPECT_EQ(20, lru.size());

    lru.expire(std::chrono::milliseconds(15));
    EXPECT_EQ(10, count);
    EXPECT_EQ(10, lru.size());
    lru.expire(std::chrono::milliseconds(5));
    EXPECT_EQ(0, count);
    EXPECT_EQ(0, lru.size());

    lru.put(1, test_struct{ on_destroy, clock::now() });
    count = lru.size();
    EXPECT_EQ(1, lru.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    lru.expire(std::chrono::milliseconds(5));
    EXPECT_EQ(0, count);
    EXPECT_EQ(0, lru.size());
}


