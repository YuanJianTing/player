#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstring>
#include <functional>
#include <memory>
#include <system_error>

// Socket headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

class UdpSocketHelper
{
public:
    using OnConfigReceiveListener = std::function<void(const std::string &)>;

    UdpSocketHelper(OnConfigReceiveListener listener)
        : listener_(std::move(listener)),
          is_quit_read_client_(false),
          last_manager_address_({0})
    {

        // Start threads for different socket operations
        threads_.emplace_back(&UdpSocketHelper::initSocket, this);
        threads_.emplace_back(&UdpSocketHelper::recvUDPBroadcast, this);
        threads_.emplace_back(&UdpSocketHelper::multicastReceiver, this);
    }

    ~UdpSocketHelper()
    {
        close();
    }

    void reply(const std::string &message)
    {
        if (last_manager_address_.sin_addr.s_addr == 0 || socket_fd_ == -1)
        {
            return;
        }

        try
        {
            sendToSocket(socket_fd_, message, last_manager_address_);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Reply error: " << e.what() << std::endl;
        }
    }

    void replySync(const std::string &message)
    {
        std::thread([this, message]()
                    { reply(message); })
            .detach();
    }

    void close()
    {
        is_quit_read_client_ = true;

        // Close sockets to unblock receive calls
        if (socket_fd_ != -1)
        {
            ::shutdown(socket_fd_, SHUT_RDWR);
            ::close(socket_fd_);
            socket_fd_ = -1;
        }

        if (broadcast_socket_fd_ != -1)
        {
            ::shutdown(broadcast_socket_fd_, SHUT_RDWR);
            ::close(broadcast_socket_fd_);
            broadcast_socket_fd_ = -1;
        }

        if (multicast_socket_fd_ != -1)
        {
            ::shutdown(multicast_socket_fd_, SHUT_RDWR);
            ::close(multicast_socket_fd_);
            multicast_socket_fd_ = -1;
        }

        // Join all threads
        for (auto &thread : threads_)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

private:
    // Constants
    static constexpr int BROADCAST_PORT = 9001;
    static constexpr int UDP_PORT = 9002;
    static constexpr int MULTICAST_PORT = 5555;
    static constexpr const char *MULTICAST_GROUP = "224.0.0.10";
    static constexpr int BUFFER_SIZE = 2048;
    static constexpr int DISPLAY_COUNT = 10; // Adjust as needed

    // Socket file descriptors
    int socket_fd_ = -1;
    int broadcast_socket_fd_ = -1;
    int multicast_socket_fd_ = -1;

    // Thread management
    std::vector<std::thread> threads_;
    std::atomic<bool> is_quit_read_client_;
    OnConfigReceiveListener listener_;
    sockaddr_in last_manager_address_;
    std::mutex address_mutex_;

    // Initialize socket for point-to-point communication
    void initSocket()
    {
        try
        {
            socket_fd_ = createSocket(UDP_PORT, false);

            char buffer[BUFFER_SIZE];
            sockaddr_in sender_addr;
            socklen_t sender_addr_len = sizeof(sender_addr);

            while (!is_quit_read_client_)
            {
                std::cout << "UDP service is listening..." << std::endl;

                ssize_t bytes_received = recvfrom(socket_fd_, buffer, BUFFER_SIZE, 0,
                                                  (sockaddr *)&sender_addr, &sender_addr_len);

                if (bytes_received <= 0)
                {
                    if (is_quit_read_client_)
                        break;
                    continue;
                }

                std::string info = decrypt(buffer, bytes_received);
                bool result = handleReceive(info);

                if (result)
                {
                    std::lock_guard<std::mutex> lock(address_mutex_);
                    last_manager_address_ = sender_addr;

                    std::string device_id = getDeviceId(); // Implement this based on SystemProperties.getID()
                    if (device_id.empty())
                    {
                        device_id = "gong";
                    }

                    sendToSocket(socket_fd_, device_id, sender_addr);
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "initSocket error: " << e.what() << std::endl;
        }
    }

    // Receive broadcast messages
    void recvUDPBroadcast()
    {
        try
        {
            broadcast_socket_fd_ = createSocket(BROADCAST_PORT, true);

            char buffer[BUFFER_SIZE];
            sockaddr_in sender_addr;
            socklen_t sender_addr_len = sizeof(sender_addr);

            while (!is_quit_read_client_)
            {
                ssize_t bytes_received = recvfrom(broadcast_socket_fd_, buffer, BUFFER_SIZE, 0,
                                                  (sockaddr *)&sender_addr, &sender_addr_len);

                if (bytes_received <= 0)
                {
                    if (is_quit_read_client_)
                        break;
                    continue;
                }

                std::string info = decrypt(buffer, bytes_received);

                {
                    std::lock_guard<std::mutex> lock(address_mutex_);
                    last_manager_address_ = sender_addr;

                    std::string device_id = getDeviceId();
                    if (device_id.empty())
                    {
                        device_id = "gong";
                    }

                    sendToSocket(broadcast_socket_fd_, device_id, sender_addr);
                }

                handleReceive(info);

                // Sleep for 10 seconds
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "recvUDPBroadcast error: " << e.what() << std::endl;
        }
    }

    // Receive multicast messages
    void multicastReceiver()
    {
        try
        {
            multicast_socket_fd_ = createMulticastSocket();

            char buffer[BUFFER_SIZE];
            sockaddr_in sender_addr;
            socklen_t sender_addr_len = sizeof(sender_addr);

            while (!is_quit_read_client_)
            {
                ssize_t bytes_received = recvfrom(multicast_socket_fd_, buffer, BUFFER_SIZE, 0,
                                                  (sockaddr *)&sender_addr, &sender_addr_len);

                if (bytes_received <= 0)
                {
                    if (is_quit_read_client_)
                        break;
                    continue;
                }

                std::string info = decrypt(buffer, bytes_received);
                handleReceive(info);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "multicastReceiver error: " << e.what() << std::endl;
        }
    }

    // Helper to create a socket
    int createSocket(int port, bool broadcast)
    {
        int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (fd == -1)
        {
            throw std::system_error(errno, std::system_category(), "socket creation failed");
        }

        // Set socket options
        int optval = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        {
            ::close(fd);
            throw std::system_error(errno, std::system_category(), "setsockopt SO_REUSEADDR failed");
        }

        if (broadcast)
        {
            if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1)
            {
                ::close(fd);
                throw std::system_error(errno, std::system_category(), "setsockopt SO_BROADCAST failed");
            }
        }

        // Set non-blocking
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1)
        {
            ::close(fd);
            throw std::system_error(errno, std::system_category(), "fcntl F_GETFL failed");
        }
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            ::close(fd);
            throw std::system_error(errno, std::system_category(), "fcntl F_SETFL failed");
        }

        // Bind
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (bind(fd, (sockaddr *)&addr, sizeof(addr)) == -1)
        {
            ::close(fd);
            throw std::system_error(errno, std::system_category(), "bind failed");
        }

        return fd;
    }

    // Helper to create multicast socket
    int createMulticastSocket()
    {
        int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (fd == -1)
        {
            throw std::system_error(errno, std::system_category(), "multicast socket creation failed");
        }

        // Set socket options
        int optval = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        {
            ::close(fd);
            throw std::system_error(errno, std::system_category(), "setsockopt SO_REUSEADDR failed");
        }

        // Bind
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(MULTICAST_PORT);

        if (bind(fd, (sockaddr *)&addr, sizeof(addr)) == -1)
        {
            ::close(fd);
            throw std::system_error(errno, std::system_category(), "multicast bind failed");
        }

        // Join multicast group
        ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);

        if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
        {
            ::close(fd);
            throw std::system_error(errno, std::system_category(), "setsockopt IP_ADD_MEMBERSHIP failed");
        }

        // Set non-blocking
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1)
        {
            ::close(fd);
            throw std::system_error(errno, std::system_category(), "fcntl F_GETFL failed");
        }
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            ::close(fd);
            throw std::system_error(errno, std::system_category(), "fcntl F_SETFL failed");
        }

