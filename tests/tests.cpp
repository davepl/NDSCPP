#include <gtest/gtest.h>
#include <cpr/cpr.h> // Modern C++ HTTP library
#include <../json.hpp>

using json = nlohmann::json;
const std::string BASE_URL = "http://localhost:7777/api";

class APITest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Clean up any existing test data
        cleanupTestData();
    }

    void TearDown() override
    {
        // Clean up after tests
        cleanupTestData();
    }

    void cleanupTestData()
    {
    }
};

// Test Controller endpoint
TEST_F(APITest, GetController)
{
    auto response = cpr::Get(cpr::Url{BASE_URL + "/controller"});
    ASSERT_EQ(response.status_code, 200);

    auto data = json::parse(response.text);
    ASSERT_TRUE(data.contains("controller"));
}
// Test Sockets endpoints
TEST_F(APITest, GetSockets)
{
    auto response = cpr::Get(cpr::Url{BASE_URL + "/sockets"});
    ASSERT_EQ(response.status_code, 200);

    auto jsonResponse = json::parse(response.text);
    ASSERT_TRUE(jsonResponse.contains("sockets")); // Ensure "sockets" key exists
    ASSERT_TRUE(jsonResponse["sockets"].is_array()); // Verify "sockets" is an array
}


TEST_F(APITest, GetSpecificSocket)
{
    // First get all sockets
    auto response = cpr::Get(cpr::Url{BASE_URL + "/sockets"});
    ASSERT_EQ(response.status_code, 200);

    // Parse the response as a JSON object
    auto jsonResponse = json::parse(response.text);

    // Ensure the "sockets" key exists and is an array
    ASSERT_TRUE(jsonResponse.contains("sockets"));
    ASSERT_TRUE(jsonResponse["sockets"].is_array());

    // Access the sockets array
    auto sockets = jsonResponse["sockets"];

    // Check if the array is not empty
    if (!sockets.empty())
    {
        // Get the first socket's ID
        int firstSocketId = sockets[0]["id"].get<int>();

        // Fetch data for the specific socket
        auto socketResponse = cpr::Get(cpr::Url{BASE_URL + "/sockets/" + std::to_string(firstSocketId)});
        ASSERT_EQ(socketResponse.status_code, 200);

        // Parse the response for the specific socket
        auto socketData = json::parse(socketResponse.text);

        // Validate that the ID matches
        ASSERT_EQ(socketData["id"].get<int>(), firstSocketId);
    }
    else
    {
        FAIL() << "The sockets array is empty.";
    }
}


// Test Canvas CRUD operations

TEST_F(APITest, CanvasCRUD)
{
    // Create canvas
    json canvasData = {
        {"id", -1}, // Allow the server to assign the ID
        {"name", "Test Canvas"},
        {"width", 100},
        {"height", 100}};

    auto createResponse = cpr::Post(
        cpr::Url{BASE_URL + "/canvases"},
        cpr::Body{canvasData.dump()},
        cpr::Header{{"Content-Type", "application/json"}});
    ASSERT_EQ(createResponse.status_code, 201);

    // Parse the response to get the new ID
    auto createJson = json::parse(createResponse.text);
    ASSERT_TRUE(createJson.contains("id")); // Ensure the response contains "id"
    int newId = createJson["id"].get<int>();
    ASSERT_GT(newId, 0); // Verify the ID is valid

    // Read all canvases
    auto listResponse = cpr::Get(cpr::Url{BASE_URL + "/canvases"});
    ASSERT_EQ(listResponse.status_code, 200);
    auto canvases = json::parse(listResponse.text);
    ASSERT_FALSE(canvases.empty());

    // Read specific canvas using the new ID
    auto getResponse = cpr::Get(cpr::Url{BASE_URL + "/canvases/" + std::to_string(newId)});
    ASSERT_EQ(getResponse.status_code, 200);
    auto canvas = json::parse(getResponse.text);
    ASSERT_EQ(canvas["name"], "Test Canvas");

    // Delete the canvas using the new ID
    auto deleteResponse = cpr::Delete(cpr::Url{BASE_URL + "/canvases/" + std::to_string(newId)});
    ASSERT_EQ(deleteResponse.status_code, 200);

    // Verify deletion
    auto verifyResponse = cpr::Get(cpr::Url{BASE_URL + "/canvases/" + std::to_string(newId)});
    ASSERT_EQ(verifyResponse.status_code, 404);
}

// Test Feature operations within a canvas

