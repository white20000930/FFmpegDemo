#include "sync_media.h"

SyncMedia::SyncMedia()
{
    input_ = GetAbsolutePath("/files/offlinevideo.mp4");
    video_drop_ = false;
    threshold_ = 10;
    fast_display_count_ = 0;
    quit_ = false;
}

SyncMedia::SyncMedia(const char *input)
{
    input_ = input;
    video_drop_ = false;
    threshold_ = 10;
    fast_display_count_ = 0;
    quit_ = false;
}

SyncMedia::~SyncMedia()
{
    delete[] input_;
}

void SyncMedia::InitSyncMedia()
{
    OpenFile();
    FindMediaStreams();
    InitializeDecoders();
    InitializeSDL();
}

bool SyncMedia::OpenFile()
{
    if (input_ == nullptr)
    {
        std::cout << "文件路径为空\n";
        return false;
    }

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

bool SyncMedia::FindMediaStreams()
{
    int ret = av_find_best_stream(format_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0)
    {
        std::cout << "找不到最佳视频流\n";
        return false;
    }
    video_parameters_.index = ret;
    video_parameters_.time_base = format_ctx_->streams[video_parameters_.index]->time_base;

    ret = av_find_best_stream(format_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (ret < 0)
    {
        std::cout << "找不到最佳音频流\n";
        return false;
    }
    audio_parameters_.index = ret;
    audio_parameters_.time_base = format_ctx_->streams[audio_parameters_.index]->time_base;
    return true;
}

bool SyncMedia::InitializeDecoders()
{
    // 初始化视频解码器
    AVCodecParameters *video_codecpar = format_ctx_->streams[video_parameters_.index]->codecpar;
    video_parameters_.codec = avcodec_find_decoder(video_codecpar->codec_id);
    if (!video_parameters_.codec)
    {
        std::cout << "无法找到视频解码器\n";
        return -1;
    }
    video_parameters_.codec_ctx = avcodec_alloc_context3(video_parameters_.codec);
    avcodec_parameters_to_context(video_parameters_.codec_ctx, video_codecpar);
    if (avcodec_open2(video_parameters_.codec_ctx, video_parameters_.codec, nullptr))
    {
        std::cout << "无法打开视频解码器\n";
        return -1;
    }

    video_parameters_.rect.w = video_codecpar->width;
    video_parameters_.rect.h = video_codecpar->height;

    // 初始化音频解码器
    AVCodecParameters *audio_codecpar = format_ctx_->streams[audio_parameters_.index]->codecpar;
    audio_parameters_.codec = avcodec_find_decoder(audio_codecpar->codec_id);
    if (!audio_parameters_.codec)
    {
        std::cout << "无法找到音频解码器\n";
        return -1;
    }
    audio_parameters_.codec_ctx = avcodec_alloc_context3(audio_parameters_.codec);
    avcodec_parameters_to_context(audio_parameters_.codec_ctx, audio_codecpar);
    if (avcodec_open2(audio_parameters_.codec_ctx, audio_parameters_.codec, nullptr))
    {
        std::cout << "无法打开音频解码器\n";
        return -1;
    }
    return true;
}

void SyncMedia::InitializeSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        std::cout << "无法初始化SDL\n";
        return;
    }

    // 窗口
    int weight = video_parameters_.rect.w;
    int height = video_parameters_.rect.h;
    video_parameters_.window = SDL_CreateWindow("Simplest Sync Player",
                                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                                weight, height,
                                                SDL_WINDOW_SHOWN);

    // 渲染器
    video_parameters_.renderer = SDL_CreateRenderer(video_parameters_.window, -1, 0);
    if (video_parameters_.renderer == nullptr)
    {
        std::cout << "无法创建渲染器\n";
        return;
    }
    // 纹理
    video_parameters_.texture = SDL_CreateTexture(video_parameters_.renderer,
                                                  SDL_PIXELFORMAT_IYUV,
                                                  SDL_TEXTUREACCESS_STREAMING,
                                                  weight, height);

    // 设置音频播放参数
    audio_parameters_.audio_spec.silence = 0;                                                   // 音频缓冲静音值
    audio_parameters_.audio_spec.format = AUDIO_F32LSB;                                         // 音频数据采样格式
    audio_parameters_.audio_spec.freq = audio_parameters_.codec_ctx->sample_rate;               // 采样率
    audio_parameters_.audio_spec.channels = audio_parameters_.codec_ctx->ch_layout.nb_channels; // 通道数
    audio_parameters_.audio_spec.samples = audio_parameters_.codec_ctx->frame_size;             // 采样点数量
    audio_parameters_.audio_spec.callback = FillAudioPcm2;                                      // 音频播放回调函数
    audio_parameters_.audio_spec.userdata = this;                                               // 用户数据

    if (SDL_OpenAudio(&audio_parameters_.audio_spec, NULL) < 0)
    {
        std::cout << "无法打开音频设备\n";
        return;
    }
    SDL_PauseAudio(0);
}