        return fd;
    }

    // Helper to send data through a socket
    void sendToSocket(int fd, const std::string &message, const sockaddr_in &dest_addr)
    {
        ssize_t sent = sendto(fd, message.data(), message.size(), 0,
                              (const sockaddr *)&dest_addr, sizeof(dest_addr));
        if (sent == -1)
        {
            throw std::system_error(errno, std::system_category(), "sendto failed");
        }
    }

    // Handle received message
    bool handleReceive(const std::string &text)
    {
        if (text.empty())
            return false;

        try
        {
            size_t at_pos = text.find('@');
            if (at_pos != std::string::npos)
            {
                size_t pipe_pos = text.find('|');
                if (pipe_pos != std::string::npos)
                {
                    std::string config_device = text.substr(pipe_pos + 1, at_pos - pipe_pos - 1);

                    if (!config_device.empty())
                    {
                        bool exists = false;
                        for (int i = 0; i < DISPLAY_COUNT; i++)
                        {
                            std::string id = getDeviceId(i); // Implement based on SystemProperties.getID(i)
                            if (id == config_device)
                            {
                                exists = true;
                                break;
                            }
                        }

                        if (!exists)
                            return false;
                    }
                }
            }

            if (listener_)
            {
                listener_(text);
            }
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    // Decrypt received data
    std::string decrypt(const char *buffer, size_t length)
    {
        if (length > 0 && static_cast<uint8_t>(buffer[0]) == 0xFC)
        {
            // Implement your decryption logic here
            // For now, just return the string without the first byte
            return std::string(buffer + 1, length - 1);
        }
        return ""; // Don't accept unencrypted data
    }

    // Placeholder for device ID - implement based on your SystemProperties
    std::string getDeviceId(int index = 0)
    {
        // Implement this based on your SystemProperties.getID() functionality
        return "00000000000";
    }
};