
#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

int main(int argc, char **argv) {
puts("-1");
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input>\n", argv[0]);
        return 1;
    }
puts("0");
    const char *filename = argv[1];

    /* Установим уровень логирования (по желанию) */
    av_log_set_level(AV_LOG_INFO);

    AVFormatContext *fmt = NULL;
    puts("1");

    if (avformat_open_input(&fmt, filename, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open input file '%s'\n", filename);
        return 2;
    }
    puts("2");

    if (avformat_find_stream_info(fmt, NULL) < 0) {
        fprintf(stderr, "Could not find stream information for '%s'\n", filename);
        avformat_close_input(&fmt);
        return 3;
    }
    puts("3");

    /* Выведем подробную информацию о контейнере/потоках */
    av_dump_format(fmt, 0, filename, 0);
    puts("4");

    /* Попробуем быстро найти первый видео-поток и показать его кодек */
    int vstream = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    puts("5");
    if (vstream >= 0) {
        AVCodecParameters *par = fmt->streams[vstream]->codecpar;
        printf("First video stream: index=%d, codec=%s, %dx%d\n",
               vstream,
               avcodec_get_name(par->codec_id),
               par->width, par->height);
    } else {
        printf("No video stream found.\n");
    }

    avformat_close_input(&fmt);
    return 0;
}
