
// main3_fixed.c
// Требует: FFmpeg dev headers/libs (libavdevice/libavformat/libavcodec/libswresample/libavutil)
// и SDL3 (headers + lib).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>

#include <SDL3/SDL.h>

static char *choose_input_device(const char *in_format_name)
{
    const AVInputFormat *iformat = av_find_input_format(in_format_name);
    if (!iformat) {
        fprintf(stderr, "Не найден вводной драйвер: %s\n", in_format_name);
        return NULL;
    }

    AVDeviceInfoList *devlist = NULL;
    if (avdevice_list_input_sources((AVInputFormat*)iformat, NULL, NULL, &devlist) < 0 || !devlist || devlist->nb_devices == 0) {
        fprintf(stderr, "Не удалось получить список устройств ввода (%s)\n", in_format_name);
        if (devlist) avdevice_free_list_devices(&devlist);
        return NULL;
    }

    printf("Доступные устройства ввода (%s):\n", in_format_name);
    for (int i = 0; i < devlist->nb_devices; ++i) {
        printf("  [%d] %s\n", i, devlist->devices[i]->device_name);
    }
    int choice = 0;
    printf("Выберите номер (Enter для 0): ");
    if (scanf("%d", &choice) != 1) choice = 0;
    if (choice < 0 || choice >= devlist->nb_devices) choice = 0;

    char *devname = strdup(devlist->devices[choice]->device_name);
    avdevice_free_list_devices(&devlist);
    return devname;
}

static SDL_AudioDeviceID choose_output_device()
{
    int count = 0;
    SDL_AudioDeviceID *devs = SDL_GetAudioPlaybackDevices(&count);
    if (!devs || count == 0) {
        if (devs) SDL_free(devs);
        printf("Нет списка playback-устройств — будет использовано устройство по умолчанию.\n");
        return SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    }

    printf("Доступные устройства вывода:\n");
    for (int i = 0; i < count; ++i) {
        const char *name = SDL_GetAudioDeviceName(devs[i]);
        printf("  [%d] %s\n", i, name ? name : "(unnamed)");
    }
    int choice = 0;
    printf("Выберите номер устройства вывода (Enter для 0): ");
    if (scanf("%d", &choice) != 1) choice = 0;
    if (choice < 0 || choice >= count) choice = 0;

    SDL_AudioDeviceID chosen = devs[choice];
    SDL_free(devs);
    return chosen;
}

