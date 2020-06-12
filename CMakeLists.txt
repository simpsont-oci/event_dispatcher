cmake_minimum_required (VERSION 2.6) 
project (event_dispatcher) 

find_package (Boost COMPONENTS unit_test_framework system date_time REQUIRED)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  #SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-declarations -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wno-unused -g -std=c++14")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  #SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wno-unused -g -std=c++14")
  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror -Wno-unused -g -std=c++14")
endif()

add_executable(event_test SystemTimer.cpp EventDispatcher.cpp ProactorEventDispatcher.cpp ProactorSystemTimer.cpp event_test.cpp)
target_include_directories(event_test PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../OpenDDS/ACE_TAO/ACE>)
target_link_directories(event_test PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../OpenDDS/ACE_TAO/ACE/lib>)
target_link_libraries(event_test ACE ${Boost_SYSTEM_LIBRARY} ${Boost_DATE_TIME_LIBRARY} pthread)

#enable_testing()
#add_test(NAME CppToolsTest COMMAND cpptools_test)
