/**
 * @file demuc_to_h264_aac.h
 * @author 晴天天
 * @brief 解复用音视频，将视频存为H264，音频存为AAC
 * @version 1.1
 * @date 2024-06-20
 */

#ifndef DEMUC_TO_H264_AAC_H
#define DEMUC_TO_H264_AAC_H

#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavutil/imgutils.h> // 在处理视频帧时使用
}

#include "get_path.h"

class DemucMedia
{
public:
    DemucMedia();
    DemucMedia(const char *input);
    ~DemucMedia();

public:
    // 运行解复用，会调用以下成员函数
    void run();

private:
    // 打开输入文件
    bool OpenInput();

    // 获取音视频流信息
    bool GetInfo();
    // 解复用音视频
    bool DemuxVideoAudio();
    // 初始化音视频流参数
    bool InitStreamParameters(AVBSFContext **absctx);

private:
    const char *input_;       // 输入文件路径(mp4)
    const char *output_h264_; // H264文件路径
    const char *output_aac_;  // AAC文件路径

    AVFormatContext *format_ctx_; // 格式上下文

    int video_index_; // 视频流
    int audio_index_; // 音频流
};

#endif // DEMUC_TO_H264_AAC_H