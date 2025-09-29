
#include <stdio.h>
#include <stdlib.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>

// ====== Вывод списка устройств ======
void list_devices(const char *format_name, int input) {
    AVDeviceInfoList *devlist = NULL;
    AVInputFormat *iformat = NULL;
    AVOutputFormat *oformat = NULL;
    AVFormatContext *tmp = NULL;

    if (input)
        iformat = av_find_input_format(format_name);
    else
        oformat = av_guess_format(format_name, NULL, NULL);

    if (!iformat && !oformat) {
        fprintf(stderr, "Не найден драйвер %s\n", format_name);
        return;
    }

    if (avdevice_list_devices(tmp, &devlist) < 0) {
        fprintf(stderr, "Ошибка при получении списка устройств %s\n", format_name);
        return;
    }

    printf("\nДоступные %s устройства (%s):\n",
           input ? "ввод" : "вывод", format_name);
    for (int i = 0; i < devlist->nb_devices; i++) {
        AVDeviceInfo *dev = devlist->devices[i];
        printf("  [%d] %s\n", i, dev->device_name);
    }
    avdevice_free_list_devices(&devlist);
}

// ====== Меню выбора ======
char *choose_device(const char *format_name, int input) {
    AVDeviceInfoList *devlist = NULL;
    AVInputFormat *iformat = NULL;
    AVOutputFormat *oformat = NULL;
    AVFormatContext *tmp = NULL;

    if (input)
        iformat = av_find_input_format(format_name);
    else
        oformat = av_guess_format(format_name, NULL, NULL);

    if (!iformat && !oformat) return NULL;

    if (avdevice_list_devices(tmp, &devlist) < 0 || devlist->nb_devices <= 0) {
        fprintf(stderr, "Нет доступных устройств (%s)\n", format_name);
        return NULL;
    }

    for (int i = 0; i < devlist->nb_devices; i++) {
        AVDeviceInfo *dev = devlist->devices[i];
        printf("  [%d] %s\n", i, dev->device_name);
    }

    int choice = 0;
    printf("Выберите номер: ");
    scanf("%d", &choice);

    if (choice < 0 || choice >= devlist->nb_devices) choice = 0;

    char *devname = strdup(devlist->devices[choice]->device_name);
    avdevice_free_list_devices(&devlist);
    return devname;
}

int main() {
    avdevice_register_all();

#ifdef _WIN32
    const char *in_format_name = "dshow";
#else
    const char *in_format_name = "alsa";
#endif

    printf("Выбор устройства ввода (микрофон):\n");
    char *in_dev = choose_device(in_format_name, 1);
    if (!in_dev) {
        fprintf(stderr, "Устройство ввода не выбрано\n");
        return -1;
    }
    printf("Выбрано устройство ввода: %s\n", in_dev);

    // Открываем устройство ввода
    AVFormatContext *fmt_ctx = NULL;
    AVInputFormat *iformat = av_find_input_format(in_format_name);
    if (avformat_open_input(&fmt_ctx, in_dev, iformat, NULL) < 0) {
        fprintf(stderr, "Не удалось открыть устройство: %s\n", in_dev);
        return -1;
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Не удалось получить stream info\n");
        return -1;
    }

    int stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (stream_index < 0) {
        fprintf(stderr, "Аудио поток не найден\n");
        return -1;
    }

    AVCodecParameters *codecpar = fmt_ctx->streams[stream_index]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "Кодек не найден\n");
        return -1;
    }

    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codecpar);
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Не удалось открыть кодек\n");
        return -1;
    }

    // Настройка SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_AudioSpec wanted_spec, obtained_spec;
    wanted_spec.freq = codec_ctx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = codec_ctx->channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;
    wanted_spec.callback = NULL;

    if (SDL_OpenAudio(&wanted_spec, &obtained_spec) < 0) {
        fprintf(stderr, "SDL_OpenAudio error: %s\n", SDL_GetError());
        return -1;
    }
    SDL_PauseAudio(0);

    SwrContext *swr = swr_alloc_set_opts(
        NULL,
        av_get_default_channel_layout(obtained_spec.channels),
        AV_SAMPLE_FMT_S16,
        obtained_spec.freq,
        av_get_default_channel_layout(codec_ctx->channels),
        codec_ctx->sample_fmt,
        codec_ctx->sample_rate,
        0, NULL
    );
    swr_init(swr);

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    uint8_t *out_buf = (uint8_t *)av_malloc(192000);

    printf("\nНачинается ретрансляция (Ctrl+C для выхода)...\n");

    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == stream_index) {
            if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    int out_samples = swr_convert(swr, &out_buf, 192000,
                                                  (const uint8_t **)frame->data, frame->nb_samples);
                    int out_size = av_samples_get_buffer_size(NULL, obtained_spec.channels,
                                                              out_samples, AV_SAMPLE_FMT_S16, 1);
                    SDL_QueueAudio(1, out_buf, out_size);
                }
            }
        }
        av_packet_unref(pkt);
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    swr_free(&swr);
    SDL_CloseAudio();
    SDL_Quit();
    free(in_dev);

    return 0;
}
