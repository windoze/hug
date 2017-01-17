#include "common.h"
#include "connection.h"

connection::connection(asio::io_service &ios,
                       const std::string &server,
                       std::function<uint16_t(void *, std::size_t)> data_sink)
        : ios_(ios), server_mode(false), data_sink_(data_sink) {
}

connection::connection(asio::io_service &ios,
                       std::function<uint16_t(void *, std::size_t)> data_sink)
        : ios_(ios), server_mode(true), data_sink_(data_sink) {

}

connection::~connection() {
    //
}

void connection::send_packet(const void *data, std::size_t sz, uint16_t cid) {
//    // Limit post queue size
//    lock lck(post_counter_mutex);
//    while (post_counter >= outgoing_queue_size) {
//        post_counter_cv.wait(lck);
//    }
//    post_counter++;
//
    // Post sending task into io_service, actually work will be done in io_service thread(s)
    ios_.post([data, sz, cid, this, _ = shared_from_this()]() {
        do_send(asio::const_buffers_1(data, sz), cid);
    });
}

void connection::do_send(const asio::const_buffers_1 &buf, uint16_t cid) {
    endpoint_t peer;
    {
        // Randomly pick one peer endpoint
        guard g(peer_endpoints_lock);
        std::vector<endpoint_t> &eps = server_mode ? peer_endpoints[cid] : peer_endpoints.begin()->second;
        if(eps.empty()) {
            // TODO: Error
            return;
        }
        peer = eps[std::uniform_int_distribution<std::size_t>(0, eps.size() - 1)(rng())];
    }
    socket_ptr s;
    {
        // Randomly pick one socket
        guard g(receiving_sockets_lock);
        s = receiving_sockets[std::uniform_int_distribution<std::size_t>(0, receiving_sockets.size() - 1)(rng())];
    }
    s->async_send_to(buf, peer, [this, _ = shared_from_this()](auto &&ec, std::size_t bytes_transferred) {
        if (ec) {
            // TODO: Error
        }
        // Reduce post counter when data sent
//        guard g(post_counter_mutex);
//        post_counter--;
//        post_counter_cv.notify_one();
    });
}

void connection::do_receive(socket_ptr s, std::size_t bytes_transferred) {
    if (s && s->buffer && (bytes_transferred > 0)) {
        // Call data_sink_ to validate and process incoming packet
        // data_sink_ should deallocate the buffer
        uint16_t cid = data_sink_(s->buffer, bytes_transferred);
        if (cid) {
            // If incoming packet is valid, add peer endpoint into list
            guard g(peer_endpoints_lock);
            peer_endpoints[cid].push_back(s->endpoint);
        }
    }

    s->buffer = nullptr;
    // Start reading on the socket again
    s->async_receive_from(s->endpoint,
                          [s, this, _ = shared_from_this()](const asio::error_code &ec, std::size_t bt) {
                              if (ec) {
                                  //TODO: Log
                                  return;
                              }
                              do_receive(s, bt);
                          });
}
