#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <ranges>
#include "json.hpp"
#include "crow_all.h"
#include "interfaces.h" // Assuming ICanvas is defined here
#include "serialization.h"

using namespace std;

class WebServer
{
private:
    struct HeaderMiddleware
    {
        struct context
        {
        };

        void before_handle(crow::request &req, crow::response &res, context &ctx)
        {
        }

        void after_handle(crow::request &req, crow::response &res, context &ctx)
        {
            res.set_header("Content-Type", "application/json");
            res.add_header("Access-Control-Allow-Origin", "*");
            res.add_header("Access-Control-Allow-Methods", "GET, OPTIONS");
        }
    };

    IController & _controller; // Reference to all canvases
    crow::App<HeaderMiddleware> _crowApp;

public:
    WebServer(IController & controller) : _controller(controller)
    {
    }

    ~WebServer()
    {
    }

    void Start()
    {
        // The main controller, the most info you can get in a single call

        CROW_ROUTE(_crowApp, "/api/controller")
            .methods(crow::HTTPMethod::GET)([&]() -> crow::response
            {
                nlohmann::json response;
                response["controller"] = _controller;
                return response.dump();

            });

        // Enumerate just the sockets

        CROW_ROUTE(_crowApp, "/api/sockets")
            .methods(crow::HTTPMethod::GET)([&]() -> crow::response
            {
                auto sockets = _controller.GetSockets();
                auto socketsJson = nlohmann::json::array();
                for (const auto &socket : sockets)
                {
                    nlohmann::json socketJson;
                    to_json(socketJson, *socket);
                    socketsJson.push_back(socketJson);
                }
                return socketsJson.dump();
            });

        // Detail a single socket

        CROW_ROUTE(_crowApp, "/api/sockets/<int>")
            .methods(crow::HTTPMethod::GET)([&](int socketId) -> crow::response
            {
                auto socket = _controller.GetSocketById(socketId);
                if (!socket)
                    return {crow::NOT_FOUND, R"({"error": "Socket not found"})"};
                
                // Return the socket using the to_json function

                nlohmann::json socketJson;
                to_json(socketJson, *socket);
                return socketJson.dump();
            });

        // Enumerate all the canvases

        CROW_ROUTE(_crowApp, "/api/canvases")
            .methods(crow::HTTPMethod::GET)([&]() -> crow::response
            {
                auto allCanvases = _controller.Canvases();
                auto canvasesJson = nlohmann::json::array();
                
                for (size_t i = 0; i < allCanvases.size(); ++i)
                {
                    nlohmann::json canvasJson = *_controller.Canvases()[i]; // Use the utility function
                    canvasesJson.push_back(canvasJson);
                }
                return canvasesJson.dump(); 
            });

        // Detail a single canvas

        CROW_ROUTE(_crowApp, "/api/canvases/<int>")
            .methods(crow::HTTPMethod::GET)([&](int id) -> crow::response
            {
                auto allCanvases = _controller.Canvases();
                if (id < 0 || id >= allCanvases.size())
                    return {crow::NOT_FOUND, R"({"error": "Canvas not found"})"};

                nlohmann::json canvasJson = *allCanvases[id]; // Use the utility function
                return canvasJson.dump(); 
            });
            
            // Create new canvas
            CROW_ROUTE(_crowApp, "/api/canvases")
                .methods(crow::HTTPMethod::POST)([&](const crow::request& req) -> crow::response 
                {
                    try 
                    {
                        // Parse the incoming JSON payload
                        nlohmann::json jsonPayload = nlohmann::json::parse(req.body);
                        
                        // Create a new canvas using from_json
                        std::unique_ptr<ICanvas> newCanvas;
                        from_json(jsonPayload, newCanvas);

                        // Add the canvas to the controller
                        if (false == _controller.AddCanvas(std::move(newCanvas)))
                            return crow::response(400, "Canvas with that ID already exists.");
                        else
                            return crow::response(201, "Canvas added successfully.");
                    } 
                    catch (const std::exception& e) 
                    {
                        // Handle errors (e.g., JSON parsing or validation)
                        return crow::response(400, std::string("Error: ") + e.what());
                    }
                });


            // Delete canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>")
                .methods(crow::HTTPMethod::DELETE)([&](int id) -> crow::response 
                {
                    if (id < 0 || id >= _controller.Canvases().size())
                        return {crow::NOT_FOUND, R"({"error": "Canvas not found"})"};
                    _controller.DeleteCanvasById(id);
                    return crow::response(crow::OK);
                });

            // Create feature and add to canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>/features")
                .methods(crow::HTTPMethod::POST)([&](const crow::request& req, int canvasId) -> crow::response 
                {
                    nlohmann::json response;
                    auto reqJson = nlohmann::json::parse(req.body);
                    auto canvas = _controller.Canvases()[canvasId];
                    auto newId  = canvas->AddFeature(reqJson.get<unique_ptr<ILEDFeature>>());
                    response["id"] = newId;
                    return response.dump();
                });

            // Delete feature from canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>/features/<int>")
                .methods(crow::HTTPMethod::DELETE)([&](int canvasId, int featureId) -> crow::response 
                {
                    _controller.GetCanvasById(canvasId)->RemoveFeatureById(featureId);
                    auto canvas = _controller.Canvases()[canvasId];
                    canvas->RemoveFeatureById(featureId);
                    return crow::response(crow::OK);
                });
                
        // Start the server
        _crowApp.port(_controller.GetPort()).multithreaded().run();
    }

    void Stop()
    {
        _crowApp.stop();
    }
};