void SyncMedia::StartPlayThread()
{
    // 初始化
    InitSyncMedia();

    // 创建四个线程，分别用于解复用、视频播放、音频播放、检查队列
    SDL_CreateThread(DemuxThread, "demux", this);
    SDL_CreateThread(VideoPlayThread, "video_play", this);
    SDL_CreateThread(AudioPlayThread, "audio_play", this);
    SDL_CreateThread(CheckQueueThread, "check_queue", this);

    SDL_Event e;
    while (quit_ == false)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit_ = true;
                break;
            }
        }
    }

    closeSDL();

    avcodec_free_context(&video_parameters_.codec_ctx);
    avcodec_free_context(&audio_parameters_.codec_ctx);
    avformat_close_input(&format_ctx_);
}

void SyncMedia::FillAudioPcm2(void *udata, Uint8 *stream, int len)
{
    SyncMedia *sync_media = static_cast<SyncMedia *>(udata);

    SDL_memset(stream, 0, len);

    if (sync_media->audio_parameters_.len == 0)
    {
        return;
    }

    // 确保len不超过音频数据的长度
    len = (len > static_cast<Uint32>(sync_media->audio_parameters_.len)) ? sync_media->audio_parameters_.len : len;

    // 将音频数据混合到stream中
    SDL_MixAudio(stream, sync_media->audio_parameters_.pos, len, SDL_MIX_MAXVOLUME / 2);

    sync_media->audio_parameters_.pos += len;
    sync_media->audio_parameters_.len -= len;
}

int SyncMedia::CheckQueueThread(void *data)
{
    SyncMedia *sync_media = (SyncMedia *)data;

    Uint32 start_time = 0;
    while (true)
    {
        if (sync_media->audio_pkt_queue_.empty())
        {
            if (start_time == 0)
            {
                // 开始计时
                start_time = SDL_GetTicks();
            }
            else if (SDL_GetTicks() - start_time >= 1000)
            {
                // 队列已经连续1秒为空，关闭视频
                sync_media->quit_ = true;
                break;
            }
        }
        else
        {
            // 重置计时器
            start_time = 0;
        }

        // 暂停一段时间
        SDL_Delay(100);
    }

    return 0;
}

int SyncMedia::DemuxThread(void *data)
{
    SyncMedia *sync_media = (SyncMedia *)data;

    AVPacket *pkt = av_packet_alloc();
    while (av_read_frame(sync_media->format_ctx_, pkt) >= 0)
    {
        if (pkt->stream_index == sync_media->video_parameters_.index)
        {
            AVPacket *pkt_copy = av_packet_clone(pkt);
            sync_media->video_pkt_queue_.push(pkt_copy);
        }
        else if (pkt->stream_index == sync_media->audio_parameters_.index)
        {
            AVPacket *pkt_copy = av_packet_clone(pkt);
            sync_media->audio_pkt_queue_.push(pkt_copy);
        }
        else
        {
            av_packet_unref(pkt);
        }
    }

    av_packet_free(&pkt);
    return 0;
}

