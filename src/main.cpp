#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

void session(tcp::socket sock)
{
    try
    {
        for (;;)
        {
            char data[1024];

            boost::system::error_code error;
            size_t length = sock.read_some(boost::asio::buffer(data), error);
            if (error == boost::asio::error::eof)
                break; // Connection closed cleanly by peer
            else if (error)
                throw boost::system::system_error(error); // Some other error

            std::cout << "Received: " << std::string(data, length) << std::endl;

            boost::asio::write(sock, boost::asio::buffer(data, length));
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

int main()
{
    try
    {
        boost::asio::io_context io_context;

        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 54321));

        std::cout << "Echo server listening on port 54321" << std::endl;

        for (;;)
        {
            std::cout << "Waiting for connection..." << std::endl;
            tcp::socket sock(io_context);
            acceptor.accept(sock);
            std::cout << "New connection accepted" << std::endl;
            std::thread(session, std::move(sock)).detach();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}