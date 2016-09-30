#	/awm-server/lib/lru/test/CMakeLists.txt
#	
#	@author zmij
#	@date Dec 30, 2015

cmake_minimum_required(VERSION 2.6)

if (NOT GTEST_INCLUDE_DIRS)
    find_package(GTest REQUIRED)
endif()
if (NOT CMAKE_THREAD_LIBS_INIT)
    find_package(Threads REQUIRED)
endif()

if (NOT Boost_SYSTEM_LIBRARIES)
    find_package(Boost 1.58 COMPONENTS system REQUIRED)
endif()

include_directories(${GTEST_INCLUDE_DIRS})

set(
	lru_test_SRCS
    lru_container_test.cpp
    lru_service_test.cpp
)
add_executable(test-lru ${lru_test_SRCS})
target_link_libraries(
	test-lru
	${GTEST_BOTH_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${Boost_SYSTEM_LIBRARIES}
)

if (GTEST_XML_OUTPUT)
    set (
        TEST_ARGS
        --gtest_output=xml:test-utils-detail.xml
    )
endif()

add_test(
    NAME test-lru
    COMMAND test-lru ${TEST_ARGS}
)
