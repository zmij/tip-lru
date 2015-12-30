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

TEST(LruContainer, KeyValue)
{
//	typedef concept_check::cache_traits< std::string, int,
//			std::chrono::high_resolution_clock::time_point, void >::cache_base_type
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

	cache.get(9);
	cache.get(6);

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
//	typedef tip::util::lru_cache< std::function< int(int) > >
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

	cache.get(9);
	cache.get(6);

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
