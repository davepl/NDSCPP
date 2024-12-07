// Main.cpp
//
// This file is the main entry point for the NDSCPP LED Matrix Server application.
// It creates a Canvas, adds a GreenFillEffect to it, and then enters a loop where it
// renders the effect to the canvas, compresses the data, and sends it to the LED
// matrix via a SocketChannel.  The program will continue to run until it receives
// a SIGINT signal (Ctrl-C).

// Main.cpp
//
// This file is the main entry point for the NDSCPP LED Matrix Server application.
// It creates a number of Canvases and adds an effect to each, after which they enter
// a loop where it renders the effect to the canvas, compresses the data, and sends
// it to the relevant LED controller via a SocketChannel.  The program will continue
// to run until it receives a SIGINT signal (Ctrl-C).

#include <csignal>
#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include <getopt.h>
#include "crow_all.h"
#include "global.h"
#include "canvas.h"
#include "interfaces.h"
#include "socketchannel.h"
#include "ledfeature.h"
#include "webserver.h"
#include "controller.h"


using namespace std;

atomic<uint32_t> Canvas::_nextId{0};        // Initialize the static member variable for canvas.h
atomic<uint32_t> LEDFeature::_nextId{0};    // Initialize the static member variable for ledfeature.h
atomic<uint32_t> SocketChannel::_nextId{0}; // Initialize the static member variable for socketchannel.h

shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("console");

// Main program entry point. Runs the webServer and starts up the LED processing.
// When SIGINT is received, exits gracefully.

int main(int argc, char *argv[])
{
    logger->set_level(spdlog::level::info);

    uint16_t port = 7777;
    string   filename = "config.led";

    // Parse command-line options
    int opt;
    while ((opt = getopt(argc, argv, "p:c:")) != -1) 
    {
        switch (opt) 
        {
            case 'p': 
            {
                int parsedPort = atoi(optarg);
                if (parsedPort < 1 || parsedPort > 65535) 
                {
                    logger->error("Error: Port number must be between 1 and 65535, but was {}", parsedPort);
                    return EXIT_FAILURE;
                }
                port = static_cast<uint16_t>(parsedPort);
                break;
            }
            case 'c':
                filename = optarg;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-p <portid>] [-c <configfile>]" << std::endl;
                return EXIT_FAILURE;
        }
    }

    // Load the canvases from the configuration file

    unique_ptr<Controller> ptrController = Controller::CreateFromFile(filename);
    
    // Until the controller can serialize effects, we still need to manually load things.
    ptrController->LoadSampleCanvases();
        
    ptrController->Connect();
    ptrController->Start();

    // Start the web server
    crow::logger::setLogLevel(crow::LogLevel::WARNING);
    WebServer webServer(*ptrController.get());
    webServer.Start();

    cout << "Shutting down..." << endl;

    // Shut down rendering and communications
    ptrController->Stop();
    ptrController->Disconnect();

    ptrController = nullptr;

    cout << "Shut down complete." << endl;

    return EXIT_SUCCESS;
}