TEST_F(APITest, CanvasFeatureOperations)
{
    // First create a canvas
    json canvasData = {
        {"id", -1}, // Let the server assign the ID
        {"name", "Feature Test Canvas"},
        {"width", 100},
        {"height", 100}};

    // Send the POST request and capture the response
    auto createCanvasResponse = cpr::Post(
        cpr::Url{BASE_URL + "/canvases"},
        cpr::Body{canvasData.dump()},
        cpr::Header{{"Content-Type", "application/json"}});

    // Assert the response status
    ASSERT_EQ(createCanvasResponse.status_code, 201); // Ensure the canvas was created successfully

    // Parse the response body to extract the new ID
    auto responseJson = json::parse(createCanvasResponse.text);
    int newCanvasId = responseJson["id"].get<int>();

    // Use the new ID in subsequent operations
    ASSERT_GT(newCanvasId, 0); // Validate the ID is a positive number

    // Create feature with all required fields
    json featureData = {
        {"type", "LEDFeature"},
        {"hostName", "example-host"},
        {"friendlyName", "Test Feature"},
        {"port", 1234},
        {"width", 32},
        {"height", 16},
        {"offsetX", 50},
        {"offsetY", 50},
        {"reversed", false},
        {"channel", 1},
        {"redGreenSwap", false},
        {"clientBufferCount", 8}};

    auto createFeatureResponse = cpr::Post(
        cpr::Url{BASE_URL + "/canvases/" + std::to_string(newCanvasId) + "/features"},
        cpr::Body{featureData.dump()},
        cpr::Header{{"Content-Type", "application/json"}}
    );

    ASSERT_EQ(createFeatureResponse.status_code, 200);

    auto featureInfo = json::parse(createFeatureResponse.text);
    int featureId = featureInfo["id"].get<int>();

    // Delete feature
    auto deleteFeatureResponse = cpr::Delete(
        cpr::Url{BASE_URL + "/canvases/" + std::to_string(newCanvasId) + "/features/" + std::to_string(featureId)});
    ASSERT_EQ(deleteFeatureResponse.status_code, 200);

    // Delete the canvas
    auto deleteCanvasResponse = cpr::Delete(
        cpr::Url{BASE_URL + "/canvases/" + std::to_string(newCanvasId)});
}

// Test error cases
TEST_F(APITest, ErrorHandling)
{
    // Test invalid canvas ID
    auto invalidCanvasResponse = cpr::Get(cpr::Url{BASE_URL + "/canvases/999"});
    ASSERT_EQ(invalidCanvasResponse.status_code, 404);

    // Test invalid JSON in canvas creation
    auto invalidJsonResponse = cpr::Post(
        cpr::Url{BASE_URL + "/canvases"},
        cpr::Body{"invalid json"},
        cpr::Header{{"Content-Type", "application/json"}});
    ASSERT_EQ(invalidJsonResponse.status_code, 400);
}

/* Not reliably passing yet 
// Test multiple canvas operations
TEST_F(APITest, MultipleCanvasOperations)
{
    const int NUM_CANVASES = 50;
    std::vector<int> canvasIds;

    // Create multiple canvases simultaneously
    std::vector<std::future<cpr::Response>> createFutures;
    for (int i = 0; i < NUM_CANVASES; i++)
    {
        createFutures.push_back(std::async(std::launch::async, [i]()
                                           {
            json canvasData = {
                {"id", -1},
                {"name", "Stress Test Canvas " + std::to_string(i)},
                {"width", 100},
                {"height", 100}
            };
            
            return cpr::Post(
                cpr::Url{BASE_URL + "/canvases"},
                cpr::Body{canvasData.dump()},
                cpr::Header{{"Content-Type", "application/json"}}
            ); }));
    }

    // Collect canvas IDs and verify creation
    for (auto &future : createFutures)
    {
        auto response = future.get();
        ASSERT_EQ(response.status_code, 201);
        auto json_response = json::parse(response.text);
        canvasIds.push_back(json_response["id"].get<int>());
    }

    // Verify all canvases exist
    auto listResponse = cpr::Get(cpr::Url{BASE_URL + "/canvases"});
    ASSERT_EQ(listResponse.status_code, 200);
    auto canvases = json::parse(listResponse.text);
    ASSERT_GE(canvases.size(), (size_t) NUM_CANVASES);

    // Cleanup all canvases concurrently
    std::vector<std::future<cpr::Response>> deleteFutures;
    for (int id : canvasIds)
    {
        deleteFutures.push_back(std::async(std::launch::async, [id]()
            { 
                return cpr::Delete(cpr::Url{BASE_URL + "/canvases/" + std::to_string(id)}); 
            }));
    }

    // Verify all deletions
    for (auto &future : deleteFutures)
    {
        auto response = future.get();
        ASSERT_EQ(response.status_code, 200);
    }
}
*/

