
/*
camera_stream_sdl3.c

SDL3 version of camera streaming + device selection (FFmpeg for capture/decoding, SDL3 for display).

Build examples (MSYS2 MinGW64):
# A) static-link SDL3 (libSDL3.a) + dynamic FFmpeg (import libs .dll.a)
gcc camera_stream_sdl3.c -o camera_stream.exe \
  -I"$HOME/static-libraries/Windows/default/sdl3/include" \
  -I"$HOME/ffmpeg_build/include" \
  "$HOME/static-libraries/Windows/default/sdl3/lib/libSDL3.a" \
  -L"$HOME/ffmpeg_build/lib" -lavdevice -lavformat -lavcodec -lswscale -lavutil \
  -lmingw32 -lSDL3main \
  -lws2_32 -lwinmm -lole32 -lshlwapi -ladvapi32 -lbcrypt -lz -lm -lpthread \
  -Wl,--enable-auto-import -static-libgcc -static-libstdc++

# B) fully static (SDL3 + FFmpeg static .a) - only if you have static libav*.a
gcc camera_stream_sdl3.c -o camera_stream_static.exe \
  -I"$HOME/static-libraries/Windows/default/sdl3/include" \
  -I"$HOME/ffmpeg_build/include" \
  "$HOME/static-libraries/Windows/default/sdl3/lib/libSDL3.a" \
  -L"$HOME/ffmpeg_build/lib" -lavdevice -lavformat -lavcodec -lswscale -lavutil \
  -lmingw32 -lSDL3main \
  -lws2_32 -lwinmm -lole32 -lshlwapi -ladvapi32 -lbcrypt -lz -lm -lpthread \
  -Wl,--whole-archive -lavcodec -lavformat -lavutil -lswscale -lavdevice -Wl,--no-whole-archive \
  -static -Wl,--enable-auto-import

Notes:
 - These examples assume MSYS2 MinGW64 environment and that the listed files exist.
 - If you only have import libraries (libSDL3.dll.a or libav*.dll.a) the exe will still depend on corresponding DLLs.
 - Adjust system libraries list if the linker reports undefined references.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define SDL_MAIN_HANDLED

#include <signal.h>

volatile sig_atomic_t sigint_received = 0;

static void handle_sigint(int signo) {
    (void)signo;
    sigint_received = 1;
}
#include <SDL3/SDL.h>

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
    AVDeviceInfoList *device_list = NULL;
    const AVInputFormat *ifmt = av_find_input_format(iformat_name);
    if (!ifmt) {
        fprintf(stderr, "Could not find input format %s\n", iformat_name);
        return NULL;
    }

    int ret = avdevice_list_input_sources((AVInputFormat*)ifmt, NULL, NULL, &device_list);
    if (ret < 0) {
        log_error("Could not list devices", ret);
        return NULL;
    }

    if (!device_list || device_list->nb_devices == 0) {
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
    int ret = avformat_open_input(&fmt_ctx, device, (AVInputFormat*)input_format, NULL);
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
    if (!dec_ctx) {
        fprintf(stderr, "Could not allocate decoder context\n");
        avformat_close_input(&fmt_ctx);
        free(device);
        return 1;
    }

    ret = avcodec_parameters_to_context(dec_ctx, codecpar);
    if (ret < 0) {
        log_error("Failed to copy codec params to context", ret);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        free(device);
        return 1;
    }

    ret = avcodec_open2(dec_ctx, decoder, NULL);
    if (ret < 0) {
        log_error("Failed to open codec", ret);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        free(device);
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        free(device);
        return 1;
    }

    int width = dec_ctx->width > 0 ? dec_ctx->width : codecpar->width;
    int height = dec_ctx->height > 0 ? dec_ctx->height : codecpar->height;
    if (width <= 0 || height <= 0) { width = 640; height = 480; }

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    if (!SDL_CreateWindowAndRenderer("Camera Stream (SDL3)", width, height, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        fprintf(stderr, "SDL_CreateWindowAndRenderer Error: %s\n", SDL_GetError());
        SDL_Quit();
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        free(device);
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_RGB24,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             width, height);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        free(device);
        return 1;
    }

    struct SwsContext *sws_ctx = sws_getContext(dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
                                                width, height, AV_PIX_FMT_RGB24,
                                                SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx) {
        fprintf(stderr, "Could not initialize sws context\n");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        free(device);
        return 1;
    }

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();
    if (!packet || !frame || !rgb_frame) {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    int rgb_bufsize = av_image_alloc(rgb_frame->data, rgb_frame->linesize, width, height, AV_PIX_FMT_RGB24, 1);
    if (rgb_bufsize < 0) {
        fprintf(stderr, "Could not allocate RGB image buffer\n");
        return 1;
    }

    int quit = 0;
    SDL_Event event;

    signal(SIGINT, handle_sigint);
    while (!quit && !sigint_received) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) quit = 1;
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) quit = 1;
            }
        }

        ret = av_read_frame(fmt_ctx, packet);
        if (ret < 0) { SDL_Delay(10); continue; }

        if (packet->stream_index == video_stream_index) {
            ret = avcodec_send_packet(dec_ctx, packet);
            if (ret < 0) {
                log_error("Error sending packet to decoder", ret);
                av_packet_unref(packet);
                continue;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(dec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                else if (ret < 0) {
                    log_error("Error decoding frame", ret);
                    break;
                }

                sws_scale(sws_ctx,
                          (const uint8_t * const*)frame->data, frame->linesize,
                          0, dec_ctx->height,
                          rgb_frame->data, rgb_frame->linesize);

                // Update SDL texture (whole texture)
                if (!SDL_UpdateTexture(texture, NULL, rgb_frame->data[0], rgb_frame->linesize[0])) {
                    // SDL_UpdateTexture returns true on success; if false, print error
                    fprintf(stderr, "SDL_UpdateTexture failed: %s\n", SDL_GetError());
                }

                // Render to window
                int win_w, win_h;
                SDL_GetWindowSize(window, &win_w, &win_h);
                SDL_FRect dstf = {0.0f, 0.0f, (float)win_w, (float)win_h};

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                // Use SDL_RenderTexture to blit texture (SDL3 API)
                if (!SDL_RenderTexture(renderer, texture, NULL, &dstf)) {
                    fprintf(stderr, "SDL_RenderTexture failed: %s\n", SDL_GetError());
                }
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
