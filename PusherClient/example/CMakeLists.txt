#          Copyright Joe Coder 2004 - 2006.
#  Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          https://www.boost.org/LICENSE_1_0.txt)

find_package(Boost 1.82.0 REQUIRED)
find_package(rapidjson REQUIRED)
find_package(CURL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS}) 

# Add the source files for the example
set(SOURCES main.cpp)

set(common_link_libraries
  ${Boost_LIBRARIES}
  ${CURL_LIBRARIES}
  PusherClient
  rapidjson
)

# Create the example executable
add_executable(client_example ${SOURCES})

# Link the library to the example executable
target_link_libraries(client_example PRIVATE ${common_link_libraries})