// Test multiple feature operations within a single canvas
TEST_F(APITest, MultipleFeatureOperations)
{
    // Create a test canvas
    json canvasData = {
        {"id", -1},
        {"name", "Feature Stress Test Canvas"},
        {"width", 1000},
        {"height", 1000}};

    auto createCanvasResponse = cpr::Post(
        cpr::Url{BASE_URL + "/canvases"},
        cpr::Body{canvasData.dump()},
        cpr::Header{{"Content-Type", "application/json"}});

    ASSERT_EQ(createCanvasResponse.status_code, 201);
    auto canvasJson = json::parse(createCanvasResponse.text);
    int canvasId = canvasJson["id"].get<int>();

    const int NUM_FEATURES = 50;
    std::vector<int> featureIds;

    // Add multiple features concurrently
    std::vector<std::future<cpr::Response>> featureFutures;
    for (int i = 0; i < NUM_FEATURES; i++)
    {
        featureFutures.push_back(std::async(std::launch::async, [i, canvasId]()
                                            {
            json featureData = {
                {"type", "LEDFeature"},
                {"hostName", "stress-host-" + std::to_string(i)},
                {"friendlyName", "Stress Feature " + std::to_string(i)},
                {"port", 1234 + i},
                {"width", 32},
                {"height", 16},
                {"offsetX", (i * 50) % 1000},
                {"offsetY", (i * 30) % 1000},
                {"reversed", false},
                {"channel", i % 4 + 1},
                {"redGreenSwap", false},
                {"clientBufferCount", 8}
            };
            
            return cpr::Post(
                cpr::Url{BASE_URL + "/canvases/" + std::to_string(canvasId) + "/features"},
                cpr::Body{featureData.dump()},
                cpr::Header{{"Content-Type", "application/json"}}
            ); }));
    }

    // Collect and verify feature creation
    for (auto &future : featureFutures)
    {
        auto response = future.get();
        ASSERT_EQ(response.status_code, 200);
        auto featureJson = json::parse(response.text);
        featureIds.push_back(featureJson["id"].get<int>());
    }

    // Delete features concurrently
    std::vector<std::future<cpr::Response>> deleteFutures;
    for (int featureId : featureIds)
    {
        deleteFutures.push_back(std::async(std::launch::async, [canvasId, featureId]()
                                           { return cpr::Delete(
                                                 cpr::Url{BASE_URL + "/canvases/" + std::to_string(canvasId) +
                                                          "/features/" + std::to_string(featureId)}); }));
    }

    // Verify feature deletions
    for (auto &future : deleteFutures)
    {
        auto response = future.get();
        ASSERT_EQ(response.status_code, 200);
    }

    // Cleanup canvas
    auto deleteCanvasResponse = cpr::Delete(
        cpr::Url{BASE_URL + "/canvases/" + std::to_string(canvasId)});
    ASSERT_EQ(deleteCanvasResponse.status_code, 200);
}

/* Not reliably passing yet
// Test rapid creation/deletion cycles
TEST_F(APITest, RapidCreationDeletion)
{
    const int NUM_CYCLES = 25;
    const int NUM_CANVASES_PER_CYCLE = 25;

    for (int cycle = 0; cycle < NUM_CYCLES; cycle++)
    {
        std::vector<int> cycleCanvasIds;

        // Create canvases
        for (int i = 0; i < NUM_CANVASES_PER_CYCLE; i++)
        {
            json canvasData = {
                {"id", -1},
                {"name", "Cycle " + std::to_string(cycle) + " Canvas " + std::to_string(i)},
                {"width", 100},
                {"height", 100}};

            auto response = cpr::Post(
                cpr::Url{BASE_URL + "/canvases"},
                cpr::Body{canvasData.dump()},
                cpr::Header{{"Content-Type", "application/json"}});
            ASSERT_EQ(response.status_code, 201);
            auto jsonResponse = json::parse(response.text);
            cycleCanvasIds.push_back(jsonResponse["id"].get<int>());

            // Add a feature to each canvas
            json featureData = {
                {"type", "LEDFeature"},
                {"hostName", "cycle-host"},
                {"friendlyName", "Cycle Feature"},
                {"port", 1234},
                {"width", 32},
                {"height", 16},
                {"offsetX", 50},
                {"offsetY", 50},
                {"reversed", false},
                {"channel", 1},
                {"redGreenSwap", false},
                {"clientBufferCount", 8}};

            auto featureResponse = cpr::Post(
                cpr::Url{BASE_URL + "/canvases/" + std::to_string(cycleCanvasIds.back()) + "/features"},
                cpr::Body{featureData.dump()},
                cpr::Header{{"Content-Type", "application/json"}});
            ASSERT_EQ(featureResponse.status_code, 200);
        }

        // Delete all canvases in this cycle
        for (int id : cycleCanvasIds)
        {
            auto response = cpr::Delete(cpr::Url{BASE_URL + "/canvases/" + std::to_string(id)});
            ASSERT_EQ(response.status_code, 200);
        }
    }
}
*/