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
            .methods(crow::HTTPMethod::GET)([&]()
            {
                auto socketsJson = nlohmann::json::array();
                for (const auto &socket : _controller.GetSockets())
                    socketsJson.push_back(socket); // Assumes `to_json` is defined for `socket`
                return crow::response(socketsJson.dump());
            });


        // Detail a single socket

        CROW_ROUTE(_crowApp, "/api/sockets/<int>")
            .methods(crow::HTTPMethod::GET)([&](int socketId) -> crow::response
            {
                return crow::response(nlohmann::json(_controller.GetSocketById(socketId)).dump());
            });

        // Enumerate all the canvases

        CROW_ROUTE(_crowApp, "/api/canvases")
            .methods(crow::HTTPMethod::GET)([&]()
            {
                nlohmann::json canvasesJson = _controller.Canvases(); // Canvases() can be direcly serialized now
                return crow::response(canvasesJson.dump());
            });

        // Detail a single canvas

        CROW_ROUTE(_crowApp, "/api/canvases/<int>")
            .methods(crow::HTTPMethod::GET)([&](int id) -> crow::response
            {
                auto allCanvases = _controller.Canvases();
                if (id < 0 || id >= allCanvases.size())
                    return {crow::NOT_FOUND, R"({"error": "Canvas not found"})"};

                nlohmann::json canvasJson = allCanvases[id]; // Use the utility function
                return canvasJson.dump(); 
            });
            
            // Create new canvas
            CROW_ROUTE(_crowApp, "/api/canvases")
                .methods(crow::HTTPMethod::POST)([&](const crow::request& req)
                {
                    try 
                    {
                        // Parse and deserialize JSON payload
                        auto jsonPayload = nlohmann::json::parse(req.body);
                        unique_ptr<ICanvas> newCanvas = nullptr;
                        from_json(jsonPayload, newCanvas);

                        // Add the canvas to the controller
                        if (!_controller.AddCanvas(std::move(newCanvas)))
                            return crow::response(400, "Error, likely canvas with that ID already exists.");
                        return crow::response(201, "Canvas added successfully.");
                    } 
                    catch (const std::exception& e) 
                    {
                        return crow::response(400, std::string("Error: ") + e.what());
                    }
                });



            // Delete canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>")
                .methods(crow::HTTPMethod::DELETE)([&](int id) -> crow::response 
                {
                    _controller.DeleteCanvasById(id);
                    return crow::response(crow::OK);
                });

            // Create feature and add to canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>/features")
                .methods(crow::HTTPMethod::POST)([&](const crow::request& req, int canvasId)
                {
                    try 
                    {
                        auto reqJson = nlohmann::json::parse(req.body);
                        auto feature = reqJson.get<std::unique_ptr<ILEDFeature>>();
                        auto newId = _controller.Canvases()[canvasId].get().AddFeature(std::move(feature));
                        return crow::response(nlohmann::json{{"id", newId}}.dump());
                    } 
                    catch (const std::exception& e) 
                    {
                        return crow::response(400, std::string("Error: ") + e.what());
                    }
                });


            // Delete feature from canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>/features/<int>")
                .methods(crow::HTTPMethod::DELETE)([&](int canvasId, int featureId) -> crow::response 
                {
                    _controller.GetCanvasById(canvasId).RemoveFeatureById(featureId);
                    auto canvas = _controller.Canvases()[canvasId];
                    canvas.get().RemoveFeatureById(featureId);
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
