
# Makefile для динамической сборки ffcheck в подпапке dynamic
# Собирает ffcheck_dynamic.exe и копирует все нужные DLL (FFmpeg + MinGW + внешние кодеки)

# Пути
FFMPEG_SHARED_DIR := ../../../deps/Windows/ffmpeg
PKG_CONFIG_PATH := $(FFMPEG_SHARED_DIR)/lib/pkgconfig

# Каталоги, где могут находиться DLL внешних зависимостей
EXTRA_DLL_DIRS := \
	$(FFMPEG_SHARED_DIR)/bin \
	$(FFMPEG_SHARED_DIR)/opus/bin \
	$(FFMPEG_SHARED_DIR)/dav1d/bin \
	$(FFMPEG_SHARED_DIR)/svtav1/bin \
	$(FFMPEG_SHARED_DIR)/x265/bin

# Определим пути к MinGW-библиотекам (работает и в MSYS, и в mingw32-make)
MINGW_BIN_POSIX := /mingw64/bin
MINGW_BIN_WIN   := $(shell cygpath -m /mingw64/bin)

# Компиляторы
CC  := gcc
CXX := g++

# Исходник и цели
SRC := ../ffcheck.c
OBJ := ffcheck.o
OUT := ffcheck_dynamic.exe

# pkg-config флаги (для shared сборки)
CFLAGS  := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags libavformat libavcodec libavutil 2>/dev/null)
LDFLAGS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs libavformat libavcodec libavutil 2>/dev/null)

.PHONY: all prepare-dlls run clean

# === Основные цели ===

all: $(OUT) prepare-dlls

$(OBJ): $(SRC)
	@echo "Compiling $< -> $@"
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUT): $(OBJ)
	@echo "Linking $@"
	$(CXX) -o $@ $^ $(LDFLAGS)

# === Копирование DLL ===
prepare-dlls: $(OUT)
	@echo "Preparing DLLs for $(OUT)..."
	@if ! command -v objdump >/dev/null 2>&1; then \
	  echo "Warning: objdump not found in PATH; cannot auto-detect DLL dependencies."; \
	  exit 0; \
	fi
	@echo "Scanning $(OUT) for required DLLs..."
	@for dll in $$(objdump -p $(OUT) 2>/dev/null | awk '/DLL Name/ {print $$3}' | sort -u); do \
	  case "$$dll" in \
	    KERNEL32.dll|MSVCRT.dll|USER32.dll|SHELL32.dll|ADVAPI32.dll|WS2_32.dll|OLE32.dll|OLEAUT32.dll|AVRT.dll|COMDLG32.dll|GDI32.dll) \
	      echo "System DLL: $$dll (skip)"; \
	      continue;; \
	  esac; \
	  if [ -f "./$$dll" ]; then \
	    echo "OK: $$dll already present"; \
	  else \
	    found=$$(find $(EXTRA_DLL_DIRS) "$(MINGW_BIN_POSIX)" "$(MINGW_BIN_WIN)" -type f -iname "$$dll" -print -quit 2>/dev/null || true); \
	    if [ -n "$$found" ]; then \
	      echo "Copy: $$dll <- $$found"; cp -v "$$found" .; \
	    else \
	      echo "Warning: $$dll not found under any known directory."; \
	    fi; \
	  fi; \
	done

	@# Дополнительные MinGW и FFmpeg DLL, которые могут не определяться напрямую
	@echo "Checking for extra MinGW/FFmpeg DLLs..."
	@for extra_dll in \
	    libbz2-1.dll libiconv-2.dll libwinpthread-1.dll \
	    liblzma-5.dll zlib1.dll libstdc++-6.dll libgcc_s_seh-1.dll \
	    swresample-6.dll libopus-0.dll; do \
	  if [ ! -f "./$$extra_dll" ]; then \
	    for dir in $(EXTRA_DLL_DIRS) $(MINGW_BIN_POSIX) $(MINGW_BIN_WIN); do \
	      if [ -f "$$dir/$$extra_dll" ]; then \
	        echo "Copy (extra): $$extra_dll <- $$dir/$$extra_dll"; \
	        cp -v "$$dir/$$extra_dll" .; \
	        break; \
	      fi; \
	    done; \
	    if [ ! -f "./$$extra_dll" ]; then \
	      echo "Warning: $$extra_dll not found anywhere!"; \
	    fi; \
	  else \
	    echo "OK: $$extra_dll already present"; \
	  fi; \
	done
	@echo "DLL preparation done."

# === Запуск ===
run: all
	@echo "Running $(OUT) with $(FFMPEG_SHARED_DIR)/bin and current dir in PATH..."
	@PATH="$(CURDIR):$(FFMPEG_SHARED_DIR)/bin:$$PATH" ./$(OUT)

# === Очистка ===
clean:
	@echo "Cleaning..."
	@rm -f $(OBJ) $(OUT)
