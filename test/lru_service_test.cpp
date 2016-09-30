/*
 * lru_service_test.cpp
 *
 *  Created on: Aug 30, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <tip/lru-cache/lru_cache_service.hpp>

TEST(CacheService, KeyValue)
{
	typedef tip::lru::lru_cache_service< std::string, int > cache_type;
	boost::asio::io_service io_service;
	EXPECT_THROW(boost::asio::use_service<cache_type>(io_service), std::logic_error);
	EXPECT_NO_THROW(
			boost::asio::add_service(io_service,
					new cache_type( io_service,
							boost::posix_time::seconds(10),
							boost::posix_time::seconds(10)))
	);
	EXPECT_NO_THROW(boost::asio::use_service<cache_type>(io_service));
	{
		cache_type& cache = boost::asio::use_service<cache_type>(io_service);
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
	}
}

TEST(CacheService, KeyExtractor)
{
	typedef tip::lru::lru_cache_service< int, std::function< int(int) > > cache_type;
	boost::asio::io_service io_service;
	EXPECT_THROW(boost::asio::use_service<cache_type>(io_service), std::logic_error);
	EXPECT_NO_THROW(boost::asio::add_service(io_service, new cache_type(
			io_service,
			boost::posix_time::seconds(10),
			boost::posix_time::seconds(60),
			[](int a) { return a; } )));
	EXPECT_NO_THROW(boost::asio::use_service<cache_type>(io_service));
	{
		cache_type& cache = boost::asio::use_service<cache_type>(io_service);
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
	}
}
