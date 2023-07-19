//          Copyright Joe Coder 2004 - 2006.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef PUSHERCLIENT_CLIENT_HPP
#define PUSHERCLIENT_CLIENT_HPP

#include <iostream>
#include <string>
#include <map>

#include <boost/asio/async_result.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket.hpp>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "event.hpp"
#include "client/read.hpp"
#include "client/channel/channel_proxy.hpp"
#include "client/channel/signal_filter.hpp"

namespace PusherClient {

  template<typename SocketT>
  class Client {
    using SignalFilter = client::channel::SignalFilter<std::string(*)(Event const&)>;
    using AuthCallback = std::function<rapidjson::Document(const std::string&, const std::string&)>;

    boost::asio::ip::tcp::resolver resolver_;
    std::string host_;
    std::string handshakeResource_;
    boost::beast::flat_buffer read_buf_;
    client::channel::Signal events_;
    SignalFilter filteredEvents_;

  public:
    boost::beast::websocket::stream<SocketT> socket_;
    SignalFilter filteredChannels_;
    std::map<std::string, SignalFilter> channels_;

    bool connected = false;
    std::string socketId;

    // Constructor
    Client(boost::asio::io_service& ios, std::string key, std::string cluster = "mt1")
      : socket_{ios}
      , resolver_{ios}
      , host_{"ws-" + std::move(cluster) + ".pusher.com"}
      , handshakeResource_{"/app/" + std::move(key) + "?client=PusherClient&version=0.01&protocol=7"}
      , events_{}
      , filteredChannels_{client::channel::filteredSignal(&client::channel::byChannel)}
      , filteredEvents_{client::channel::filteredSignal(&client::channel::byName)} {}

    // Initialize the client
    void initialise() {
      filteredChannels_.connectSource(events_);
      filteredEvents_.connectSource(events_);
    }

    // Asynchronously connect to the Pusher server
    template<typename TokenT>
    auto asyncConnect(TokenT&& token) {
      initialise();

      typename boost::asio::handler_type<TokenT, void(boost::system::error_code)>::type handler(std::forward<TokenT>(token));
      boost::asio::async_result<decltype(handler)> result(handler);
      using query = boost::asio::ip::tcp::resolver::query;
      resolver_.async_resolve(query{host_, "http"}, [this, handler](auto ec, auto endpoint) mutable {
        if(ec)
          return handler(ec);

        boost::asio::async_connect(socket_.next_layer(), endpoint, [this, handler](auto ec, auto) mutable {
          if(ec)
            return handler(ec);

          socket_.async_handshake(host_, handshakeResource_, [this, handler](auto ec) mutable {
            if(ec)
              return handler(ec);

            this->readImpl();
            return handler(ec);
          });
        });
      });

      onInitialised();

      return result.get();
    }

    // Synchronously connect to the Pusher server
    auto connect() {
      initialise();
      boost::asio::connect(socket_.next_layer(), resolver_.resolve(boost::asio::ip::tcp::resolver::query{host_, "80"}));
      socket_.handshake(host_, handshakeResource_);

      readImpl();

      onInitialised();
    }

    // Disconnect from the Pusher server
    void disconnect() {
      resolver_.cancel();
      socket_.close(boost::beast::websocket::close_code::normal);
    }

    // Create a new channel with the given name and authentication callback
    auto channel(std::string const& name, AuthCallback authCallback) {
      return client::channel::ChannelProxy<SocketT>(this, name, authCallback);
    }

    // Bind a callback function to all events
    template<typename FuncT>
    auto bindAll(FuncT&& func) {
      return filteredEvents_.connect(std::forward<FuncT>(func));
    }

    // Bind a callback function to a specific event name
    template<typename FuncT>
    auto bind(std::string const& event_name, FuncT&& func) {
      return filteredEvents_.connect(event_name, std::forward<FuncT>(func));
    }

    // Set a callback function to be called when the client is successfully connected
    template<typename FuncT>
    auto onConnect(FuncT&& func) {
      return filteredEvents_.connect("pusher:connection_established", std::forward<FuncT>(func));
    }

    // Set a callback function to be called when the client is disconnected
    template<typename FuncT>
    auto onDisconnect(FuncT&& func) {
      return filteredEvents_.connect("pusher:disconnected", std::forward<FuncT>(func));
    }

    // Set a callback function to be called when an error occurs
    template<typename FuncT>
    auto onError(FuncT&& func) {
      return filteredEvents_.connect("pusher:error", std::forward<FuncT>(func));
    }

    // Send an event to a specific channel
    void sendEvent(const std::string& eventName, const rapidjson::Value& eventData) {
      // Create the event data string
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

      eventData.Accept(writer);
      std::string data = buffer.GetString();

      // Create the event message
      rapidjson::StringBuffer msgBuffer;
      rapidjson::Writer<rapidjson::StringBuffer> msgWriter(msgBuffer);
      msgWriter.StartObject();
      msgWriter.String("event");
      msgWriter.String(eventName.c_str());
      msgWriter.String("data");
      msgWriter.RawValue(data.c_str(), data.size(), rapidjson::kObjectType);
      msgWriter.EndObject();
      
      // Send the event message through the WebSocket connection
      socket_.write(boost::asio::buffer(msgBuffer.GetString(), msgBuffer.GetSize()));
    }

    // Get all channels
    // auto getChannels() const {
    //   std::vector<ChannelProxy<SocketT>> channels;
    //   for (const auto& pair : channels_) {
    //     channels.push_back(pair.second);
    //   }
    //   return channels;
    // }

    // Unsubscribe from all channels
    // void unsubscribeAll() {
    //   auto channels = getChannels();
    //   for (const auto& channel : channels) {
    //     channel.unsubscribe();
    //   }
    // }

  private:
    // Read data from the WebSocket connection
    void readImpl() {
      return socket_.async_read(read_buf_, [this](auto ec, std::size_t bytes_written) {
        if(ec)
          return;

        events_(client::makeEvent(read_buf_));
        read_buf_.consume(read_buf_.size());

        this->readImpl();
      });
    }

    // Perform necessary actions after the client is initialized
    void onInitialised() {
      printf("pusher initialised successfully\n");

      onConnect([this](const Event& event) {
        printf("pusher connect successfully\n");
        rapidjson::Document data;
        data.Parse(event.data.c_str());

        if (!data.HasParseError() && data.HasMember("socket_id"))
          socketId = data["socket_id"].GetString();

        connected = true;
      });

      onDisconnect([this](const Event& event) {
        socketId = "";
        connected = false;
      });
    }
  };
}

#endif // PUSHERCLIENT_CLIENT_HPP