cmake_minimum_required(VERSION 3.0)

cmake_policy(SET CMP0048 NEW)

project (event_dispatcher VERSION 0.0.1 LANGUAGES CXX)
find_package (Boost)
find_package (OpenDDS)
add_library(event_dispatch Barrier.cpp ThreadPool.cpp SystemTimer.cpp EventDispatcher.cpp AsioEventDispatcher.cpp AsioSystemTimer.cpp ProactorEventDispatcher.cpp ProactorSystemTimer.cpp ReactorEventDispatcher.cpp ReactorSystemTimer.cpp)
target_compile_options(event_dispatch PUBLIC -Wall -pedantic -Werror -Wno-unused -g -std=${OPENDDS_STD})
target_link_libraries(event_dispatch OpenDDS::Dcps ${Boost_SYSTEM_LIBRARY} ${Boost_DATE_TIME_LIBRARY})

project(event_dispatcher_tests VERSION 0.0.1 LANGUAGES CXX)
find_package(PkgConfig)
pkg_search_module(GTEST REQUIRED gtest_main)
add_executable(event_test event_test.cpp)
target_compile_options(event_test PUBLIC -Wall -pedantic -Werror -Wno-unused -g -std=${OPENDDS_STD})
target_link_libraries(event_test event_dispatch ${GTEST_LDFLAGS})
target_compile_options(event_test PUBLIC ${GTEST_CFLAGS})
