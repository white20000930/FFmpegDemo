/**
 * @file pcm_yuv_player.h
 * @author 晴天天
 * @brief PCM YUV简易播放器
 * @version 0.1
 * @date 2024-06-20
 */

#ifndef PCM_YUV_PLAYER_H
#define PCM_YUV_PLAYER_H

#include <iostream>
#include <SDL.h>
#include "get_path.h"
class PcmYuvPlayer
{
public:
    PcmYuvPlayer();
    ~PcmYuvPlayer();

public:
    // 播放
    void PlayYUV();
    void PlayPCM();

private:
    // 设置yuv播放参数
    void SetYuvPlayer(const char *yuv_filename);
    void SetYuvPlayer(const char *yuv_filename, int width, int height, int fps);

    // 设置pcm播放参数
    void SetPcmPlayer(const char *pcm_filename);
    void SetPcmPlayer(const char *pcm_filename, int freq, int channels, SDL_AudioFormat format);

private:
    const char *yuv_filename_; // YUV文件名
    int width_, height_;       // 视频宽高
    int fps_;                  // 帧率

    const char *pcm_filename_; // PCM文件名
    int freq_;                 // 采样率
    int channels_;             // 声道数
    SDL_AudioFormat format_;   // 音频格式
};

#endif // PCM_YUV_PLAYER_H