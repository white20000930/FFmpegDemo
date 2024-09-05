#include "demuc_to_h264_aac.h"

DemucMedia::DemucMedia()
{
    input_ = GetAbsolutePath("/files/offlinevideo.mp4");

    output_h264_ = GetAbsolutePath("/files/out_h264.h264");
    output_aac_ = GetAbsolutePath("/files/out_aac.aac");

    format_ctx_ = nullptr;

    video_index_ = -1;
    audio_index_ = -1;
}

DemucMedia::DemucMedia(const char *input)
{
    input_ = input;

    output_h264_ = GetAbsolutePath("/files/out_h264.h264");
    output_aac_ = GetAbsolutePath("/files/out_aac.aac");

    format_ctx_ = nullptr;

    video_index_ = -1;
    audio_index_ = -1;
}

DemucMedia::~DemucMedia()
{
    if (format_ctx_)
    {
        avformat_close_input(&format_ctx_);
        format_ctx_ = nullptr;
    }

    delete[] input_;
    delete[] output_h264_;
    delete[] output_aac_;
}

void DemucMedia::run()
{
    OpenInput();
    GetInfo();
    DemuxVideoAudio();
}

bool DemucMedia::OpenInput()
{
    // 打开视频文件
    if (avformat_open_input(&format_ctx_, input_, nullptr, nullptr) != 0)
    {
        std::cout << "无法打开文件\n";
        return false;
    }

    // 检索流信息
    if (avformat_find_stream_info(format_ctx_, nullptr) < 0)
    {
        std::cout << "无法获取流信息\n";
        return false;
    }

    return true;
}

bool DemucMedia::GetInfo()
{
    std::cout << "视频文件信息" << std::endl;

    double seconds = (double)format_ctx_->duration / 1000000; // 秒
    std::cout << "视频时长: " << seconds << "秒" << std::endl;

    // 遍历所有流
    for (unsigned int i = 0; i < format_ctx_->nb_streams; i++) // nb_streams 流的数量
    {
        std::cout << "流: " << i << std::endl;

        AVStream *stream = format_ctx_->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;

        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            std::cout << "视频编码方式: " << avcodec_get_name(codecpar->codec_id) << std::endl;
            std::cout << "分辨率: " << codecpar->width << "x" << codecpar->height << std::endl;
            std::cout << "码率: " << codecpar->bit_rate << std::endl; // 每秒传送bit数
            std::cout << "颜色格式: " << av_get_pix_fmt_name((AVPixelFormat)codecpar->format) << std::endl;

            video_index_ = i;
        }
        else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            std::cout << "音频编码方式: " << avcodec_get_name(codecpar->codec_id) << std::endl;
            std::cout << "声道数: " << codecpar->ch_layout.nb_channels << std::endl;
            std::cout << "音频采样率: " << codecpar->sample_rate << std::endl;

            audio_index_ = i;
        }
    }
    return true;
}

const int sampling_frequencies[] = {
    96000, // 0x0
    88200, // 0x1
    64000, // 0x2
    48000, // 0x3
    44100, // 0x4
    32000, // 0x5
    24000, // 0x6
    22050, // 0x7
    16000, // 0x8
    12000, // 0x9
    11025, // 0xa
    8000   // 0xb
    // 0xc d e f是保留的
};

