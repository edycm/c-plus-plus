/*
 * Copyright (c) 2015 Ludmila Glinskih
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

 /**
  * H264 codec test.
  */
#ifndef _FFMPEG_HPP_
#define _FFMPEG_HPP_

extern "C" {
#include "libavutil/adler32.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libavutil/timestamp.h"
}
#include "opencv2/opencv.hpp"
#include "opencv2/imgcodecs/legacy/constants_c.h"

static unsigned char header[54] = {
    0x42, 0x4d,     //WORD  bfType----------- [0,1]   
    0, 0, 0, 0,     //DWORD bfSize----------- [2,3,4,5]   
    0, 0,           //WORD  bfReserved1------ [6,7]   
    0, 0,           //WORD  bfReserved2------ [8,9]   
    54, 0, 0, 0,    //WORD  bfOffBits-------- [10,11,12,13]   

    40, 0, 0, 0,    //DWORD biSize----------- [14,15,16,17]   
    0, 0, 0, 0,     //LONG  biWidth---------- [18,19,20,21]    
    0, 0, 0, 0,     //LONG  biHeight--------- [22,23,24,25]    
    1, 0,           //WORD  biplanes--------- [26,27]   
    24, 0,          //WORD  biCount---------- [28,29]   
    0, 0, 0, 0,     //DWORD biCompression---- [30,31,32,33]   
    0, 0, 0, 0,     //DWORD biSizeImage------ [34,35,36,37]   
    0, 0, 0, 0,     //LONG  biXPelsPerMeter-- [38,39,40,41]   
    0, 0, 0, 0,     //LONG  biYPelsPerMeter-- [42,43,44,45]    
    0, 0, 0, 0,     //DWORD biClrUsed-------- [46,47,48,49]    
    0, 0, 0, 0      //DWORD biClrImportant--- [50,51,52,53]   
};


static void save_frame(AVFrame* fr, int width, int height, int index)
{
    FILE* fd;
    char file_name[32];
    int y;

    sprintf(file_name, "frame%d.ppm", index);
    fd = fopen(file_name, "wb");
    if (!fd) {
        printf("open file %s failed\n", file_name);
        return;
    }

    //write header
    fprintf(fd, "P6 %d %d 255", width, height);

    //write pixel data
    for (y = 0; y < height; ++y) 
    {
        fwrite(fr->data[0] + y * fr->linesize[0], 1, width * 3, fd);
    }

    fclose(fd);
    return;
}

static void save_frame(uint8_t *yuv_frame, int width, int height, int index)
{
    int uIndex = width * height;
    int vIndex = uIndex + ((width * height) >> 2);
    int gIndex = width * height;
    int bIndex = gIndex * 2;
    int temp, x, y, i, j;
    long file_size;
    FILE* fd;
    char file_name[32] = { 0 };
    uint8_t* rgb_frame = (uint8_t*)malloc(width * height * 3 * sizeof(uint8_t));
    memset(rgb_frame, 0, width * height * 3);
    uint8_t* data = (uint8_t*)malloc(width * height * 3 * sizeof(uint8_t));
    memset(data, 0, width * height * 3);

    double YUV2RGB_CONVERT_MATRIX[3][3] = { { 1, 0, 1.4022 }, { 1, -0.3456, -0.7145 }, { 1, 1.771, 0 } };
    file_size = (long)width * (long)height * 3 + 54;


    header[2] = (unsigned char)(file_size & 0x000000ff);
    header[3] = (file_size >> 8) & 0x000000ff;
    header[4] = (file_size >> 16) & 0x000000ff;
    header[5] = (file_size >> 24) & 0x000000ff;

    header[18] = width & 0x000000ff;
    header[19] = (width >> 8) & 0x000000ff;
    header[20] = (width >> 16) & 0x000000ff;
    header[21] = (width >> 24) & 0x000000ff;

    header[22] = height & 0x000000ff;
    header[23] = (height >> 8) & 0x000000ff;
    header[24] = (height >> 16) & 0x000000ff;
    header[25] = (height >> 24) & 0x000000ff;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            // R分量        
            temp = (int)(yuv_frame[y * width + x] + (yuv_frame[vIndex + (y / 2) * (width / 2) + x / 2] - 128) * YUV2RGB_CONVERT_MATRIX[0][2]);
            rgb_frame[y * width + x] = (uint8_t)(temp < 0 ? 0 : (temp > 255 ? 255 : temp));
            // G分量       
            temp = (int)(yuv_frame[y * width + x] + (yuv_frame[uIndex + (y / 2) * (width / 2) + x / 2] - 128) * YUV2RGB_CONVERT_MATRIX[1][1] + (yuv_frame[vIndex + (y / 2) * (width / 2) + x / 2] - 128) * YUV2RGB_CONVERT_MATRIX[1][2]);
            rgb_frame[gIndex + y * width + x] = (uint8_t)(temp < 0 ? 0 : (temp > 255 ? 255 : temp));
            // B分量             
            temp = (int)(yuv_frame[y * width + x] + (yuv_frame[uIndex + (y / 2) * (width / 2) + x / 2] - 128) * YUV2RGB_CONVERT_MATRIX[2][1]);
            rgb_frame[bIndex + y * width + x] = (uint8_t)(temp < 0 ? 0 : (temp > 255 ? 255 : temp));
        }
    }

    for (y = height - 1, j = 0; y >= 0; y--, j++)
    {
        for (x = 0, i = 0; x < width; x++)
        {
            data[y * width * 3 + i++] = rgb_frame[bIndex + j * width + x];
            // B                    

            data[y * width * 3 + i++] = rgb_frame[gIndex + j * width + x];
            // G          

            data[y * width * 3 + i++] = rgb_frame[j * width + x];
            // R          
        }
    }

    sprintf(file_name, "frame%d.bmp", index);
    fd = fopen(file_name, "wb");
    if (!fd) {
        printf("open file %s failed\n", file_name);
        return;
    }
    
    fwrite(header, sizeof(unsigned char), 54, fd);
    fwrite(data, sizeof(unsigned char), width * height * 3, fd);
    free(data);
    free(rgb_frame);
    fclose(fd);
}

