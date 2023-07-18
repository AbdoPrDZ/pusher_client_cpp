//          Copyright Joe Coder 2004 - 2006.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef PUSHERCLIENT_CLIENT_SIGNAL_FILTER_HPP
#define PUSHERCLIENT_CLIENT_SIGNAL_FILTER_HPP

#include <map>
#include <string>
#include <type_traits>
#include <utility>

#include <boost/signals2.hpp>

#include <PusherClient/event.hpp>

namespace PusherClient {
  namespace client {
    namespace channel {

      // Define type aliases for convenience
      using SignalMutex = boost::signals2::keywords::mutex_type<boost::signals2::dummy_mutex>;
      using Signal = boost::signals2::signal_type<void(PusherClient::Event const&), SignalMutex>::type;

      using SignalMap = std::map<std::string, Signal>;
      using ScopedConnection = boost::signals2::scoped_connection;

      template<typename FilterT>
      class SignalFilter {

      public:
        Signal* source_; // Pointer to the source signal
        std::decay_t<FilterT> filter_; // Filter function
        SignalMap filtered_; // Map of filtered signals

        explicit SignalFilter(FilterT&& filter)
        : source_{nullptr}
        , filter_{std::forward<FilterT>(filter)}
        , filtered_{} {}

        // Connect the source signal to the filtered signals
        auto connectSource(Signal& source) {
          source_ = &source;
          source_->connect([this](PusherClient::Event const& ev) {
            auto name = filter_(ev);
            if (!name.empty()) {
              auto it = filtered_.find(name);
              if (it != std::end(filtered_))
                it->second(ev);
            }
          });
        }

        // Connect a function to the source signal
        template<typename FuncT>
        auto connect(FuncT&& func) {
          return source_->connect(std::forward<FuncT>(func));
        }

        // Connect a function to a filtered signal based on the name
        template<typename FuncT>
        auto connect(std::string const& name, FuncT&& func) {
          return filtered_[name].connect(std::forward<FuncT>(func));
        }
      };

      // Helper function to create a SignalFilter object
      template<typename FilterT>
      SignalFilter<FilterT> filteredSignal(FilterT&& filter) {
        return SignalFilter<FilterT>{std::forward<FilterT>(filter)};
      }

      // Filter function that filters events by channel
      inline std::string byChannel(const PusherClient::Event& ev) {
        return ev.channel;
      }

      // Filter function that filters events by name
      inline std::string byName(const PusherClient::Event& ev) {
        return ev.name;
      }

    }
  }
}

#endif // PUSHERCLIENT_CLIENT_SIGNAL_FILTER_HPP
