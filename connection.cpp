#include "pch.h"

#include "connection.h"

connection::connection(asio::io_service& ios
                       , std::function<std::size_t(const asio::const_buffers_1 & , const asio::mutable_buffers_1 & )> before_sending
                       , std::function<asio::mutable_buffers_1(std::size_t)> get_receive_buffer
                       , std::function<bool(const asio::mutable_buffers_1&)> data_sink)
	: ios_(ios)
	  , before_sending_(before_sending)
	  , get_receive_buffer_(get_receive_buffer)
	  , data_sink_(data_sink)
{
}

connection::~connection()
{
	//
}


/**
* This function is running in caller thread, and can be blocked when posting queue exceeds outgoing_queue_size
*/
void connection::send_packet(const void* data, std::size_t sz)
{
	// TODO: Buffer management
	std::shared_ptr<uint8_t> buf{ new uint8_t[buf_size], std::default_delete<uint8_t[]>() };
	std::size_t ssz = before_sending_(asio::const_buffers_1(data, sz), asio::mutable_buffers_1(buf.get(), buf_size));

	// Limit post queue size
	lock lck(post_counter_mutex);
	while (post_counter >= outgoing_queue_size)
	{
		post_counter_cv.wait(lck);
	}
	post_counter++;

	// Post sending task into io_service, actually work will be done in io_service thread(s)
	ios_.post([buf, ssz, this, _ = shared_from_this()]()
	{
		do_send(asio::const_buffers_1(buf.get(), ssz));
	});
}

void connection::do_send(const asio::const_buffers_1& buf)
{
	endpoint_t peer;
	{
		// Randomly pick one peer endpoint
		guard g(peer_endpoints_lock);
		peer = peer_endpoints[std::uniform_int_distribution<std::size_t>(0, peer_endpoints.size() - 1)(rng())];
	}
	socket_ptr s;
	{
		// Randomly pick one socket
		guard g(receiving_sockets_lock);
		s = receiving_sockets[std::uniform_int_distribution<std::size_t>(0, receiving_sockets.size() - 1)(rng())];
	}
	s->async_send_to(buf, peer, [this, _ = shared_from_this()](auto&& ec, std::size_t bytes_transferred)
	{
		if (ec)
		{
			// TODO: Error
		}
		// Reduce post counter when data sent
		guard g(post_counter_mutex);
		post_counter--;
		post_counter_cv.notify_one();
	});
}

void connection::do_receive(socket_ptr s, std::size_t bytes_transferred)
{
	if(bytes_transferred>0)
	{
		// Call get_receive_buffer_ to allocate buffer to receive data
		// Call data_sink_ to process incoming packet 
		if (data_sink_(get_receive_buffer_(bytes_transferred)))
		{
			// If incoming packet is valid, add peer endpoint into list
			guard g(peer_endpoints_lock);
			peer_endpoints.push_back(s->endpoint);
		}
	}

	// Start reading on the socket again
	s->async_receive_from(s->endpoint, [s, this, _ = shared_from_this()](const asio::error_code& ec, std::size_t bytes_transferred)
	{
		if (ec)
		{
			//TODO: Log
			return;
		}
		do_receive(s, bytes_transferred);
	});
}
