#pragma once
using namespace std;
using namespace std::chrono;

#include "../interfaces.h"
#include "../ledeffectbase.h"
#include "../pixeltypes.h"
#include <string>
#include <iostream>
#include <vector>

extern "C" 
{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
}

class MP4PlaybackEffect : public LEDEffectBase
{
private:
    std::string _filePath;
    AVFormatContext* _formatCtx = nullptr;
    AVCodecContext* _codecCtx = nullptr;
    AVFrame* _frame = nullptr;
    AVPacket* _packet = nullptr;
    SwsContext* _swsCtx = nullptr;
    int _videoStreamIndex = -1;

    bool InitializeFFmpeg()
    {
        // Open the input file
        if (avformat_open_input(&_formatCtx, _filePath.c_str(), nullptr, nullptr) != 0)
        {
            logger->error("Failed to open video file: {}", _filePath);
            return false;
        }

        // Retrieve stream information
        if (avformat_find_stream_info(_formatCtx, nullptr) < 0)
        {
            logger->error("Failed to retrieve stream info.");
            return false;
        }

        // Find the video stream
        for (unsigned i = 0; i < _formatCtx->nb_streams; i++)
        {
            if (_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                _videoStreamIndex = i;
                break;
            }
        }

        if (_videoStreamIndex == -1)
        {
            logger->error("No video stream found.");
            return false;
        }

        // Find the decoder for the video stream
        const AVCodec* codec = avcodec_find_decoder(_formatCtx->streams[_videoStreamIndex]->codecpar->codec_id);
        if (!codec)
        {
            logger->error("Codec not found.");
            return false;
        }

        // Allocate the codec context
        _codecCtx = avcodec_alloc_context3(codec);
        if (!_codecCtx)
        {
            logger->error("Failed to allocate codec context.");
            return false;
        }

        if (avcodec_parameters_to_context(_codecCtx, _formatCtx->streams[_videoStreamIndex]->codecpar) < 0)
        {
            logger->error("Failed to copy codec parameters to context.");
            return false;
        }

        // Open the codec
        if (avcodec_open2(_codecCtx, codec, nullptr) < 0)
        {
            logger->error("Failed to open codec.");
            return false;
        }

        // Allocate frames and packets
        _frame = av_frame_alloc();
        _packet = av_packet_alloc();
        return true;
    }



    void CleanupFFmpeg()
    {
        if (_swsCtx) 
            sws_freeContext(_swsCtx);

        if (_frame) 
            av_frame_free(&_frame);

        if (_packet) 
            av_packet_free(&_packet);

        if (_codecCtx) 
            avcodec_free_context(&_codecCtx);

        if (_formatCtx) 
            avformat_close_input(&_formatCtx);
    }

public:
    MP4PlaybackEffect(const std::string& name, const std::string& filePath)
        : LEDEffectBase(name), _filePath(filePath) {}

    ~MP4PlaybackEffect()
    {
        CleanupFFmpeg();
    }

    void Start(ICanvas& canvas) override
    {
        if (!InitializeFFmpeg())
        {
            logger->error("Failed to initialize FFmpeg for MP4 playback.");
            return;
        }

        auto& graphics = canvas.Graphics();
        int canvasWidth = graphics.Width();
        int canvasHeight = graphics.Height();

        _swsCtx = sws_getContext(
            _codecCtx->width, _codecCtx->height, _codecCtx->pix_fmt,
            canvasWidth, canvasHeight, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
    }

    void Update(ICanvas& canvas, milliseconds deltaTime) override 
    {
        if (!_formatCtx || !_codecCtx || !_swsCtx) 
            return;

        while (av_read_frame(_formatCtx, _packet) >= 0)
        {
            if (_packet->stream_index == _videoStreamIndex)
            {
                if (avcodec_send_packet(_codecCtx, _packet) == 0)
                {
                    while (avcodec_receive_frame(_codecCtx, _frame) == 0)
                    {
                        auto& graphics = canvas.Graphics();
                        int canvasWidth = graphics.Width();
                        int canvasHeight = graphics.Height();

                        // Buffer for RGB24 pixels
                        std::vector<uint8_t> rgbBuffer(canvasWidth * canvasHeight * 3);

                        uint8_t* dstData[1] = { rgbBuffer.data() };
                        int dstLinesize[1] = { 3 * canvasWidth };

                        sws_scale(_swsCtx, _frame->data, _frame->linesize, 0, _codecCtx->height, dstData, dstLinesize);

                        // Render to canvas
                        for (int y = 0; y < canvasHeight; ++y)
                        {
                            for (int x = 0; x < canvasWidth; ++x)
                            {
                                int idx = (y * canvasWidth + x) * 3;
                                graphics.SetPixel(x, y, CRGB(rgbBuffer[idx], rgbBuffer[idx + 1], rgbBuffer[idx + 2]));
                            }
                        }

                        av_packet_unref(_packet);
                        return; // Process one frame per update
                    }
                }
            }
            av_packet_unref(_packet);
        }

        // Loop the video by restarting
        av_seek_frame(_formatCtx, _videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
    }

    void ToJson(nlohmann::json& j) const override
    {
        j = {
            {"type", "MP4PlaybackEffect"},
            {"name", Name()},
            {"filePath", _filePath}
        };
    }

    static std::unique_ptr<MP4PlaybackEffect> FromJson(const nlohmann::json& j)
    {
        return std::make_unique<MP4PlaybackEffect>(
            j.at("name").get<std::string>(),
            j.at("filePath").get<std::string>()
        );
    }
};
