#          Copyright Joe Coder 2004 - 2006.
#  Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          https://www.boost.org/LICENSE_1_0.txt)

# Add the include directories for your library
set(PusherClient_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "")
include_directories(PusherClient_INCLUDE_DIRS)

find_package(Boost 1.82.0 REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(cryptopp CONFIG REQUIRED)
find_package(CURL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS}) 

# Create the library target
add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE
  ${Boost_INCLUDE_DIRS}
  ${PusherClient_INCLUDE_DIRS}
)

target_link_libraries(PusherClient INTERFACE
  ${Boost_LIBRARIES}
  ${CURL_LIBRARIES}
  rapidjson
  cryptopp::cryptopp
)

# Include the example directory
add_subdirectory(example)
