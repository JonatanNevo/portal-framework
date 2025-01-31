//
// Created by Jonatan Nevo on 31/01/2025.
//

#include "portal/networking/connection.h"
#include "portal/core/log.h"

int main()
{
    using namespace std::chrono_literals;
    portal::Log::init();

    auto on_data = [](const portal::Buffer& buff)
    {
        LOG_INFO("Data: {}", buff[0]);
    };

    portal::network::Connection connection;
    connection.register_on_data_received_callback(on_data);
    connection.connect("127.0.0.1:1337");

    connection.send_string("d");
    while (connection.is_running())
    {
        std::this_thread::sleep_for(1s);
    }

    return 0;
}