/**
 * @file decode_to_yuv_pcm.h
 * @author 晴天天
 * @brief 解码音视频，将音频存为PCM，视频存为YUV
 * @version 1.1
 * @date 2024-06-20
 */

#ifndef DECODE_TO_YUV_PCM_H
#define DECODE_TO_YUV_PCM_H

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

extern "C"
{
#include <libavformat/avformat.h>     // 文件格式和编解码器
#include <libavcodec/avcodec.h>       // 编解码（压缩/解压缩）音频/视频数据
#include <libavcodec/bsf.h>           // 比特流过滤器
#include <libavutil/imgutils.h>       // 在处理视频帧时使用
#include <libavutil/channel_layout.h> // 处理音频通道布局
}

#include "get_path.h"

class DecodeMedia
{
public:
    DecodeMedia();
    ~DecodeMedia();

public:
    // 解码视频, .h264 -> .yuv
    void DecodeVideo();
    // 解码音频, .aac -> .pcm
    void DecodeAudio();

private:
    // 打开解码器上下文
    void OpenCodecContext(AVCodecID codec_id, AVCodecContext **dec_ctx, AVCodecParserContext **parser);
    // 解码数据
    void DecodeData(AVCodecContext *dec_ctx, AVCodecParserContext *parser, FILE *in_file, FILE *out_file);
    // 解码数据包
    int DecodePacket(AVCodecContext *dec_ctx, FILE *outfile);

    void write_video_frame(AVFrame *frame, FILE *outfile);

    int write_audio_frame(AVFrame *frame, FILE *outfile);

private:
    int inbuf_size_;    // 输入缓冲区大小
    int refill_thresh_; // 重填阈值

    const char *input_h264_; // H264文件路径
    const char *input_aac_;  // AAC文件路径

    const char *output_pcm_; // PCM文件路径
    const char *output_yuv_; // YUV文件路径

    AVCodecContext *video_codec_ctx_; // 视频解码器上下文
    AVCodecContext *audio_codec_ctx_; // 音频解码器上下文

    AVCodecParserContext *video_parser_; // 解码器上下文
    AVCodecParserContext *audio_parser_; // 解码器上下文

    AVPacket *pkt;  // 解码数据包
    AVFrame *frame; // 解码帧
};
#endif // DECODE_TO_YUV_PCM_H