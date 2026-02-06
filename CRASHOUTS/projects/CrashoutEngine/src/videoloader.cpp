/*!
@file		videoloader.cpp
@author     m.biniswandi@digipen.edu
@date       31/01/2026
@brief		Implementation of the VideoLoader class, responsible for loading and playing video files using FFmpeg and FMOD.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
#include "videoloader.h"
#include "audioSystem.h"
#include <iostream>
#include <filesystem>
#include "assetmanager.h"
#include <fmod.hpp>

#ifdef USE_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
}
#endif

namespace CrashoutsEngine {

	/**
	 * @brief Constructor for VideoLoader.
	 * @param path The relative path to the video file.
	 */
	VideoLoader::VideoLoader(const std::string& path) : filepath(path) {
#ifdef USE_FFMPEG
		frame = av_frame_alloc();
		frameRGB = av_frame_alloc();
		audioFrame = av_frame_alloc();
		packet = av_packet_alloc();
#endif
	}

	/**
	 * @brief Destructor for VideoLoader, ensures proper cleanup.
	 */
	VideoLoader::~VideoLoader() {
		Cleanup();
#ifdef USE_FFMPEG
		if (frame) av_frame_free(&frame);
		if (frameRGB) av_frame_free(&frameRGB);
		if (audioFrame) av_frame_free(&audioFrame);
		if (packet) av_packet_free(&packet);
#endif
	}

	/**
	 * @brief Loads the video file and initializes FFmpeg and FMOD resources.
	 * @return True if loading was successful, false otherwise.
	 */
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
		audioStreamIndex = -1;
		for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
			if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIndex == -1) {
				videoStreamIndex = i;
			}
			else if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIndex == -1) {
				audioStreamIndex = i;
			}
		}
		if (videoStreamIndex == -1) return false;

		// --- Video Setup ---
		AVCodecParameters* codecPar = formatCtx->streams[videoStreamIndex]->codecpar;
		const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
		if (!codec) return false;
		codecCtx = avcodec_alloc_context3(codec);
		avcodec_parameters_to_context(codecCtx, codecPar);
		if (avcodec_open2(codecCtx, codec, nullptr) < 0) return false;

		width = codecCtx->width;
		height = codecCtx->height;
		duration = (double)formatCtx->duration / AV_TIME_BASE;

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

		// --- Audio Extraction ---
		if (audioStreamIndex != -1) {
			AVCodecParameters* aCodecPar = formatCtx->streams[audioStreamIndex]->codecpar;
			const AVCodec* aCodec = avcodec_find_decoder(aCodecPar->codec_id);
			if (aCodec) {
				audioCodecCtx = avcodec_alloc_context3(aCodec);
				avcodec_parameters_to_context(audioCodecCtx, aCodecPar);
				if (avcodec_open2(audioCodecCtx, aCodec, nullptr) == 0) {

					// Setup Resampler (Target: 44100Hz Stereo Float)
					AVChannelLayout out_ch_layout;
					av_channel_layout_default(&out_ch_layout, 2);
					AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_FLT;
					int out_sample_rate = 44100;

					swr_alloc_set_opts2(&swrCtx,
						&out_ch_layout, out_sample_fmt, out_sample_rate,
						&audioCodecCtx->ch_layout, audioCodecCtx->sample_fmt, audioCodecCtx->sample_rate,
						0, NULL);
					swr_init(swrCtx);

					std::vector<float> pcmData;

					// Read all packets
					AVPacket* aPacket = av_packet_alloc();
					while (av_read_frame(formatCtx, aPacket) >= 0) {
						if (aPacket->stream_index == audioStreamIndex) {
							if (avcodec_send_packet(audioCodecCtx, aPacket) == 0) {
								while (avcodec_receive_frame(audioCodecCtx, audioFrame) == 0) {
									// Convert to target format
									int max_out_samples = swr_get_out_samples(swrCtx, audioFrame->nb_samples);
									float* out_buf = nullptr;
									av_samples_alloc((uint8_t**)&out_buf, NULL, 2, max_out_samples, out_sample_fmt, 0);

									int converted = swr_convert(swrCtx, (uint8_t**)&out_buf, max_out_samples,
										(const uint8_t**)audioFrame->data, audioFrame->nb_samples);

									if (converted > 0) {
										pcmData.insert(pcmData.end(), out_buf, out_buf + (converted * 2));
									}
									av_freep(&out_buf);
								}
							}
						}
						av_packet_unref(aPacket);
					}
					av_packet_free(&aPacket);

					// Create FMOD User Sound
					if (!pcmData.empty()) {
						FMOD_CREATESOUNDEXINFO exinfo = { 0 };
						exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
						exinfo.length = (unsigned int)pcmData.size() * sizeof(float);
						exinfo.numchannels = 2;
						exinfo.defaultfrequency = 44100;
						exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;

						FMOD::System* core = Audio::AudioSystem::instance().core();
						FMOD_RESULT res = core->createSound((const char*)pcmData.data(),
							FMOD_OPENMEMORY | FMOD_OPENRAW | FMOD_2D, &exinfo, &audioSound);

						if (res == FMOD_OK) {
							hasAudio = true;
							std::cout << "[VideoLoader] Audio extracted successfully: " << (pcmData.size() / (2 * 44100)) << " seconds." << std::endl;
						}
					}

					// Seek back to start for video playback
					avformat_seek_file(formatCtx, -1, 0, 0, 0, AVSEEK_FLAG_BACKWARD);
					avcodec_flush_buffers(codecCtx);
				}
			}
		}

		isValid = true;
		return true;
