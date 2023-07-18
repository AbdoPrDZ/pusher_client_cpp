//          Copyright Joe Coder 2004 - 2006.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef PUSHERCLIENT_EVENT_HPP
#define PUSHERCLIENT_EVENT_HPP

#include <chrono>
#include <string>

namespace PusherClient {
  using clock = std::chrono::system_clock;

  // Structure representing a PusherClient event
  struct Event {
    std::string channel;              // Channel name
    std::string name;                 // Event name
    std::string data;                 // Event data
    clock::time_point timestamp;      // Timestamp of the event
  };
}

#endif // PUSHERCLIENT_EVENT_HPP
