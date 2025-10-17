
#include <stdio.h>
#include <libavformat/avformat.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <video_file>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];

    AVFormatContext *fmt_ctx = NULL;

    // Открываем видеофайл
    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open file: %s\n", filename);
        return 1;
    }

    // Получаем информацию о потоках
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Выводим информацию о файле
    av_dump_format(fmt_ctx, 0, filename, 0);

    // Закрываем файл
    avformat_close_input(&fmt_ctx);

    return 0;
}
