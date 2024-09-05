/**
 * @file packet_queue.h
 * @author 晴天天
 * @brief 音视频数据包队列
 * @version 1.1
 * @date 2024-06-19
 */

#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

extern "C"
{
#include <libavcodec/avcodec.h>
}

class PacketQueue
{
public:
    PacketQueue() = default;
    ~PacketQueue()
    {
        std::lock_guard<std::mutex> lock(mutex);
        while (!queue.empty())
        {
            AVPacket *packet = queue.front();
            queue.pop();
            av_packet_free(&packet);
        }
    }

public:
    void push(AVPacket *packet)
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(packet);
        cond.notify_one();
    }

    AVPacket *pop()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this]
                  { return !queue.empty(); });
        AVPacket *packet = queue.front();
        queue.pop();
        return packet;
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.empty();
    }

    int size()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }

private:
    std::queue<AVPacket *> queue;
    std::mutex mutex;
    std::condition_variable cond;
};

#endif // PACKETQUEUE_H