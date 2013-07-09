// Standard includes
#include <iostream>
#include <string>

// 3rd party includes
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

struct result_t {
    boost::system::error_code error;
    std::size_t               bytes_transferred;
};

void
callback(const boost::system::error_code& error,
         std::size_t                      bytes_transferred,
         boost::promise<result_t>*        promise)
{
    result_t result;
    result.error = error;
    result.bytes_transferred = bytes_transferred;
    promise->set_value(result);
}

enum { max_length = 1024 };

int main(
    int,
    char* argv[])
{
    try {
        std::cout << "connecting to " << argv[1] << ":" << argv[2] << std::endl;

        boost::asio::io_service io;
        std::auto_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(io));

        boost::asio::ip::tcp::resolver resolver(io);
        boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), argv[1], argv[2]);
        boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

        boost::asio::ip::tcp::socket socket(io);
        boost::asio::connect(socket, iterator);

        std::cout << "Enter message: ";
        char request[max_length];
        std::cin.getline(request, max_length);
        size_t request_length = std::strlen(request);

        boost::thread thread(boost::bind(&boost::asio::io_service::run, &io));
        thread.detach(); // wait is going to be performed by the futures

        std::cout << "I'm detached!!" << std::endl;

        boost::promise<result_t> write_promise;
        boost::asio::async_write(socket,
                                 boost::asio::buffer(request, request_length),
                                 boost::bind(callback,
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred,
                                             &write_promise));


        std::cout << write_promise.get_future().get().error << std::endl;


        boost::promise<result_t> read_promise;
        boost::asio::streambuf reply;
        boost::asio::async_read_until(socket,
                                      reply,
                                      '\n',
                                      boost::bind(callback,
                                                  boost::asio::placeholders::error,
                                                  boost::asio::placeholders::bytes_transferred,
                                                  &read_promise));

        std::cout << read_promise.get_future().get().error << std::endl;
        std::cout << "Reply is: " << boost::asio::buffer_cast<const char*>(reply.data()) << std::endl;
        work.reset();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }


    return 0;
}
