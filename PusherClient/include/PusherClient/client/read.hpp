//          Copyright Joe Coder 2004 - 2006.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef PUSHERCLIENT_CLIENT_READ_HPP
#define PUSHERCLIENT_CLIENT_READ_HPP

#include <string>

#include <boost/asio/buffers_iterator.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/core/string.hpp>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <PusherClient/event.hpp>

namespace PusherClient {
  namespace client {

    // Function to convert a JSON value to a string
    template<typename ValueT>
    std::string stringify(ValueT const& value) {
      if (value.IsString())
        return value.GetString();

      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      if (!value.Accept(writer))
        return std::string();

      auto begin = buffer.GetString();
      auto size = buffer.GetSize();

      return std::string(begin, size);
    }

    // Function to create a PusherClient::Event from a boost::beast::flat_buffer
    inline PusherClient::Event makeEvent(boost::beast::flat_buffer const& buf) {
      rapidjson::Document d;

      // Parse the JSON from the buffer
      d.Parse(&*boost::asio::buffers_begin(buf.data()), buf.size());

      PusherClient::Event ev{};
      if (d.HasMember("channel"))
        ev.channel = d["channel"].GetString();
      if (d.HasMember("event"))
        ev.name = d["event"].GetString();
      if (d.HasMember("data"))
        ev.data = stringify(d["data"]);
      ev.timestamp = PusherClient::clock::now();

      return ev;
    }

  }
}

#endif // PUSHERCLIENT_CLIENT_READ_HPP