static void save_frame(uint8_t* buf, int size, int index)
{
    char file_name[20] = { 0 };
#if 0
    cv::Mat image;
    image = cv::imdecode(cv::Mat(1, size, CV_8UC1, buf), CV_LOAD_IMAGE_COLOR);
    if (!image.data)
    {
        printf("imdecode error\n");
        return;
    }
    printf("cols: %d, rows: %d\n", image.cols, image.rows);
#endif
    printf("save frame\n");
    sprintf(file_name, "frame%d.jpeg", index);
    FILE* fd = fopen(file_name, "wb");
    if (!fd) {
        printf("open file %s failed\n", file_name);
        return;
    }
    fwrite(buf, 1, size, fd);
    fclose(fd);
    //cv::imwrite(file_name, image);

    return;
}

static int video_decode_example(const char* input_filename)
{
    AVCodec* codec = NULL;
    AVCodecContext* ctx = NULL;
    AVCodecParameters* origin_par = NULL;
    AVFrame* fr = NULL;
    uint8_t* byte_buffer = NULL;
    AVPacket* pkt;
    AVFormatContext* fmt_ctx = NULL;
    int number_of_written_bytes;
    int video_stream;
    int byte_buffer_size;
    int i = 0;
    int result;

    int fps = 0;
    bool is_init = false;
    int size = 0;
    int index = 0;
    uint8_t* out_buffer = NULL;
    AVFrame* fr_rgb = NULL;
    SwsContext* sws_context = NULL;

    result = avformat_open_input(&fmt_ctx, input_filename, NULL, NULL);
    if (result < 0) {
        av_log(NULL, AV_LOG_ERROR, "Can't open file\n");
        return result;
    }

    result = avformat_find_stream_info(fmt_ctx, NULL);
    if (result < 0) {
        av_log(NULL, AV_LOG_ERROR, "Can't get stream info\n");
        return result;
    }

    video_stream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream < 0) {
        av_log(NULL, AV_LOG_ERROR, "Can't find video stream in input file\n");
        return -1;
    }

    origin_par = fmt_ctx->streams[video_stream]->codecpar;

    codec = avcodec_find_decoder(origin_par->codec_id);
    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "Can't find decoder\n");
        return -1;
    }

    ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate decoder context\n");
        return AVERROR(ENOMEM);
    }

    result = avcodec_parameters_to_context(ctx, origin_par);
    if (result) {
        av_log(NULL, AV_LOG_ERROR, "Can't copy decoder context\n");
        return result;
    }

    result = avcodec_open2(ctx, codec, NULL);
    if (result < 0) {
        av_log(ctx, AV_LOG_ERROR, "Can't open decoder\n");
        return result;
    }

    fr = av_frame_alloc();
    if (!fr) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate frame\n");
        return AVERROR(ENOMEM);
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        av_log(NULL, AV_LOG_ERROR, "Cannot allocate packet\n");
        return AVERROR(ENOMEM);
    }

    byte_buffer_size = av_image_get_buffer_size(ctx->pix_fmt, ctx->width, ctx->height, 16);
    byte_buffer = (uint8_t*)av_malloc(byte_buffer_size);
    if (!byte_buffer) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate buffer\n");
        return AVERROR(ENOMEM);
    }

    printf("#tb %d: %d/%d\n", video_stream, fmt_ctx->streams[video_stream]->time_base.num, fmt_ctx->streams[video_stream]->time_base.den);
    i = 0;
    //printf("den: %d, num: %d\n", fmt_ctx->streams[video_stream]->r_frame_rate.den, fmt_ctx->streams[video_stream]->r_frame_rate.num);
    if (fmt_ctx->streams[video_stream]->r_frame_rate.den)
        fps = fmt_ctx->streams[video_stream]->r_frame_rate.num / fmt_ctx->streams[video_stream]->r_frame_rate.den;
    else
        fps = 30;

    printf("ctx->pix_fmt: %d\n", ctx->pix_fmt);

    result = 0;
    while (result >= 0) {
        result = av_read_frame(fmt_ctx, pkt);
        if (result >= 0 && pkt->stream_index != video_stream) {
            av_packet_unref(pkt);
            continue;
        }

        if (result < 0)
            result = avcodec_send_packet(ctx, NULL);
        else {
            if (pkt->pts == AV_NOPTS_VALUE)
                pkt->pts = pkt->dts = i;
            result = avcodec_send_packet(ctx, pkt);
        }
        av_packet_unref(pkt);

        if (result < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error submitting a packet for decoding\n");
            return result;
        }

        while (result >= 0) {
            result = avcodec_receive_frame(ctx, fr);
            if (result == AVERROR_EOF)
                goto finish;
            else if (result == AVERROR(EAGAIN)) {
                result = 0;
                break;
            }
            else if (result < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error decoding frame\n");
                return result;
            }
#if 0          
            //get yuv420 image data
            number_of_written_bytes = av_image_copy_to_buffer(byte_buffer, byte_buffer_size,
                (const uint8_t* const*)fr->data, (const int*)fr->linesize,
                ctx->pix_fmt, ctx->width, ctx->height, 1);
            if (number_of_written_bytes < 0) {
                av_log(NULL, AV_LOG_ERROR, "Can't copy image to buffer\n");
                av_frame_unref(fr);
                return number_of_written_bytes;
            }
            /*
            printf("%d, %d, %d, %8\"PRId64\", %8d, 0x%08lx\n", video_stream,
                fr->pts, fr->pkt_dts, fr->pkt_duration,
                number_of_written_bytes, av_adler32_update(0, (const uint8_t*)byte_buffer, number_of_written_bytes));
            */
            if (index++ % fps == 0) {
                printf("number_of_written_bytes: %d\n", number_of_written_bytes);
                char file_name[20] = { 0 };
                sprintf(file_name, "frame%d.yuv", index);
                FILE *fp = fopen(file_name, "wb");
                if (fp) {
                    fwrite(byte_buffer, 1, number_of_written_bytes, fp);
                    //fwrite(byte_buffer, 1, number_of_written_bytes * 2 / 3, fp);
                    //fwrite(byte_buffer, 1, number_of_written_bytes * 1 / 6, fp);
                    //fwrite(byte_buffer, 1, number_of_written_bytes * 1 / 6, fp);
                    fclose(fp);
                }

                //save_frame(byte_buffer, ctx->width, ctx->height, index);
            }
#else         
            if (!is_init) {
                printf("Init\n");
                fr_rgb = av_frame_alloc();

                sws_context = sws_getContext(ctx->width, ctx->height, ctx->pix_fmt, ctx->width, ctx->height,
                    AV_PIX_FMT_BGR24, SWS_BILINEAR, NULL, NULL, NULL);
                //size = avpicture_get_size(AV_PIX_FMT_RGB32, ctx->width, ctx->height);
                //out_buffer = (uint8_t*)malloc(size * sizeof(uint8_t));
                //avpicture_fill((AVPicture*)fr_rgb, out_buffer, AV_PIX_FMT_RGB32, ctx->width, ctx->height);
                is_init = true;
            }

            if (index++ % fps == 0) {
#if 1
                printf("write image\n");
                cv::Mat image;
                char file_name[20] = { 0 };
                image.create(ctx->height, ctx->width, CV_8UC3);
                int linesize = ctx->width * 3;
                //sws_scale(sws_context, fr->data, fr->linesize, 0, ctx->height, fr_rgb->data, fr_rgb->linesize);
                sws_scale(sws_context, fr->data, fr->linesize, 0, ctx->height, &image.data, &linesize);
                sprintf(file_name, "frame%d.jpeg", index);
                cv::imwrite(file_name, image);
#else
                save_frame(fr_rgb, ctx->width, ctx->height, index);
#endif
            }
#endif
            av_frame_unref(fr);
        }
        i++;
    }

finish:
    av_packet_free(&pkt);
    av_frame_free(&fr);
    if (fr_rgb) {
        av_frame_free(&fr_rgb);
    }
    avformat_close_input(&fmt_ctx);
    avcodec_free_context(&ctx);
    av_freep(&byte_buffer);
    return 0;
}

#endif