int SyncMedia::VideoPlayThread(void *data)
{
    SyncMedia *sync_media = (SyncMedia *)data;

    AVPacket *video_pkt = nullptr;
    AVFrame *frame = av_frame_alloc();
    double lastPTS = 0; // 上一帧的pts

    while (true)
    {
        // 从视频帧队列中取出一个 AVPacket
        video_pkt = sync_media->video_pkt_queue_.pop();

        // 发送数据包到解码器
        int ret = avcodec_send_packet(sync_media->video_parameters_.codec_ctx, video_pkt);
        if (ret < 0)
        {
            av_packet_unref(video_pkt);
            std::cout << "Error sending a packet to video decoder: " << av_err2str(ret) << std::endl;
            return -1;
        }

        while (ret >= 0)
        {
            // 接收解码后的帧数据
            ret = avcodec_receive_frame(sync_media->video_parameters_.codec_ctx, frame);

            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                break;
            }
            else if (ret < 0)
            {
                av_frame_unref(frame);
                break;
            }

            // 计算pts
            if (frame->pts != AV_NOPTS_VALUE)
            {
                sync_media->video_parameters_.pts = frame->pts * av_q2d(sync_media->video_parameters_.time_base) * 1000; // 毫秒
            }
            else
            {
                sync_media->video_parameters_.pts = 0;
            }

            // 计算显示时间
            double delay = sync_media->video_parameters_.pts - lastPTS;
            double difference = sync_media->video_parameters_.pts - sync_media->audio_parameters_.pts;

            lastPTS = sync_media->video_parameters_.pts;

            // videodrop 是否跳过视频帧
            if (sync_media->video_drop_ == true)
            {
                if (std::abs(difference) < sync_media->threshold_)
                {
                    sync_media->video_drop_ = false;
                }
                else
                {
                    continue;
                }
            }

            // 音视频同步
            if (difference <= -sync_media->threshold_) // 视频比音频慢
            {
                delay += difference;
                if (difference < -1000 && sync_media->fast_display_count_++ > 10) // 差距较大且已快速显示10次
                {
                    sync_media->video_drop_ = true;
                }
            }
            else if (difference >= sync_media->threshold_) // 视频比音频快
            {
                if (delay < sync_media->threshold_) // delay值较小
                {
                    delay *= 2; // 让视频平缓显示
                }
                else
                {
                    delay += difference; // delay值较大，直接增大delay
                }
            }

            // 设置纹理的数据
            SDL_UpdateYUVTexture(sync_media->video_parameters_.texture, nullptr,
                                 frame->data[0], frame->linesize[0],
                                 frame->data[1], frame->linesize[1],
                                 frame->data[2], frame->linesize[2]);

            // 清空渲染器
            SDL_RenderClear(sync_media->video_parameters_.renderer);
            // 复制纹理到渲染器
            SDL_RenderCopy(sync_media->video_parameters_.renderer, sync_media->video_parameters_.texture,
                           NULL, &sync_media->video_parameters_.rect);
            // 显示到窗口
            SDL_RenderPresent(sync_media->video_parameters_.renderer);
            // 实现延迟
            SDL_Delay((Uint32)delay);
        }

        av_packet_unref(video_pkt);
    }
    av_frame_free(&frame);

    return 0;
}

int SyncMedia::AudioPlayThread(void *data)
{
    SyncMedia *sync_media = (SyncMedia *)data;

    AVPacket *audio_pkt = nullptr;
    AVFrame *frame = av_frame_alloc();
    AVCodecContext *codec_ctx = sync_media->audio_parameters_.codec_ctx; // 音频解码器上下文

    while (true)
    {
        // 从音频帧队列中取出一个 AVPacket
        audio_pkt = sync_media->audio_pkt_queue_.pop();

        int ret = avcodec_send_packet(codec_ctx, audio_pkt);
        if (ret < 0)
        {
            std::cout << "发送音频数据包到解码器失败\n";
            return -1;
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_frame(codec_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            else if (ret < 0)
            {
                std::cout << "从音频解码器接收帧失败\n";
                break;
            }

            // 根据采样格式，获取每个采样所占的字节数
            AVSampleFormat sample_format = codec_ctx->sample_fmt;
            int data_size = av_get_bytes_per_sample(sample_format);
            if (data_size < 0)
            {
                std::cout << "无法获取采样字节数\n";
                break;
            }

            // 为pcm缓冲区分配内存
            int pcm_buffer_size = data_size * frame->nb_samples * codec_ctx->ch_layout.nb_channels;
            uint8_t *pcm_buffer = (uint8_t *)malloc(pcm_buffer_size);
            memset(pcm_buffer, 0, pcm_buffer_size);

            // 转换为 packed 模式
            if (sample_format == AV_SAMPLE_FMT_FLTP)
            {
                FltpToF32le((float *)pcm_buffer, (float *)frame->data[0], (float *)frame->data[1],
                            frame->nb_samples, codec_ctx->ch_layout.nb_channels);
            }

            // 将音频数据填充到音频播放器参数中
            sync_media->audio_parameters_.chunk = pcm_buffer;
            sync_media->audio_parameters_.len = pcm_buffer_size;
            sync_media->audio_parameters_.pos = sync_media->audio_parameters_.chunk;
            sync_media->audio_parameters_.pts = frame->pts * av_q2d(sync_media->audio_parameters_.time_base) * 1000; // ms

            // 等待音频数据播放完毕
            while (sync_media->audio_parameters_.len > 0)
            {
                SDL_Delay(1);
            }

            free(pcm_buffer);
        }
    }

    av_packet_unref(audio_pkt);
    av_frame_free(&frame);

    return 0;
}

void SyncMedia::FltpToF32le(float *f32le, float *fltp_l, float *fltp_r, int nb_samples, int channels)
{
    for (int i = 0; i < nb_samples; i++)
    {
        f32le[i * channels] = fltp_l[i];
        f32le[i * channels + 1] = fltp_r[i];
    }
}

void SyncMedia::closeSDL()
{
    SDL_CloseAudio();

    SDL_DestroyWindow(video_parameters_.window);
    video_parameters_.window = nullptr;

    SDL_DestroyRenderer(video_parameters_.renderer);
    video_parameters_.renderer = nullptr;

    SDL_DestroyTexture(video_parameters_.texture);
    video_parameters_.texture = nullptr;

    SDL_Quit();
}
