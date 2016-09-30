#    CMakeLists.txt for tip-lru library
#
#    @author zmij
#    @date Nov 30, 2015

cmake_minimum_required(VERSION 2.6)

set(lib_name lru)
string(TOUPPER ${lib_name} LIB_NAME)

add_definitions("-std=c++11")

set(
    lru_cache_SRCS
    include/tip/lru-cache/lru_cache.hpp
    include/tip/lru-cache/lru_cache_service.hpp
)

install(
    FILES ${lru_cache_SRCS}
    DESTINATION include/tip/lru-cache
)

if(BUILD_TESTS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
    enable_testing()
    add_subdirectory(test)
endif()

get_directory_property(has_parent PARENT_DIRECTORY)
if (has_parent)
    set(TIP_${LIB_NAME}_LIB ${PROJECT_PREFIX}-${lib_name} CACHE INTERNAL "Name of tip-lru library target")
    set(TIP_${LIB_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "Path to tip-lru libaray includes" )
endif()
