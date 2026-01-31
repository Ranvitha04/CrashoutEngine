#include "videoloader.h"
#include <iostream>
#include <filesystem>
#include "assetmanager.h"

#ifdef USE_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
#endif

namespace CrashoutsEngine {

    VideoLoader::VideoLoader(const std::string& path) : filepath(path) {
#ifdef USE_FFMPEG
        frame = av_frame_alloc();
        frameRGB = av_frame_alloc();
        packet = av_packet_alloc();
#endif
    }

    VideoLoader::~VideoLoader() {
        Cleanup();
#ifdef USE_FFMPEG
        if (frame) av_frame_free(&frame);
        if (frameRGB) av_frame_free(&frameRGB);
        if (packet) av_packet_free(&packet);
#endif
    }

    bool VideoLoader::Load() {
#ifdef USE_FFMPEG
        std::string fullPath = Assets::rootPath + filepath;
        if (std::filesystem::path(filepath).is_absolute()) fullPath = filepath;

        if (avformat_open_input(&formatCtx, fullPath.c_str(), nullptr, nullptr) != 0) {
            std::cerr << "[VideoLoader] Failed to open: " << fullPath << std::endl;
            return false;
        }

        if (avformat_find_stream_info(formatCtx, nullptr) < 0) return false;

        videoStreamIndex = -1;
        for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
            if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIndex = i;
                break;
            }
        }
        if (videoStreamIndex == -1) return false;

        AVCodecParameters* codecPar = formatCtx->streams[videoStreamIndex]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
        if (!codec) return false;

        codecCtx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codecCtx, codecPar);
        if (avcodec_open2(codecCtx, codec, nullptr) < 0) return false;

        width = codecCtx->width;
        height = codecCtx->height;
        duration = (double)formatCtx->duration / AV_TIME_BASE;

        // Cache Frame Delay
        AVRational fps = formatCtx->streams[videoStreamIndex]->avg_frame_rate;
        if (fps.num > 0) frameDelay = 1.0 / av_q2d(fps);
        else frameDelay = 1.0 / 30.0;

        swsCtx = sws_getContext(width, height, codecCtx->pix_fmt,
                                width, height, AV_PIX_FMT_RGBA,
                                SWS_BILINEAR, nullptr, nullptr, nullptr);

        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
        buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
        av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_RGBA, width, height, 1);

        glGenTextures(1, &textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        isValid = true;
        return true;
#endif
        return false;
    }

    void VideoLoader::Update(float dt) {
        if (!isValid || !isPlaying) return;

        currentTime += dt;
        frameTimer += dt;

        // Catch-up / Frame Skipping logic
        // If we fall behind, decode multiple frames but only the last one 
        // will be uploaded to the GPU (efficiently keeping time sync).
        int safetyLimit = 0;
        while (frameTimer >= frameDelay && safetyLimit < 5) {
            frameTimer -= frameDelay;
            if (!ReadFrame()) break;
            safetyLimit++;
        }
    }

    bool VideoLoader::ReadFrame() {
#ifdef USE_FFMPEG
        while (av_read_frame(formatCtx, packet) >= 0) {
            if (packet->stream_index == videoStreamIndex) {
                if (avcodec_send_packet(codecCtx, packet) == 0) {
                    if (avcodec_receive_frame(codecCtx, frame) == 0) {
                        sws_scale(swsCtx, (uint8_t const* const*)frame->data,
                                  frame->linesize, 0, height,
                                  frameRGB->data, frameRGB->linesize);

                        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                        glBindTexture(GL_TEXTURE_2D, textureHandle);
                        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, frameRGB->data[0]);
                        glBindTexture(GL_TEXTURE_2D, 0);
                        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

                        av_packet_unref(packet);
                        return true;
                    }
                }
            }
            av_packet_unref(packet);
        }
        if (isLooping) {
            avio_seek(formatCtx->pb, 0, SEEK_SET);
            avformat_seek_file(formatCtx, videoStreamIndex, 0, 0, 0, AVSEEK_FLAG_BACKWARD);
        } else isPlaying = false;
#endif
        return false;
    }

    void VideoLoader::Stop() {
        isPlaying = false;
#ifdef USE_FFMPEG
        if (isValid) {
            avio_seek(formatCtx->pb, 0, SEEK_SET);
            avformat_seek_file(formatCtx, videoStreamIndex, 0, 0, 0, AVSEEK_FLAG_BACKWARD);
        }
#endif
    }

    void VideoLoader::Cleanup() {
        if (textureHandle != 0) { glDeleteTextures(1, &textureHandle); textureHandle = 0; }
#ifdef USE_FFMPEG
        if (buffer) av_free(buffer);
        if (codecCtx) avcodec_free_context(&codecCtx);
        if (formatCtx) avformat_close_input(&formatCtx);
        if (swsCtx) sws_freeContext(swsCtx);
#endif
    }
}