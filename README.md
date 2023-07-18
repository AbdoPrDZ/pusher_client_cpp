# PusherClient Library (v^0.0.1)

PusherClient is a C++ library for interacting with the Pusher real-time messaging service. It provides a simple and convenient way to connect to Pusher, subscribe to channels, and handle events.

## Features

- Connect to the Pusher service using WebSocket.
- Subscribe to channels and receive events.
- Bind event handlers to specific event names or all events in a channel.
- Authenticate channels with a custom authentication callback.

## Requirements

- C++17 compatible compiler
- [vcpkg](https://vcpkg.io), or [Conan](https://conan.io). (For install and include packages)
- Boost.Asio
- Boost.Beast
- RapidJSON
- Curl

## Installation

1. Clone the PusherClient repository:

    ```shell
    $ git clone https://github.com/AbdoPrDZ/pusher_client_cpp.git
    ```

2. Install requirements packages using [vcpkg](https://vcpkg.io) or any other tool (like [conan](https://conan.io))

3. You can follow example include way to include the library to your project.

## Usage

1. Include the necessary headers in your code:

    ```CPP
    #include <PuserClient/client.hpp>
    ```

2. Create a PusherClient::Client object and provide the necessary parameters:

    ```CPP
    boost::asio::io_service ios;
    std::string apiKey = "your-api-key";
    std::string cluster = "your-cluster";
    PusherClient::Client<boost::asio::ip::tcp::socket> client{ ios, key, cluster };
    ```

3. Connect to the Pusher service:

    ```CPP
    client.connect();
    ```

4. Create a channel using the channel function and specify an authentication callback:

    ```CPP
    auto authCallback = [](const std::string& socketId, const std::string& channelName) {
        // Perform authentication and return the authentication data
        rapidjson::Document authData(rapidjson::kObjectType);
        authData.AddMember("auth", "your-auth-token", authData.GetAllocator());

        return authData;
    };

    auto& channel = client.channel("your-channel-name", authCallback);
    ```

   Note: This is a simple example, you have to read [pusher user authentication](https://pusher.com/docs/channels/server_api/authenticating-users/). for more details (for laravel you can see the [example](https://github.com/AbdoPrDZ/pusher_client_cpp/blob/main/PusherClient/example/main.cpp)).

5. Bind event handlers to the channel:

    ```CPP
    channel.bind("event-name", [](const PusherClient::Event& event) {
      // Handle the event
      std::cout << "Received event: " << event.name << std::endl;
    });
    ```

6. Start the I/O service to initiate the WebSocket communication:

    ```CPP
    ios.run();
    ```

7. Handle events and perform other operations as needed.

    For more detailed examples and usage instructions, refer to the documentation and examples provided in the library repository.
