#include <csignal>
#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include "global.h"
#include "canvas.h"
#include "canvascontroller.h"
#include "clientchannel.h"
#include "interfaces.h"
#include "socketchannel.h"
#include "socketcontroller.h"
#include "ledeffectbase.h"
#include "utilities.h"
#include "server.h"

using namespace std;

// Global Objects
WebServer webServer;

// Atomic flag to indicate whether the program should continue running.
// Will be set to false when SIGINT is received.
atomic<bool> running(true);

void handle_signal(int signal)
{
    if (signal == SIGINT)
    {
        running = false;
        cerr << "Received SIGINT, exiting...\n" << flush;
    }
}

// Main program entry point. Runs the webServer and updates the known symbols list every hour.
// When SIGINT is received, exits gracefully.
int main(int, char *[])
{
    // Register signal handler for SIGINT
    signal(SIGINT, handle_signal);

    // Start the web server
    pthread_t serverThread = webServer.Start();
    if (!serverThread)
    {
        cerr << "Failed to start the server thread\n";
        return 1;
    }
    cout << "Started server, waiting..." << endl;

    // Main program loop
    while (running)
    {
        for (int i = 0; i < 3600 && running; ++i)
            this_thread::sleep_for(chrono::seconds(1)); // Sleep in one-second increments
    }

    cout << "Stopping server..." << endl;
    webServer.Stop();
    return 0;
}
