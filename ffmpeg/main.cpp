#include "ffmpeg.hpp"


int main(int argc, char** argv)
{
    const char* url = "rtsp://admin:admin@192.168.10.80/streaming/channels/stream1";
    if (video_decode_example(url) != 0)
        return 1;

    return 0;
}