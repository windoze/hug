#pragma once
#ifndef CONNECTION_H_INCLUDED
#define CONNECTION_H_INCLUDED

#include "common.h"
#include "queue.h"

class connection : public std::enable_shared_from_this<connection> {
public:
    typedef std::shared_ptr<connection> ptr;

    /**
     * Constructor of connection in client mode
     * @param ios asio::io_service instance
     * @param server connection point of server
     * @param data_sink callback on receiving packet, data_sink owns the data block and *must* release it with `deallocate_buffer`
     */
    connection(asio::io_service &ios, const std::string &server, std::function<uint16_t(void *, std::size_t)> data_sink);

    /**
     * Constructor of connection in server mode
     * @param ios asio::io_service instance
     * @param data_sink callback on receiving packet, data_sink owns the data block and *must* release it with `deallocate_buffer`
     */
    connection(asio::io_service &ios,
               std::function<uint16_t(void *, std::size_t)> data_sink);

    ~connection();

    uint16_t connection_id() const { return connection_id_; }

    void connection_id(uint16_t cid) {
        connection_id_ = cid;
    }

    /**
     * This function is running in caller thread, and can be blocked when posting queue exceeds outgoing_queue_size
     * @param data pointer to data block, data *must* be allocated by `allocate_buffer`
     * @param sz size of data block
     */
    void send_packet(const void *data, std::size_t sz, uint16_t cid=0);

protected:

    struct receiving_socket : public std::enable_shared_from_this<receiving_socket> {
        template<typename ...Args>
        receiving_socket(asio::io_service &ios, Args &&...args)
                : socket(ios, asio::ip::udp::endpoint(std::forward<Args>(args)...)) {
        }

        operator asio::ip::udp::socket &() {
            return socket;
        }

        template<typename ...Args>
        void async_receive_from(Args &&... args) {
            buffer = allocate_buffer();
            socket.async_receive_from(asio::mutable_buffers_1(buffer, buf_size),
                                      std::forward<Args>(args)...);
        }

        template<typename ...Args>
        void async_send_to(Args &&... args) {
            socket.async_send_to(std::forward<Args>(args)...);
        }

        endpoint_t endpoint;
        socket_t socket;
        void *buffer = nullptr;
    };

    typedef std::shared_ptr<receiving_socket> socket_ptr;

    template<typename ...Args>
    void start_receiving(Args &&...args) {
        std::lock_guard<std::mutex> lck(receiving_sockets_lock);
        receiving_sockets.emplace_back(std::make_shared<receiving_socket>(std::forward<Args>(args)...));
        do_receive(*receiving_sockets.rbegin());
    }

    void do_receive(socket_ptr s, std::size_t bytes_transferred = 0);

    void do_send(const asio::const_buffers_1 &buf, uint16_t cid);

private:
    asio::io_service &ios_;
    std::function<uint16_t(void *, std::size_t)> data_sink_;

    bool server_mode;
    uint16_t connection_id_ = 0;

    std::mutex peer_endpoints_lock;
    std::map<uint16_t, std::vector<endpoint_t>> peer_endpoints;

    std::mutex receiving_sockets_lock;
    std::vector<socket_ptr> receiving_sockets;

//    std::size_t post_counter = 0;
//    std::mutex post_counter_mutex;
//    std::condition_variable post_counter_cv;
};

#endif	// !defined(CONNECTION_H_INCLUDED)