#pragma once

#include "common.h"
#include "queue.h"

class connection : public std::enable_shared_from_this<connection>
{
public:
	typedef std::shared_ptr<connection> ptr;

	connection(asio::io_service& ios,
		std::function<std::size_t(const asio::const_buffers_1&, const asio::mutable_buffers_1&)> before_sending,
		std::function<asio::mutable_buffers_1(std::size_t)> get_receive_buffer,
		std::function<bool(const asio::mutable_buffers_1&)> data_sink);

	~connection();

	void send_packet(const void *data, std::size_t sz);

protected:

	struct receiving_socket : public std::enable_shared_from_this<receiving_socket>
	{
		template <typename ...Args>
		receiving_socket(asio::io_service& ios, Args&&...args)
			: socket(ios, asio::ip::udp::endpoint(std::forward<Args>(args)...))
			, buffer(buf_size)
		{
		}

		operator asio::ip::udp::socket&()
		{
			return socket;
		}

		operator asio::const_buffers_1() const
		{
			return asio::const_buffers_1(buffer.data(), buffer.size());
		}

		template <typename ...Args>
		void async_receive_from(Args&& ... args)
		{
			socket.async_receive_from(asio::mutable_buffers_1(buffer.data(), buffer.size()), std::forward<Args>(args)...);
		}

		template <typename ...Args>
		void async_send_to(Args&& ... args)
		{
			socket.async_send_to(std::forward<Args>(args)...);
		}

		endpoint_t endpoint;
		socket_t socket;
		std::vector<uint8_t> buffer;
	};

	typedef std::shared_ptr<receiving_socket> socket_ptr;

	template <typename ...Args>
	void start_receiving(Args&&...args)
	{
		std::lock_guard<std::mutex> lck(receiving_sockets_lock);
		receiving_sockets.emplace_back(std::make_shared<receiving_socket>(std::forward<Args>(args)...));
		do_receive(*receiving_sockets.rbegin());
	}

	void do_receive(socket_ptr s, std::size_t bytes_transferred=0);

	void do_send(const asio::const_buffers_1& buf);

private:
	asio::io_service& ios_;
	std::function<std::size_t(const asio::const_buffers_1&, const asio::mutable_buffers_1&)> before_sending_;
	std::function<asio::mutable_buffers_1(std::size_t)> get_receive_buffer_;
	std::function<bool(const asio::mutable_buffers_1&)> data_sink_;

	std::mutex peer_endpoints_lock;
	std::vector<endpoint_t> peer_endpoints;

	std::mutex receiving_sockets_lock;
	std::vector<socket_ptr> receiving_sockets;

	std::size_t post_counter = 0;
	std::mutex post_counter_mutex;
	std::condition_variable post_counter_cv;
};
