/*
ACHTUNG: AI GENERATED!!!

Dependencies:
 - FFmpeg (libavformat, libavcodec, libavutil, libswscale, libavdevice)
 - SDL2

Build example (Linux):
 gcc camera_stream.c -o camera_stream `pkg-config --cflags --libs sdl2` \
    -lavformat -lavcodec -lavutil -lswscale -lavdevice -pthread
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>


//#define SDL_MAIN_HANDLED
//#include <SDL.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavdevice/avdevice.h>

#ifdef _WIN32
#define DEFAULT_INPUT_FORMAT "dshow"
#else
#define DEFAULT_INPUT_FORMAT "v4l2"
#endif

static void log_error(const char *msg, int err) {
    char buf[256];
    av_strerror(err, buf, sizeof(buf));
    fprintf(stderr, "%s: %s\n", msg, buf);
}

static char *choose_device(const char *iformat_name) {
    void *opaque = NULL;
    AVDeviceInfoList *device_list = NULL;
    const AVInputFormat *ifmt = av_find_input_format(iformat_name);
    if (!ifmt) {
        fprintf(stderr, "Could not find input format %s\n", iformat_name);
        return NULL;
    }

    int ret = avdevice_list_input_sources(ifmt, NULL, NULL, &device_list);
    if (ret < 0) {
        log_error("Could not list devices", ret);
        return NULL;
    }

    if (device_list->nb_devices == 0) {
        fprintf(stderr, "No devices found for format %s\n", iformat_name);
        avdevice_free_list_devices(&device_list);
        return NULL;
    }

    printf("Available devices for format %s:\n", iformat_name);
    for (int i = 0; i < device_list->nb_devices; i++) {
        printf("[%d] %s\n", i, device_list->devices[i]->device_name);
        if (device_list->devices[i]->device_description)
            printf("    %s\n", device_list->devices[i]->device_description);
    }

    printf("Select device index: ");
    int choice = 0;
    if (scanf("%d", &choice) != 1 || choice < 0 || choice >= device_list->nb_devices) {
        printf("Invalid choice, defaulting to first device.\n");
        choice = 0;
    }

    char *result = strdup(device_list->devices[choice]->device_name);
    avdevice_free_list_devices(&device_list);
    return result;
}

int main(int argc, char *argv[]) {
    const char *iformat_name = DEFAULT_INPUT_FORMAT;
    avdevice_register_all();
    avformat_network_init();

    char *device = NULL;
    if (argc > 1) {
        device = strdup(argv[1]);
    } else {
        device = choose_device(iformat_name);
        if (!device) {
            fprintf(stderr, "No device selected.\n");
            return 1;
        }
    }

    const AVInputFormat *input_format = av_find_input_format(iformat_name);
    if (!input_format) {
        fprintf(stderr, "Could not find input format '%s'\n", iformat_name);
        free(device);
        return 1;
    }

    AVFormatContext *fmt_ctx = NULL;
    int ret = avformat_open_input(&fmt_ctx, device, input_format, NULL);
    if (ret < 0) {
        log_error("Failed to open input device", ret);
        free(device);
        return 1;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        log_error("Could not find stream info", ret);
        avformat_close_input(&fmt_ctx);
        free(device);
        return 1;
    }

    int video_stream_index = -1;
    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        fprintf(stderr, "No video stream found\n");
        avformat_close_input(&fmt_ctx);
        free(device);
        return 1;
    }

    AVCodecParameters *codecpar = fmt_ctx->streams[video_stream_index]->codecpar;
    const AVCodec *decoder = avcodec_find_decoder(codecpar->codec_id);
    if (!decoder) {
        fprintf(stderr, "Decoder not found for codec id %d\n", codecpar->codec_id);
        avformat_close_input(&fmt_ctx);
        free(device);
        return 1;
    }

    AVCodecContext *dec_ctx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(dec_ctx, codecpar);
    avcodec_open2(dec_ctx, decoder, NULL);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        free(device);
        return 1;
    }

    int width = dec_ctx->width > 0 ? dec_ctx->width : 640;
    int height = dec_ctx->height > 0 ? dec_ctx->height : 480;

    SDL_Window *window = SDL_CreateWindow("Camera Stream", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          width, height, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                            SDL_TEXTUREACCESS_STREAMING, width, height);

    struct SwsContext *sws_ctx = sws_getContext(dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
                                                width, height, AV_PIX_FMT_RGB24,
                                                SWS_BILINEAR, NULL, NULL, NULL);

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();
    av_image_alloc(rgb_frame->data, rgb_frame->linesize, width, height, AV_PIX_FMT_RGB24, 1);

    int quit = 0;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) quit = 1;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
        }
        ret = av_read_frame(fmt_ctx, packet);
        if (ret < 0) { SDL_Delay(10); continue; }

        if (packet->stream_index == video_stream_index) {
            ret = avcodec_send_packet(dec_ctx, packet);
            while (ret >= 0) {
                ret = avcodec_receive_frame(dec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                if (ret < 0) break;

                sws_scale(sws_ctx, (const uint8_t * const*)frame->data, frame->linesize,
                          0, dec_ctx->height, rgb_frame->data, rgb_frame->linesize);

                SDL_UpdateTexture(texture, NULL, rgb_frame->data[0], rgb_frame->linesize[0]);
                int win_w, win_h;
                SDL_GetWindowSize(window, &win_w, &win_h);
                SDL_Rect dst = {0, 0, win_w, win_h};
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, &dst);
                SDL_RenderPresent(renderer);

                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
    }

    av_freep(&rgb_frame->data[0]);
    av_frame_free(&rgb_frame);
    av_frame_free(&frame);
    av_packet_free(&packet);
    sws_freeContext(sws_ctx);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt_ctx);
    avformat_network_deinit();
    free(device);
    return 0;
}