int main(int argc, char **argv)
{
    avdevice_register_all();

#ifdef _WIN32
    const char *in_format_name = "dshow";
#else
    const char *in_format_name = "alsa"; // можно "pulse" если предпочитаете PulseAudio
#endif

    char *in_dev = choose_input_device(in_format_name);
    if (!in_dev) {
        fprintf(stderr, "Входное устройство не выбрано.\n");
        return 1;
    }
    printf("Выбрано устройство ввода: %s\n", in_dev);

    // Открываем устройство ввода через FFmpeg
    AVFormatContext *fmt_ctx = NULL;
    AVInputFormat *iformat = (AVInputFormat*)av_find_input_format(in_format_name);
    if (!iformat) {
        fprintf(stderr, "Драйвер не найден: %s\n", in_format_name);
        free(in_dev);
        return 1;
    }

    AVDictionary *opts = NULL;
    if (avformat_open_input(&fmt_ctx, in_dev, iformat, &opts) < 0) {
        fprintf(stderr, "Не удалось открыть устройство ввода: %s\n", in_dev);
        free(in_dev);
        return 1;
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Не удалось получить stream info\n");
        avformat_close_input(&fmt_ctx);
        free(in_dev);
        return 1;
    }

    int stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (stream_index < 0) {
        fprintf(stderr, "Аудиопоток не найден\n");
        avformat_close_input(&fmt_ctx);
        free(in_dev);
        return 1;
    }

    AVStream *astream = fmt_ctx->streams[stream_index];
    AVCodecParameters *codecpar = astream->codecpar;
    const AVCodec *decoder = avcodec_find_decoder(codecpar->codec_id);
    if (!decoder) {
        fprintf(stderr, "Декодер не найден\n");
        avformat_close_input(&fmt_ctx);
        free(in_dev);
        return 1;
    }

    AVCodecContext *codec_ctx = avcodec_alloc_context3(decoder);
    if (!codec_ctx) {
        fprintf(stderr, "Не удалось аллоцировать codec_ctx\n");
        avformat_close_input(&fmt_ctx);
        free(in_dev);
        return 1;
    }
    if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
        fprintf(stderr, "avcodec_parameters_to_context failed\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        free(in_dev);
        return 1;
    }
    if (avcodec_open2(codec_ctx, decoder, NULL) < 0) {
        fprintf(stderr, "Не удалось открыть декодер\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        free(in_dev);
        return 1;
    }

    // Инициализация SDL3 аудио подсистемы
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        free(in_dev);
        return 1;
    }

    // Выбор устройства вывода пользователем
    SDL_AudioDeviceID out_devid = choose_output_device();
    if (out_devid == 0) { // это маловероятно, но на всякий случай
        out_devid = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    }
    if (out_devid == SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK) {
        printf("Используется устройство вывода по умолчанию.\n");
    } else {
        const char *dname = SDL_GetAudioDeviceName(out_devid);
        printf("Выбрано устройство вывода: %s\n", dname ? dname : "(unnamed)");
    }

    // Целевой формат вывода (мы будем подавать SDL interleaved S16)
    SDL_AudioSpec out_spec;
    SDL_zero(out_spec);
    out_spec.format = SDL_AUDIO_S16;   // используем S16 (portable)
    out_spec.channels = 2;             // стандартно — стерео выход
    out_spec.freq = 48000;             // стандартная частота вывода (можно поменять)

    // Создаём аудиопоток и открываем/привязываем к выбранному устройству
    SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(out_devid, &out_spec, NULL, NULL);
    if (!stream) {
        fprintf(stderr, "SDL_OpenAudioDeviceStream error: %s\n", SDL_GetError());
        SDL_Quit();
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        free(in_dev);
        return 1;
    }

    // Подготовим AVChannelLayout для src и dst (новая API FFmpeg)
    AVChannelLayout src_ch_layout = {0};
    AVChannelLayout dst_ch_layout = {0};

    // Попробуем взять layout из codecpar (новая структура ch_layout)
#if 1
    // большинство современных сборок FFmpeg имеют codecpar->ch_layout (AVChannelLayout)
    // если nb_channels == 0 — используем av_channel_layout_default по числу каналов (fallback)
    if (codecpar->ch_layout.nb_channels > 0) {
        src_ch_layout = codecpar->ch_layout; // struct copy
    } else {
        // попробуем использовать codecpar->channels (старые версии)
        int nb = 0;
        // в старых версиях codecpar->channels может существовать; безопасно читать через поле через параметры
        // но мы постараемся определить nb через codecpar->ch_layout.nb_channels (если не ноль - уже handled)
        // иначе попробуем использовать codecpar->channels via av_get_channel_layout_default
#ifdef AV_CODEC_CAP_CHANNELS
        nb = 2; // fallback — если ничего не известно — предполагаем 2
#else
        nb = 2;
#endif
        av_channel_layout_default(&src_ch_layout, nb);
    }
#else
    av_channel_layout_default(&src_ch_layout, 2); // fallback (не должно случиться)
#endif

    av_channel_layout_default(&dst_ch_layout, out_spec.channels);

    // Создадим swr контекст: вход — codec_ctx, выход — dst_ch_layout / S16 / out_spec.freq
    SwrContext *swr = swr_alloc_set_opts2(
        NULL,
        &dst_ch_layout, AV_SAMPLE_FMT_S16, out_spec.freq,
        &src_ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate,
        0, NULL
    );
    if (!swr) {
        fprintf(stderr, "swr_alloc_set_opts2 failed (проверьте версию libswresample)\n");
        SDL_DestroyAudioStream(stream);
        SDL_Quit();
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        free(in_dev);
        return 1;
    }
    if (swr_init(swr) < 0) {
        fprintf(stderr, "swr_init failed\n");
        swr_free(&swr);
        SDL_DestroyAudioStream(stream);
        SDL_Quit();
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        free(in_dev);
        return 1;
    }

    AVPacket *pkt = av_packet_alloc();
    AVFrame  *frame = av_frame_alloc();
    if (!pkt || !frame) {
        fprintf(stderr, "Не удалось аллоцировать pkt/frame\n");
        goto cleanup;
    }

    uint8_t *out_buf = av_malloc(192000); // временный буфер для S16 данных
    if (!out_buf) {
        fprintf(stderr, "Нет памяти для out_buf\n");
        goto cleanup;
    }

    printf("Ретрансляция запущена. Ctrl+C для остановки.\n");

    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == stream_index) {
            if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    // ресэмплим
                    const uint8_t **in_data = (const uint8_t **)frame->extended_data;
                    int in_samples = frame->nb_samples;

                    // выделяем столько out_samples, сколько нужно. swr_convert принимает указатель на массив указателей,
                    // но мы передаём &out_buf (interleaved) и ограничение в сэмплах (вместо байтов).
                    int max_out_samples = 192000 / (out_spec.channels * 2);
                    int out_samples = swr_convert(swr, &out_buf, max_out_samples, in_data, in_samples);
                    if (out_samples < 0) {
                        fprintf(stderr, "swr_convert error\n");
                        goto cleanup;
                    }

                    int out_size = av_samples_get_buffer_size(NULL, out_spec.channels, out_samples, AV_SAMPLE_FMT_S16, 1);
                    if (out_size <= 0) continue;

                    // Положим данные в SDL stream (копируется)
                    if (!SDL_PutAudioStreamData(stream, out_buf, out_size)) {
                        fprintf(stderr, "SDL_PutAudioStreamData failed: %s\n", SDL_GetError());
                        goto cleanup;
                    }

                    // Необязательно — можно периодически дергать SDL_GetAudioStreamAvailable или так просто спать
                    // чтобы CPU не жрал 100% (чтение из устройств обычно блокирующее и этого достаточно).
                }
            }
        }
        av_packet_unref(pkt);
    }

    printf("Входной поток завершён.\n");

    // Подождём, пока SDL проиграет оставшиеся данные (пуллится внутренне)
    // Можно опрашивать SDL_GetAudioStreamAvailable и читать с помощью SDL_GetAudioStreamData, но
    // т.к. мы используем SDL_OpenAudioDeviceStream, устройство будет опустошать stream.
    SDL_Delay(200); // короткая пауза чтобы немного дать сыграть хвост

cleanup:
    if (out_buf) av_free(out_buf);
    if (frame) av_frame_free(&frame);
    if (pkt) av_packet_free(&pkt);
    if (swr) swr_free(&swr);

    if (stream) SDL_DestroyAudioStream(stream);
    SDL_Quit();

    if (codec_ctx) avcodec_free_context(&codec_ctx);
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
    free(in_dev);
    return 0;
}
