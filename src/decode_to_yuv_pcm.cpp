#include "decode_to_yuv_pcm.h"

DecodeMedia::DecodeMedia()
{
    inbuf_size_ = 20480;
    refill_thresh_ = 4096;

    input_h264_ = GetAbsolutePath("/files/out_h264.h264");
    input_aac_ = GetAbsolutePath("/files/out_aac.aac");

    output_yuv_ = GetAbsolutePath("/files/out_yuv.yuv");
    output_pcm_ = GetAbsolutePath("/files/out_pcm.pcm");

    video_codec_ctx_ = nullptr;
    audio_codec_ctx_ = nullptr;

    video_parser_ = nullptr;
    audio_parser_ = nullptr;

    OpenCodecContext(AV_CODEC_ID_H264, &video_codec_ctx_, &video_parser_);
    OpenCodecContext(AV_CODEC_ID_AAC, &audio_codec_ctx_, &audio_parser_);
}

DecodeMedia::~DecodeMedia()
{
    delete[] input_h264_;
    delete[] input_aac_;
    delete[] output_yuv_;
    delete[] output_pcm_;

    avcodec_free_context(&video_codec_ctx_);
    av_parser_close(video_parser_);
    avcodec_free_context(&audio_codec_ctx_);
    av_parser_close(audio_parser_);
}

void DecodeMedia::DecodeVideo()
{
    std::cout << "开始解码视频" << std::endl;
    FILE *in_file = fopen(input_h264_, "rb");
    FILE *out_file = fopen(output_yuv_, "wb");

    pkt = av_packet_alloc();
    frame = av_frame_alloc();

    DecodeData(video_codec_ctx_, video_parser_, in_file, out_file);

    av_packet_free(&pkt);
    av_frame_free(&frame);

    fclose(in_file);
    fclose(out_file);

    std::cout << "解码视频完成，文件位于 " << output_yuv_ << std::endl;
}

void DecodeMedia::DecodeAudio()
{
    std::cout << "开始解码音频" << std::endl;
    FILE *in_file = fopen(input_aac_, "rb");
    FILE *out_file = fopen(output_pcm_, "wb");

    pkt = av_packet_alloc();
    frame = av_frame_alloc();

    DecodeData(audio_codec_ctx_, audio_parser_, in_file, out_file);

    av_packet_free(&pkt);
    av_frame_free(&frame);

    fclose(in_file);
    fclose(out_file);

    std::cout << "解码音频完成，文件位于 " << output_pcm_ << std::endl;
}

void DecodeMedia::OpenCodecContext(AVCodecID codec_id, AVCodecContext **dec_ctx, AVCodecParserContext **parser)
{
    // 查找解码器
    const AVCodec *codec = avcodec_find_decoder(codec_id);
    if (!codec)
    {
        std::cout << "未找到解码器\n";
        return;
    }

    // 分配解码器上下文
    *dec_ctx = avcodec_alloc_context3(codec);
    if (!*dec_ctx)
    {
        std::cout << "无法分配解码器上下文\n";
        return;
    }

    // 将解码器和解码器上下文进行关联
    if (avcodec_open2(*dec_ctx, codec, NULL) < 0)
    {
        std::cout << "无法打开解码器\n";
        return;
    }

    // 获取裸流的解析器上下文
    *parser = av_parser_init(codec->id);
    if (!parser)
    {
        std::cout << "无法分配解析器上下文\n";
        return;
    }
}

void DecodeMedia::DecodeData(AVCodecContext *dec_ctx, AVCodecParserContext *parser, FILE *in_file, FILE *out_file)
{
    // 读取文件数据
    uint8_t inbuf[inbuf_size_ + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data = inbuf;
    size_t data_size = fread(inbuf, 1, inbuf_size_, in_file);

    while (data_size > 0)
    {
        int parsed_size = av_parser_parse2(parser, dec_ctx, &pkt->data, &pkt->size,
                                           data, data_size,
                                           AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (parsed_size < 0)
        {
            std::cout << "解析数据包失败，错误代码：" << parsed_size << std::endl;
        }

        data += parsed_size;      // 跳过已经解析的数据
        data_size -= parsed_size; // 减去已经解析的数据

        if (pkt->size)
        {
            DecodePacket(dec_ctx, out_file);
        }

        if (data_size < refill_thresh_) // 缓存不足时，从文件中读取数据
        {
            memmove(inbuf, data, data_size); // 将剩余数据移到缓存头部
            data = inbuf;
            int len = fread(data + data_size, 1, inbuf_size_ - data_size, in_file);
            if (len > 0)
                data_size += len;
        }
    }

    // 刷新解码器
    pkt->data = NULL;
    pkt->size = 0;
    DecodePacket(dec_ctx, out_file);
}

int DecodeMedia::DecodePacket(AVCodecContext *dec_ctx, FILE *outfile)
{
    int ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0)
    {
        std::cout << "发送数据包到解码器失败，错误代码：" << ret << std::endl;
        return ret;
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN))
        {
            continue; // 需要更多数据
        }
        else if (ret < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
            // std::cout << "从解码器接收帧失败，错误代码：" << ret << ", 错误消息：" << errbuf << std::endl;
            return ret;
        }

        if (dec_ctx->codec->type == AVMEDIA_TYPE_VIDEO)
        {
            write_video_frame(frame, outfile);
        }
        else if (dec_ctx->codec->type == AVMEDIA_TYPE_AUDIO)
        {
            if (write_audio_frame(frame, outfile) < 0)
                return -1;
        }
    }
    av_packet_unref(pkt);
    return 0;
}

void DecodeMedia::write_video_frame(AVFrame *frame, FILE *outfile)
{
    fwrite(frame->data[0], 1, frame->width * frame->height, outfile);         // Y
    fwrite(frame->data[1], 1, (frame->width) * (frame->height) / 4, outfile); // U:宽高均是Y的一半
    fwrite(frame->data[2], 1, (frame->width) * (frame->height) / 4, outfile); // V
}

int DecodeMedia::write_audio_frame(AVFrame *frame, FILE *outfile)
{
    int data_size = av_get_bytes_per_sample(audio_codec_ctx_->sample_fmt);
    if (data_size < 0)
    {
        return -1;
    }
    for (int i = 0; i < frame->nb_samples; i++)
    {
        for (int ch = 0; ch < audio_codec_ctx_->ch_layout.nb_channels; ch++) // 交错的方式写入, 大部分float的格式输出
            fwrite(frame->data[ch] + data_size * i, 1, data_size, outfile);
    }
    return 0;
}
