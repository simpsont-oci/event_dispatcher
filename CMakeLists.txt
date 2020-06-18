cmake_minimum_required(VERSION 3.0)

cmake_policy(SET CMP0048 NEW)

project (event_dispatcher)
find_package (Boost COMPONENTS system date_time REQUIRED)
SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror -Wno-unused -g -std=c++14")
add_library(event_dispatch Barrier.cpp ThreadPool.cpp SystemTimer.cpp EventDispatcher.cpp AsioEventDispatcher.cpp AsioSystemTimer.cpp ProactorEventDispatcher.cpp ProactorSystemTimer.cpp ReactorEventDispatcher.cpp)
target_include_directories(event_dispatch PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../OpenDDS/ACE_TAO/ACE>)
target_link_directories(event_dispatch PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../OpenDDS/ACE_TAO/ACE/lib>)
target_link_libraries(event_dispatch ACE ${Boost_SYSTEM_LIBRARY} ${Boost_DATE_TIME_LIBRARY} pthread)

project(my_gtest_pkgconfig VERSION 0.0.1 LANGUAGES CXX)
find_package(PkgConfig)
pkg_search_module(GTEST REQUIRED gtest_main)
SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror -Wno-unused -g -std=c++14")
add_executable(event_test event_test.cpp)
target_link_libraries(event_test event_dispatch ${GTEST_LDFLAGS})
target_compile_options(event_test PUBLIC ${GTEST_CFLAGS})
