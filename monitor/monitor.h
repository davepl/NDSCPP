 #define _XOPEN_SOURCE_EXTENDED 1
#include <locale.h>
#include <ncurses.h>
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

#include "../serialization.h"
#include "../interfaces.h"

using json = nlohmann::json;

// Updated columns with new information
extern const std::vector<std::pair<std::string, int>> COLUMNS;

// Screen layout constants
constexpr int HEADER_HEIGHT = 3;
constexpr int FOOTER_HEIGHT = 2;


extern size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

inline std::string buildMeter(double value, double threshold, int width = 21)
{
    // Width should be odd to have a center point
    if (width % 2 == 0)
        width++;

    // Normalize value to -1..1 range based on threshold
    double normalized = std::min(1.0, std::max(-1.0, value / threshold));

    // Calculate position (center = width/2)
    int center = width / 2;
    int pos = center + static_cast<int>(normalized * center);

    std::string meter;
    for (int i = 0; i < width; i++)
    {
        meter += (i == pos) ? "|" : "-";
    }
    return meter;
}

inline std::string buildProgressBar(double value, double maximum, int width = 10) 
{
    static const std::array<const char*, 9> blocks = {
        " ", "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"
    };
    
    double percentage = std::min(1.0, std::max(0.0, value / maximum));
    double exactBlocks = percentage * width;
    int fullBlocks = static_cast<int>(exactBlocks);
    int remainder = static_cast<int>((exactBlocks - fullBlocks) * 8);
    
    std::string bar;
    bar.reserve(width * 3);
    
    for (int i = 0; i < fullBlocks; i++) {
        bar += blocks[8];
    }
    
    if (fullBlocks < width) {
        bar += blocks[remainder];
        bar.append(width - fullBlocks - 1, blocks[0][0]);
    }
    return bar;
}



// Helper to make HTTP requests
inline std::string httpGet(const std::string &url)
{
    CURL *curl = curl_easy_init();
    std::string response;

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            response = "Error: " + std::string(curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }
    return response;
}

// Format helpers
inline std::string formatBytes(double bytes)
{
    const char *units[] = {"B/s", "KB/s", "MB/s", "GB/s"};
    int unit = 0;
    while (bytes >= 1024 && unit < 3)
    {
        bytes /= 1024;
        unit++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0) << bytes << units[unit]; // Removed decimal places
    return oss.str();
}

inline std::string formatWifiSignal(double signal)
{
    std::ostringstream oss;
    if (signal == 1000)
        oss << "1Gb/s";
    else if (signal == 10000)
        oss << "10Gb/s";
    else
        oss << std::abs((int)signal) << "dBm"; // Added abs() and removed decimal places
    return oss.str();
}

inline std::string formatTimeDelta(double delta)
{
    string meter = buildMeter(delta, 3.0, 5);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << delta << "s " << meter;
    return oss.str();
}

class Monitor
{
    WINDOW *headerWin;
    WINDOW *contentWin;
    WINDOW *footerWin;
    int contentHeight;
    int scrollOffset = 0;
    std::string baseUrl;
    double _fps;

public:
    Monitor(const std::string& hostname = "localhost", int port = 7777, double fps = 10.0)
        : baseUrl(std::string("http://") + hostname + ":" + std::to_string(port)),
          _fps(fps)
    {
        setlocale(LC_ALL, "");
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        refresh();

        init_pair(1, COLOR_GREEN, COLOR_BLACK);   // Connected
        init_pair(2, COLOR_RED, COLOR_BLACK);     // Disconnected
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);  // Headers
        init_pair(4, COLOR_CYAN, COLOR_BLACK);    // Highlights
        init_pair(5, COLOR_RED, COLOR_BLACK);     // High delta
        init_pair(6, COLOR_YELLOW, COLOR_BLACK);  // Warning level

        int maxY, maxX;
        getmaxyx(stdscr, maxY, maxX);
        contentHeight = maxY - HEADER_HEIGHT - FOOTER_HEIGHT;

        headerWin = newwin(HEADER_HEIGHT, maxX, 0, 0);
        contentWin = newwin(contentHeight, maxX, HEADER_HEIGHT, 0);
        footerWin = newwin(FOOTER_HEIGHT, maxX, maxY - FOOTER_HEIGHT, 0);

        scrollok(contentWin, TRUE);
        keypad(contentWin, TRUE);
    }

    ~Monitor()
    {
        delwin(headerWin);
        delwin(contentWin);
        delwin(footerWin);
        endwin();
    }

    void drawHeader()
    {
        werase(headerWin);
        box(headerWin, 0, 0);
        mvwprintw(headerWin, 0, 2, " NightDriver Monitor ");

        int x = 1;
        wattron(headerWin, COLOR_PAIR(3));
        for (const auto &col : COLUMNS)
        {
            mvwprintw(headerWin, 1, x, "%-*s", col.second, col.first.c_str());
            x += col.second + 1;
        }
        wattroff(headerWin, COLOR_PAIR(3));

        wrefresh(headerWin);
    }

    void drawContent();

    void drawFooter()
    {
        werase(footerWin);
        box(footerWin, 0, 0);
        mvwprintw(footerWin, 0, 2, " Controls ");
        mvwprintw(footerWin, 1, 2, "Q:Quit  ↑/↓:Scroll  R:Refresh");
        wrefresh(footerWin);
    }

    void run()
    {
        // Make content window non-blocking
        nodelay(contentWin, TRUE);
        
        bool running = true;
        auto lastUpdate = std::chrono::steady_clock::now();
        
        while (running)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate);
            
            // Update display every 100ms
            if (elapsed.count() >= 100) {
                drawHeader();
                drawContent();
                drawFooter();
                lastUpdate = now;
            }

            int ch = wgetch(contentWin);
            if (ch != ERR) {  // Only process if we actually got input
                switch (ch)
                {
                    case 'q':
                    case 'Q':
                        running = false;
                        break;
                    case KEY_UP:
                        if (scrollOffset > 0)
                            scrollOffset--;
                        break;
                    case KEY_DOWN:
                        scrollOffset++;
                        break;
                    case 'r':
                    case 'R':
                        lastUpdate = std::chrono::steady_clock::time_point::min(); // Force immediate refresh
                        break;
                }
            }
            
            // Small sleep to prevent CPU spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(_fps > 0 ? static_cast<int>(1000.0 / _fps) : 100));
        }
    }
};