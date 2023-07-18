#include <iomanip>
#include <iostream>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <PusherClient/client.hpp>
#include <PusherClient/event.hpp>

// Replace with your Pusher credentials
const std::string key = "037c47e0cbdc81fb7144";
const std::string cluster = "mt1";

// Replace with your channel and event names
const std::string channel_name = "public-messages";
const std::string event_name = "MessageCreatedEvent";

// Replace with your authentication endpoint and token
const std::string authEndpoint = "http://localhost/broadcasting/auth";
const std::string authToken = "34|yzWaxwGZz75Xqk4tXviP4uhAc0sVB14OLVXEmoxg";

namespace {

  // Custom logger function to print Pusher events
  std::ostream& operator<<(std::ostream& os, PusherClient::Event ev) {
    // Get the current local time
    auto now = std::chrono::system_clock::now();

    // Convert the system time to a time_t object
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    // Convert the time_t object to a local time
    std::tm* localTime = std::localtime(&currentTime);

    // Extract individual components of the local time
    int year = localTime->tm_year + 1900;     // Years since 1900
    int month = localTime->tm_mon + 1;        // Months since January (0-based)
    int day = localTime->tm_mday;             // Day of the month
    int hour = localTime->tm_hour;            // Hour of the day (24-hour clock)
    int minute = localTime->tm_min;           // Minute past the hour
    int second = localTime->tm_sec;           // Second past the minute

    // Format and print the event
    return os
      << "[" << std::setw(4) << std::setfill('0') << year << "-"
      << std::setw(2) << std::setfill('0') << month << "-"
      << std::setw(2) << std::setfill('0') << day << " "
      << std::setw(2) << std::setfill('0') << hour << ":"
      << std::setw(2) << std::setfill('0') << minute << ":"
      << std::setw(2) << std::setfill('0') << second << "]: "
      << "{\"channel\":\"" << ev.channel
      << "\",\"event\":\"" << ev.name
      << "\",\"data\":\"" << ev.data << "\"}\n";
  }

  // logger
  auto logger = [](PusherClient::Event const& ev) {
    std::cout << ev;
  };
}

//---- Custom exception classes for Curl errors ---//
class CurlException : public std::exception {
public:
  CurlException(const std::string error, const int statusCode) : error_(error), statusCode_(statusCode) {}

  const char* what() const noexcept override {
    printf("CurlError[%d]: %s\n", statusCode_, error_.c_str());
    return error_.c_str();
  }

private:
  const std::string error_;
  const int statusCode_;
};

class CurlInitException : public CurlException {

public:
  CurlInitException() : CurlException("Cannot initializing curl", -1) {}
};

class CurlRequestException : public CurlException {

public:
  CurlRequestException(const std::string error, int statusCode) : CurlException(error, statusCode) {}
};

class CurlBadRequestException : public CurlException {

public:
  CurlBadRequestException(const std::string error) : CurlException(error, STATUS_CODE) {}

  static const int STATUS_CODE = 400;
};

class CurlUnauthorizedException : public CurlException {

public:
  CurlUnauthorizedException(const std::string error) : CurlException(error, STATUS_CODE) {}

  static const int STATUS_CODE = 401;
};

class CurlForbiddenException : public CurlException {

public:
  CurlForbiddenException(const std::string error) : CurlException(error, STATUS_CODE) {}

  static const int STATUS_CODE = 403;
};

class CurlNotFoundException : public CurlException {

public:
  CurlNotFoundException(const std::string error) : CurlException(error, STATUS_CODE) {}

  static const int STATUS_CODE = 404;
};

class CurlServerErrorException : public CurlException {

public:
  CurlServerErrorException(const std::string error) : CurlException(error, STATUS_CODE) {}

  static const int STATUS_CODE = 500;
};
//------------------------------------------------//

// Custom authentication callback
/// body writer callback
size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

/// auth authentication function (works with laravel server)
rapidjson::Document MyAuthCallback(const std::string& socketId, const std::string& channel) {
  // auth received data
  rapidjson::Document authData;

  // Perform the authentication request using Curl
  CURL* curl = curl_easy_init();
  std::string body;
  std::string form = "{\"socket_id\": \"" + std::move(socketId) + "\", \"channel_name\": \"" + std::move(channel) + "\"}";

  if (curl) { // check if curl initialized successfully
    // Set Curl options and perform the request
    curl_easy_setopt(curl, CURLOPT_URL, authEndpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, form.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.38.0");
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

    struct curl_slist* headers = NULL;

    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + std::move(authToken)).c_str());
    
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    // response status code
    long statusCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);

    // Handle Curl response and status code
    if (res != CURLE_OK) {
      throw CurlRequestException(curl_easy_strerror(res), res);
    }

    if (statusCode != 200) switch (statusCode) {
      case CurlBadRequestException::STATUS_CODE:
        throw CurlBadRequestException(body);
      case CurlUnauthorizedException::STATUS_CODE:
        throw CurlUnauthorizedException(body);
      case CurlForbiddenException::STATUS_CODE:
        throw CurlForbiddenException(body);
      case CurlNotFoundException::STATUS_CODE:
        throw CurlNotFoundException(body);
      case CurlServerErrorException::STATUS_CODE:
        throw CurlServerErrorException(body);
      default:
        throw CurlRequestException(body, statusCode);
        break;
      }

    // cleanup curl
    curl_easy_cleanup(curl);

    // free headers
    curl_slist_free_all(headers);
  } else 
    throw CurlInitException(); // curl uninitialized successfully

  // global cleanup curl
  curl_global_cleanup();

  // parse response json to authData
  authData.Parse(body.c_str());

  return authData;
}

// Detect errors
static void m_asio_event_loop(boost::asio::io_service& svc) {
  for (;;) {
    try {
      svc.run();
      break; // exited normally
    }
    catch (std::exception const& e) {
      std::cout << "An unexpected error occurred running websocket task: " << e.what();
    }
    catch (...) {
      std::cout << "An unexpected error occurred running websocket task";
    }
  }
}

int main() {
  boost::asio::io_service ios; // create io service
  // create Client object
  PusherClient::Client<boost::asio::ip::tcp::socket> client{ ios, key, cluster };

  // Connect to the Pusher service
  client.connect();
  
  // Bind the logger function to handle all events
  client.bindAll(logger);

  // Define the authentication callback function
  auto authCallback = MyAuthCallback;
  
  // Create a channel
  auto channel = client.channel(channel_name, authCallback);

  // bind to event
  channel.bind(event_name, [&channel](PusherClient::Event const& event) {
    printf("channel(%s)[event(%s)]: data(%s)\n", channel_name.c_str(), event_name.c_str(), event.data.c_str()); // write a received event data
  });

  // bind an event handler to unsubscribe when receiving a specific event
  channel.bind("unsubscribe", [&channel](PusherClient::Event const& event) {
    if(event.data == "now")
      channel.unsubscribe(); // unsubscribe from channel
  });

  // run a service
  m_asio_event_loop(ios);

  return 0;
}
