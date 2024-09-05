#include "pcm_yuv_player.h"

PcmYuvPlayer::PcmYuvPlayer()
{
    yuv_filename_ = GetAbsolutePath("/files/out_yuv.yuv");
    width_ = 1920;
    height_ = 1080;
    fps_ = 30;

    pcm_filename_ = GetAbsolutePath("/files/out_pcm.pcm");
    freq_ = 48000;
    channels_ = 2;
    format_ = AUDIO_F32LSB;
}

PcmYuvPlayer::~PcmYuvPlayer()
{
    delete[] yuv_filename_;
    delete[] pcm_filename_;
}

void PcmYuvPlayer::PlayYUV()
{

    SDL_Window *screen = SDL_CreateWindow("YUV Player",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          width_, height_,
                                          SDL_WINDOW_OPENGL);
    if (!screen)
    {
        printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
        return;
    }

    SDL_Renderer *sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
    SDL_Texture *sdlTexture = SDL_CreateTexture(sdlRenderer,
                                                SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
                                                width_, height_);

    FILE *file = fopen(yuv_filename_, "rb");
    if (!file)
    {
        printf("cannot open file %s\n", yuv_filename_);
        return;
    }

    SDL_Rect sdlRect;
    SDL_Event event;
    uint8_t *buffer = (uint8_t *)malloc(width_ * height_ * 12 / 8); // for YUV420

    double currentPTS = 0;
    double lastPTS = 0;
    while (1)
    {
        if (fread(buffer, 1, width_ * height_ * 12 / 8, file) <= 0)
        {
            printf("end of file\n");
            break;
        }

        SDL_UpdateTexture(sdlTexture, NULL, buffer, width_);

        sdlRect.x = 0;
        sdlRect.y = 0;
        sdlRect.w = width_;
        sdlRect.h = height_;

        // 清除当前渲染目标中的内容，准备绘制新的一帧
        SDL_RenderClear(sdlRenderer);

        // 将纹理（视频帧）复制到渲染目标（窗口）上
        SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);

        // 更新窗口的内容，将新绘制的一帧显示出来
        SDL_RenderPresent(sdlRenderer);

        // 根据视频的帧率
        SDL_Delay(1000 / fps_);

        //  处理窗口的事件，比如窗口关闭、键盘输入等
        SDL_PollEvent(&event);

        switch (event.type)
        {
        case SDL_QUIT:
            goto end;
        default:
            break;
        }
    }

end:
    free(buffer);
    SDL_DestroyTexture(sdlTexture);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(screen);
    SDL_Quit();
}

void PcmYuvPlayer::PlayPCM()
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return;
    }

    FILE *file = fopen(pcm_filename_, "rb");
    if (!file)
    {
        printf("cannot open file %s\n", pcm_filename_);
        return;
    }

    SDL_AudioSpec wavSpec;
    wavSpec.freq = freq_;
    wavSpec.format = format_;
    wavSpec.channels = channels_;
    wavSpec.samples = 1024;
    wavSpec.callback = NULL;

    SDL_AudioDeviceID deviceID;
    if ((deviceID = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE)) < 2)
    {
        std::cout << "SDL_OpenAudioDevice with error deviceID : " << deviceID << std::endl;
        return;
    }

    Uint32 buffer_size = 4096;
    char *buffer = (char *)malloc(buffer_size);
    SDL_PauseAudioDevice(deviceID, 0);
    Uint32 total_bytes_read = 0;
    while (true)
    {
        size_t bytes_read = fread(buffer, 1, buffer_size, file);
        if (bytes_read == (size_t)-1)
        {
            std::cout << "fread error: " << strerror(errno) << std::endl;
            break;
        }
        total_bytes_read += bytes_read;
        if (bytes_read != buffer_size)
        {
            // std::cout << "end of file. Total bytes read: " << total_bytes_read << std::endl;
            break;
        }
        if (SDL_QueueAudio(deviceID, buffer, buffer_size) < 0)
        {
            std::cout << "SDL_QueueAudio error: " << SDL_GetError() << std::endl;
            break;
        }
    }
    SDL_Delay(1000 * 10); // 暂停，等待播放完成
    SDL_CloseAudio();
    SDL_Quit();
    fclose(file);
    free(buffer);
}

void PcmYuvPlayer::SetYuvPlayer(const char *yuv_filename)
{
    yuv_filename_ = yuv_filename;
    width_ = 1920;
    height_ = 1080;
    fps_ = 30;
}

void PcmYuvPlayer::SetYuvPlayer(const char *yuv_filename, int width, int height, int fps)
{

    yuv_filename_ = yuv_filename;
    width_ = width;
    height_ = height;
    fps_ = fps;
}

void PcmYuvPlayer::SetPcmPlayer(const char *pcm_filename)
{
    pcm_filename_ = pcm_filename;
    freq_ = 48000;
    channels_ = 2;
    format_ = AUDIO_F32LSB;
}

void PcmYuvPlayer::SetPcmPlayer(const char *pcm_filename, int freq, int channels, SDL_AudioFormat format)
{
    pcm_filename_ = pcm_filename;
    freq_ = freq;
    channels_ = channels;
    format_ = format;
}