#endif
		return false;
	}

	/**
	 * @brief Updates the video playback state and synchronizes with audio.
	 * @param dt Delta time since the last frame.
	 */
	void VideoLoader::Update(float dt) {
		if (!isValid || !isPlaying) {
			if (audioChannel) audioChannel->setPaused(true);
			return;
		}

		// --- Audio Sync ---
		if (hasAudio && audioSound) {
			if (!audioChannel) {
				FMOD::System* core = Audio::AudioSystem::instance().core();
				core->playSound(audioSound, nullptr, false, &audioChannel);
			}

			bool paused;
			audioChannel->getPaused(&paused);
			if (paused) audioChannel->setPaused(false);

			unsigned int ms = 0;
			audioChannel->getPosition(&ms, FMOD_TIMEUNIT_MS);
			double audioTime = (double)ms / 1000.0;

			// Use audio clock to drive the video clock if drift is detected
			if (abs(currentTime - audioTime) > 0.05) {
				double drift = audioTime - currentTime;
				currentTime = audioTime;
				frameTimer += drift; // Adjust timer to trigger catch-up or delay
			}
		}

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

	/**
	 * @brief Decodes the next video frame and uploads it to the GPU texture.
	 * @return True if a frame was successfully read and uploaded, false otherwise.
	 */
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
		}
		else isPlaying = false;
#endif
		return false;
	}

	/**
	 * @brief Stops video playback and resets the playback position.
	 */
	void VideoLoader::Stop() {
		isPlaying = false;
		currentTime = 0;
		if (audioChannel) {
			audioChannel->setPaused(true);
			audioChannel->setPosition(0, FMOD_TIMEUNIT_MS);
		}
#ifdef USE_FFMPEG
		if (isValid) {
			avio_seek(formatCtx->pb, 0, SEEK_SET);
			avformat_seek_file(formatCtx, videoStreamIndex, 0, 0, 0, AVSEEK_FLAG_BACKWARD);
		}
#endif
	}

	/**
	 * @brief Sets the volume of the video's audio track.
	 * @param vol The volume level (0.0 to 1.0).
	 */
	void VideoLoader::SetVolume(float vol) {
		if (audioChannel) {
			audioChannel->setVolume(vol);
		}
	}

	/**
	 * @brief Cleans up and releases all allocated video and audio resources.
	 */
	void VideoLoader::Cleanup() {
		if (audioChannel) {
			audioChannel->stop();
			audioChannel = nullptr;
		}
		if (audioSound) {
			audioSound->release();
			audioSound = nullptr;
		}
		if (textureHandle != 0) { glDeleteTextures(1, &textureHandle); textureHandle = 0; }
#ifdef USE_FFMPEG
		if (buffer) av_free(buffer);
		if (codecCtx) avcodec_free_context(&codecCtx);
		if (audioCodecCtx) avcodec_free_context(&audioCodecCtx);
		if (swsCtx) sws_freeContext(swsCtx);
		if (swrCtx) swr_free(&swrCtx);
		if (formatCtx) avformat_close_input(&formatCtx);
#endif
	}
}