int adts_header(char *const p_adts_header, const int data_length,
                const int profile, const int samplerate,
                const int channels)
{

    int sampling_frequency_index = 3; // 默认使用48000hz
    int adtsLen = data_length + 7;

    int frequencies_size = sizeof(sampling_frequencies) / sizeof(sampling_frequencies[0]);
    int i = 0;
    for (i = 0; i < frequencies_size; i++)
    {
        if (sampling_frequencies[i] == samplerate)
        {
            sampling_frequency_index = i;
            break;
        }
    }
    if (i >= frequencies_size)
    {
        printf("unsupport samplerate:%d\n", samplerate);
        return -1;
    }

    p_adts_header[0] = 0xff;      // syncword:0xfff                          高8bits
    p_adts_header[1] = 0xf0;      // syncword:0xfff                          低4bits
    p_adts_header[1] |= (0 << 3); // MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
    p_adts_header[1] |= (0 << 1); // Layer:0                                 2bits
    p_adts_header[1] |= 1;        // protection absent:1                     1bit

    p_adts_header[2] = (profile) << 6;                          // profile:profile               2bits
    p_adts_header[2] |= (sampling_frequency_index & 0x0f) << 2; // sampling frequency index:sampling_frequency_index  4bits
    p_adts_header[2] |= (0 << 1);                               // private bit:0                   1bit
    p_adts_header[2] |= (channels & 0x04) >> 2;                 // channel configuration:channels  高1bit

    p_adts_header[3] = (channels & 0x03) << 6;      // channel configuration:channels 低2bits
    p_adts_header[3] |= (0 << 5);                   // original：0                1bit
    p_adts_header[3] |= (0 << 4);                   // home：0                    1bit
    p_adts_header[3] |= (0 << 3);                   // copyright id bit：0        1bit
    p_adts_header[3] |= (0 << 2);                   // copyright id start：0      1bit
    p_adts_header[3] |= ((adtsLen & 0x1800) >> 11); // frame length：value   高2bits

    p_adts_header[4] = (uint8_t)((adtsLen & 0x7f8) >> 3); // frame length:value    中间8bits
    p_adts_header[5] = (uint8_t)((adtsLen & 0x7) << 5);   // frame length:value    低3bits
    p_adts_header[5] |= 0x1f;                             // buffer fullness:0x7ff 高5bits
    p_adts_header[6] = 0xfc;                              // ‭11111100‬       //buffer fullness:0x7ff 低6bits
    // number_of_raw_data_blocks_in_frame：
    //    表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧。
    return 0;
}

bool DemucMedia::DemuxVideoAudio()
{
    AVBSFContext *absctx = NULL; // 比特流过滤器上下文
    InitStreamParameters(&absctx);

    FILE *video_outfile = fopen(output_h264_, "wb");
    FILE *audio_outfile = fopen(output_aac_, "wb");

    AVPacket *pkt = av_packet_alloc();
    if (!pkt)
    {
        std::cout << "无法分配数据包" << std::endl;
        return -1;
    }

    while (av_read_frame(format_ctx_, pkt) >= 0)
    {
        if (pkt->stream_index == video_index_)
        {
            // 发送数据包给过滤线程
            int ret = av_bsf_send_packet(absctx, pkt);
            if (ret < 0)
            {
                std::cout << "av_bsf_send_packet failed" << std::endl;
                break;
            }
            while (1)
            {
                ret = av_bsf_receive_packet(absctx, pkt);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    break; // 需要更多数据
                }

                // 将过滤后的数据写入文件
                fwrite(pkt->data, 1, pkt->size, video_outfile);

                av_packet_unref(pkt);
            }
            av_packet_unref(pkt);
        }
        else if (pkt->stream_index == audio_index_)
        {
            // 写入头
            AVCodecParameters *codecpar = format_ctx_->streams[audio_index_]->codecpar;
            char adts_header_buf[7] = {0};
            adts_header(adts_header_buf, pkt->size,
                        codecpar->profile,
                        codecpar->sample_rate,
                        codecpar->ch_layout.nb_channels);
            fwrite(adts_header_buf, 1, 7, audio_outfile);

            // 写入数据
            fwrite(pkt->data, 1, pkt->size, audio_outfile);

            av_packet_unref(pkt);
        }
    }

    // 释放
    av_bsf_free(&absctx);
    av_packet_free(&pkt);

    fclose(video_outfile);
    fclose(audio_outfile);

    std::cout << "解复用完成" << std::endl;
    return true;
}

bool DemucMedia::InitStreamParameters(AVBSFContext **absctx)
{
    AVCodecParameters *codecpar = format_ctx_->streams[video_index_]->codecpar;

    // 过滤器
    const AVBitStreamFilter *absfilter = av_bsf_get_by_name("h264_mp4toannexb");
    if (!absfilter)
    {
        std::cout << "get bsFilter failed! " << std::endl;
        return false;
    }

    if (av_bsf_alloc(absfilter, absctx) < 0)
    {
        std::cout << "av_bsf_alloc failed" << std::endl;
        return false;
    }

    // 流信息复制进上下文中
    avcodec_parameters_copy((*absctx)->par_in, codecpar);
    if (av_bsf_init((*absctx)) < 0)
    {
        std::cout << "av_bsf_init失败 " << std::endl;
        return false;
    }

    return true;
}
