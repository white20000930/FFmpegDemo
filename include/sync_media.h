/**
 * @file sync_media.h
 * @author 晴天天
 * @brief 同步音视频并播放
 * @version 0.1
 * @date 2024-06-19
 */

#ifndef SYNC_MEDIA_H
#define SYNC_MEDIA_H
#include "packet_queue.h"

#include <iostream>
#include <stdio.h>

#include <SDL.h>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
}

#include "get_path.h"
// 音频播放参数
struct AudioParameters
{
    SDL_AudioSpec audio_spec; // SDL音频参数
    Uint8 *chunk = nullptr;   // 音频数据缓冲区
    Uint8 *pos = nullptr;     // 音频数据指针
    Uint32 len;               // 音频数据长度

    double pts;           // 时间戳
    AVRational time_base; // 时间基

    const AVCodec *codec = nullptr;      // 音频解码器
    AVCodecContext *codec_ctx = nullptr; // 音频解码器上下文

    int index = -1; // 音频流索引
};

// 视频播放参数
struct VideoParameters
{
    SDL_Window *window = nullptr;       // 窗口
    SDL_Renderer *renderer = nullptr;   // 渲染器
    SDL_Texture *texture = nullptr;     // 纹理
    SDL_Rect rect = {0, 0, 1920, 1080}; // 渲染显示区域

    AVRational time_base; // 时间基

    double pts; // 时间戳

    const AVCodec *codec = nullptr;      // 视频解码器
    AVCodecContext *codec_ctx = nullptr; // 视频解码器上下文

    int index = -1; // 视频流索引
};

// SyncMedia 类用于同步音频和视频
class SyncMedia
{
public:
    SyncMedia();
    SyncMedia(const char *input);
    ~SyncMedia();

public:
    // 播放音视频
    void StartPlayThread();

private:
    // 初始化SyncMedia，包括打开文件、查找媒体流、初始化解码器、初始化SDL
    void InitSyncMedia();

    // 打开文件
    bool OpenFile();
    // 查找媒体流
    bool FindMediaStreams();
    // 初始化解码器
    bool InitializeDecoders();
    // 初始化 SDL
    void InitializeSDL();

    // 关闭 SDL
    void closeSDL();

    // 周期循环检查音频数据包队列是否为空
    static int CheckQueueThread(void *data);
    // 解复用线程
    static int DemuxThread(void *data);
    // 视频播放线程
    static int VideoPlayThread(void *data);
    // 音频播放线程
    static int AudioPlayThread(void *data);

    // 将 fltp 格式转换为 f32le 格式
    static void FltpToF32le(float *f32le, float *fltp_l, float *fltp_r, int nb_samples, int channels);
    // 填充音频 pcm2
    static void FillAudioPcm2(void *udata, Uint8 *stream, int len);

private:
    const char *input_; // 文件路径

    AVFormatContext *format_ctx_; // 格式上下文

    PacketQueue video_pkt_queue_, audio_pkt_queue_; // 帧队列
    AVPacket *pkt_;                                 // 解码前的数据包
    AVFrame *video_frame_, *audio_frame_;           // 视频帧、音频帧

    AudioParameters audio_parameters_; // 音频播放器参数
    VideoParameters video_parameters_; // 视频播放器参数

    bool video_drop_;        // 是否丢弃视频
    double threshold_;       // 阈值
    int fast_display_count_; // 快速显示次数
    bool quit_;              // 是否退出播放
};
#endif // SYNC_MEDIA_H