// WebServer
//
// WebServer class implementation

#define PORT 7777

// Function to handle incoming HTTP requests
#include <microhttpd.h>
#include <cstring>
#include <string>
#include <map>
#include "webserver.h"
#include "serialization.h"

using namespace std;

// WebServer::AnswerConnection
//
// Handle an incoming connection request

enum MHD_Result WebServer::AnswerConnection(void                   * cls,
                                             struct MHD_Connection * connection,
                                             const char            * url,
                                             const char            * method,
                                             const char            * version,
                                             const char            * upload_data,
                                             size_t                * upload_data_size,
                                             void                 ** con_cls)
{
    WebServer * server = static_cast<WebServer*>(cls);

    string response_text;
    struct MHD_Response *response;
    enum MHD_Result ret;

    if (strcmp(method, "GET") != 0)
        return MHD_NO;  // Only accept GET method

    // Lambda function to handle key-value pairs from the query string, used by MHD_get_connection_values
    auto keyValueHandler = [](void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
    {
        auto params = static_cast<map<string, string>*>(cls);
        if (value)
            (*params)[key] = value;
        else
            (*params)[key] = "";                // Handle the case where the value is missing 
        return MHD_YES;
    };

    map<string, string> parameters;
    MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, keyValueHandler, &parameters);

    uint response_code = MHD_HTTP_OK;

    // Check for REST API endpoints
    if (strncmp(url, "/api/canvases", 13) == 0) 
    {
        // List all canvases
        if (strcmp(url, "/api/canvases") == 0)
        {
            auto canvasesJson = nlohmann::json::array();
            for (size_t i = 0; i < server->_allCanvases.size(); ++i)
            {
                nlohmann::json canvasJson = * server->_allCanvases[i];
                canvasJson["id"] = i;                       // Add ID for reference
                canvasesJson.push_back(canvasJson);
            }
            response_text = canvasesJson.dump();
        }
        // Get details for a specific canvas
        else if (strncmp(url, "/api/canvases/", 14) == 0)
        {
            size_t id = stoi(url + 14); // Extract ID from the URL
            if (id < server->_allCanvases.size())
            {
                nlohmann::json canvasJson = * server->_allCanvases[id];
                canvasJson["id"] = id;                      // Include ID in the details
                response_text = canvasJson.dump();
            }
            else
            {
                response_code = MHD_HTTP_NOT_FOUND;
                response_text = R"({"error": "Canvas not found"})";
            }
        }
        else
        {
            response_code = MHD_HTTP_BAD_REQUEST;
            response_text = R"({"error": "Invalid API request"})";
        }
    }
    else
    {
        response_code = MHD_HTTP_BAD_REQUEST;
        response_text = R"({"error": "Unknown Request"})";
    }

    response = MHD_create_response_from_buffer(response_text.size(), (void*)response_text.c_str(), MHD_RESPMEM_MUST_COPY);

    // Allow cross-domain requests by setting the CORS header
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");

    ret = MHD_queue_response(connection, response_code, response);
    MHD_destroy_response(response);
    return ret;
}



// WebServer::RunServer
//
// Function to initialize and run the HTTP server, which then waits for the completion mutex to be signalled
// before returning

void * WebServer::RunServer(void *arg)
{
    WebServer * that = (WebServer *) arg;

    that->daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, PORT, NULL, NULL, &AnswerConnection, that, MHD_OPTION_END);
    if (NULL == that->daemon) {
        fprintf(stderr, "Failed to start the daemon\n");
        return nullptr;
    }

    printf("Server started on port %d\n", PORT);

    // Wait for the signal to stop the server
    pthread_mutex_lock(&that->mutex);
    pthread_cond_wait(&that->cond, &that->mutex);
    pthread_mutex_unlock(&that->mutex);

    MHD_stop_daemon(that->daemon);
    that->daemon = nullptr;
    printf("Server stopped\n");
    return nullptr;
}

pthread_t WebServer::Start()
{

    // Create a thread to run the HTTP server
    if (pthread_create(&server_thread, NULL, &WebServer::RunServer, this) != 0) {
        fprintf(stderr, "Failed to create the server thread\n");
        return 0;
    }

    return server_thread;
}

void WebServer::Stop()
{
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}


