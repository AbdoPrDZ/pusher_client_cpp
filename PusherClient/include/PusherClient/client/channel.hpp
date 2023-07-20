//          Copyright Joe Coder 2004 - 2006.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef PUSHERCLIENT_CLIENT_CHANNEL_CHANNEL_PROXY_HPP
#define PUSHERCLIENT_CLIENT_CHANNEL_CHANNEL_PROXY_HPP

#include <string>
#include <map>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/beast/websocket.hpp>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>

#include <PusherClient/client.hpp>
#include <PusherClient/event.hpp>
#include "channel/signal_filter.hpp"

namespace PusherClient {
  
  template<typename SocketT>
  class Client;

  namespace client {

    namespace channel {
  
      template<typename SocketT>
      class Channel {
        using SignalFilter = SignalFilter<std::string(*)(PusherClient::Event const&)>;
        using AuthCallback = std::function<rapidjson::Document(const std::string&, const std::string&)>;

        PusherClient::Client<SocketT>* client_;
        SignalFilter* signalFilter_;

      public:
        const std::string name;
        bool subscribed = false;

        explicit Channel(PusherClient::Client<SocketT>* client, const std::string channelName, bool subscribe = true)
          : name{channelName}
          , client_{client}
          , signalFilter_{nullptr}
        {
          auto result = init();
          // If the channel was newly inserted, subscribe to it when the client is connected
          if (subscribe && result.second)
            if (client_->connected) this->subscribe(); // subscribe if already connected
            else client_->onConnect([this](const PusherClient::Event& event) { // connect when is connected
              this->subscribe();
            });
        }

        explicit Channel(PusherClient::Client<SocketT>* client, const std::string channelName, const std::string auth)
          : name{channelName}
          , client_{client}
          , signalFilter_{nullptr}
        {
          // init channel
          auto result = init();

          // If the channel was newly inserted, subscribe to it when the client is connected
          if (result.second)
            if (client_->connected) subscribe(auth); // subscribe if already connected
            else client_->onConnect([this, &auth](const PusherClient::Event& event) { // connect when is connected
              subscribe(auth);
            });
        }

        explicit Channel(PusherClient::Client<SocketT>* client, const std::string channelName, const AuthCallback authCallback)
          : name{channelName}
          , client_{client}
          , signalFilter_{nullptr}
        {
          // init channel
          auto result = init();

          // If the channel was newly inserted, subscribe to it when the client is connected
          if (result.second)
            if (client_->connected) subscribe(authCallback); // subscribe if already connected
            else client_->onConnect([this, &authCallback](const PusherClient::Event& event) { // connect when is connected
              subscribe(authCallback);
            });
        }

        auto init() {
          // Create a new channel and connect it to the filtered signal
          auto channel_result = client_->filteredChannels_.filtered_.emplace(name, Signal{});
          auto& channel = channel_result.first->second;

          // Connect the new channel to the corresponding filtered signal in the channels map
          auto result = client_->channels_.emplace(name, filteredSignal(&byName));
          result.first->second.connectSource(channel);

          signalFilter_ = &(result.first->second);

          // Set the onSubscribe callback to update the subscribed status
          onSubscribe([this](PusherClient::Event const& event) {
            this->subscribed = true;
          });

          return channel_result;
        }

        // Bind a callback function to a specific event name in the channel
        template<typename FuncT>
        auto bind(std::string const& event_name, FuncT&& func) {
          return signalFilter_->connect(event_name, std::forward<FuncT>(func));
        }

        // Bind a callback function to all events in the channel
        template<typename FuncT>
        auto bindAll(FuncT&& func) {
          return signalFilter_->connect(std::forward<FuncT>(func));
        }

        // Set a callback function to be called when the channel is successfully subscribed
        template<typename FuncT>
        auto onSubscribe(FuncT&& func) {
          return signalFilter_->connect("pusher_internal:subscription_succeeded", std::forward<FuncT>(func));
        }

        // Set a callback function to be called when a member joins the channel
        template<typename FuncT>
        auto onMemberJoin(FuncT&& func) {
          return signalFilter_->connect("pusher_internal:subscription_count", std::forward<FuncT>(func));
        }

        void subscribe() {
          if (client_->connected)
            subscribe_("");
          else client_->onConnect([this](const PusherClient::Event& event) {
            subscribe_("");
          });
        }

        void subscribe(std::string auth) {
          if (client_->connected)
            subscribe_(auth);
          else client_->onConnect([this, auth = auth](const PusherClient::Event& event) {
            subscribe_(auth);
          });
        }

        void subscribe(AuthCallback authCallback) {
          if (client_->connected) {
            rapidjson::Document authData = authCallback(client_->socketId, name);
            std::string auth = authData["auth"].GetString();
            subscribe_(auth);
          } else {
            client_->onConnect([this, authCallback = authCallback](const PusherClient::Event& event) {
              rapidjson::Document authData = authCallback(client_->socketId, name);
              std::string auth = authData["auth"].GetString();
              subscribe_(auth);
            });
          }
        }

        // Unsubscribe from the channel
        auto unsubscribe() {
          printf("Unsubscribing from channel %s\n", name.c_str());

          // Create the unsubscribe message
          rapidjson::Document data(rapidjson::kObjectType);
          data.AddMember("channel", rapidjson::StringRef(name.c_str()), data.GetAllocator());

          // Send the unsubscribe message through the WebSocket connection
          client_->sendEvent("pusher:unsubscribe", data);

          // Update the subscribed status
          subscribed = false;

          printf("Successfully Unsubscribing");
        }

        // Get the number of event handlers connected to a specific event name in the channel
        template<typename FuncT>
        std::size_t getEventHandlerCount(std::string const& event_name, FuncT&& func) const {
          return signalFilter_->getHandlerCount(event_name, std::forward<FuncT>(func));
        }

        // Disconnect a specific event handler from the channel
        template<typename FuncT>
        bool disconnectEventHandler(std::string const& event_name, FuncT&& func) {
          return signalFilter_->disconnect(event_name, std::forward<FuncT>(func));
        }

      private:
        // Subscribe to the channel
        void subscribe_(std::string auth = "") {
          printf("Subscribing from channel %s\n", name.c_str());

          rapidjson::Document data(rapidjson::kObjectType);
          data.AddMember("channel", rapidjson::StringRef(name.c_str()), data.GetAllocator());

          // if(authCallback_) {
          //   // Generate the authentication data
          //   rapidjson::Document authData = authCallback_(client_->socketId, name);
          //   data.AddMember("auth", rapidjson::StringRef(authData["auth"].GetString()), data.GetAllocator());
          // }
          if (!auth.empty())
            data.AddMember("auth", rapidjson::StringRef(auth.c_str()), data.GetAllocator());

          client_->sendEvent("pusher:subscribe", data);
        }
      };
    }
  }
}

#endif // PUSHERCLIENT_CLIENT_CHANNEL_CHANNEL_PROXY_HPP
