
#include "sync_media.h"
#include "demuc_to_h264_aac.h"
#include "decode_to_yuv_pcm.h"
#include "pcm_yuv_player.h"

#include <iostream>

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        std::cout << "输入数字：\n"
                  << "1. 获取音视频信息，解复用为H264和AAC\n"
                  << "2. 解码H264为YUV，AAC为PCM\n"
                  << "3. 单独播放YUV\n"
                  << "4. 单独PCM\n"
                  << "5. 同步播放音视频\n";
        return 0;
    }

    DemucMedia demuc_media;
    DecodeMedia decode_media;
    PcmYuvPlayer pcm_yuv_player;
    SyncMedia sync_media;

    int arg = std::stoi(argv[1]);
    switch (arg)
    {
    case 1:

        demuc_media.run();
        break;
    case 2:
        decode_media.DecodeVideo();
        decode_media.DecodeAudio();
        break;
    case 3:
        pcm_yuv_player.PlayYUV();
        break;
    case 4:
        pcm_yuv_player.PlayPCM();
        break;
    case 5:
        sync_media.StartPlayThread();
    default:
        break;
    }

    return 0;
}