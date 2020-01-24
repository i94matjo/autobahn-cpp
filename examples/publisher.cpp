///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Crossbar.io Technologies GmbH and contributors
//
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#include "parameters.hpp"

#include <autobahn/autobahn.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>

int main(int argc, char** argv)
{
    std::cerr << "Boost: " << BOOST_VERSION << std::endl;

    try {
        auto parameters = get_parameters(argc, argv);

        boost::asio::io_service io;
        bool debug = parameters->debug();

        auto transport = std::make_shared<autobahn::wamp_tcp_transport>(
                io, parameters->rawsocket_endpoint(), debug);

        // create a WAMP session that talks WAMP-RawSocket over TCP
        //
        auto session = std::make_shared<autobahn::wamp_session>(io, debug);

        transport->attach(std::static_pointer_cast<autobahn::wamp_transport_handler>(session));

        // Use callback asynchronous mechanism
        auto on_exception = [&](boost::exception_ptr eptr) {
            try {
                boost::rethrow_exception(eptr);
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                io.stop();
            }
        };

        transport->connect([&]() {
            std::cerr << "transport connected" << std::endl;

            session->start([&]() {
                std::cerr << "session started" << std::endl;

                session->join([&](const uint64_t& joined) {
                    std::cerr << "joined realm: " << joined << std::endl;

                    std::tuple<std::string> arguments(std::string("hello"));
                    session->publish("com.examples.subscriptions.topic1", arguments);
                    std::cerr << "event published" << std::endl;

                    session->leave([&](const std::string& reason) {
                        std::cerr << "left session (" << reason << ")" << std::endl;

                        session->stop([&]() {
                            std::cerr << "stopped session" << std::endl;
                            io.stop();
                        },
                        on_exception);
                    },
                    on_exception);
                },
                on_exception,
                parameters->realm());
            },
            on_exception);
        },
        on_exception);

        std::cerr << "starting io service" << std::endl;
        io.run();
        std::cerr << "stopped io service" << std::endl;

        transport->detach();
    }
    catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
