#include "common.h"
#include "connection.h"

int main()
{
	asio::io_service io;
	auto s = asio::ip::udp::socket(io, asio::ip::udp::endpoint());
	std::size_t sent = s.send_to(asio::const_buffers_1("0123456789", 10),
	                             asio::ip::udp::endpoint(asio::ip::address_v4::from_string("192.168.0.1"), 53));
	std::cout << "sent:" << sent << std::endl;

	//connection::ptr conn_ptr = std::make_shared<connection>(io, [](auto ...){});

	timer_t t(io, std::chrono::seconds(1));
	t.async_wait([](auto&& ec)
		{
			std::cout << "Timer expired\n";
		});
	io.run();
	return 0;
}
