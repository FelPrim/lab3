
# === Makefile для сборки ffcheck_static.exe ===

# Пути
FFMPEG_STATIC_DIR := ../../deps/Windows/ffmpeg_static
PKG_CONFIG_PATH := $(FFMPEG_STATIC_DIR)/lib/pkgconfig

# Компиляторы
CC  := gcc
CXX := g++

# Исходники / цели
SRC := ffcheck.c
OBJ := ffcheck.o
OUT := ffcheck_static.exe

# Флаги (через pkg-config)
CFLAGS  := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags libavformat libavcodec libavutil)
LDFLAGS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --static libavformat libavcodec libavutil)

# Цель по умолчанию
all: $(OUT)

# Правила сборки
$(OBJ): $(SRC)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUT): $(OBJ)
	$(CXX) -static -o $@ $^ $(LDFLAGS)

# Очистка
clean:
	rm -f $(OBJ) $(OUT)

.PHONY: all clean
