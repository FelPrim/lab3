# Корневой Makefile для VideoCapture проекта
PROJECT_ROOT := $(shell pwd)
SRC_DIR := $(PROJECT_ROOT)/src
BUILD_DIR := $(PROJECT_ROOT)/build
BIN_DIR := $(BUILD_DIR)/bin
TARGET := VideoCapture

# Определение ОС
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    ifeq ($(UNAME_S),Linux)
        DETECTED_OS := Linux
    endif
    ifeq ($(UNAME_S),Darwin)
        DETECTED_OS := macOS
    endif
endif

# Аргументы CMake
ifdef QTDIR
    CMAKE_PREFIX_ARG := -DCMAKE_PREFIX_PATH=$(QTDIR)
else
    ifdef CMAKE_PREFIX_PATH
        CMAKE_PREFIX_ARG := -DCMAKE_PREFIX_PATH=$(CMAKE_PREFIX_PATH)
    endif
endif

ifdef OpenCV_DIR
    OPENCV_ARG := -DOpenCV_DIR=$(OpenCV_DIR)
endif

# Основные цели
all: configure build

configure:
	@mkdir -p $(BUILD_DIR)
	@echo "Конфигурация проекта для $(DETECTED_OS)..."
	cmake -S $(SRC_DIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release $(CMAKE_PREFIX_ARG) $(OPENCV_ARG)

build:
	@echo "Сборка проекта..."
	cmake --build $(BUILD_DIR) --config Release --parallel $(or $(NPROC),4)

debug: configure-debug build-debug

configure-debug:
	@mkdir -p $(BUILD_DIR)
	cmake -S $(SRC_DIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug $(CMAKE_PREFIX_ARG) $(OPENCV_ARG)

build-debug:
	cmake --build $(BUILD_DIR) --config Debug --parallel $(or $(NPROC),4)

run: build
	@echo "Запуск $(TARGET)..."
	@if [ "$(DETECTED_OS)" = "Windows" ]; then \
		$(BIN_DIR)/$(TARGET).exe; \
	else \
		$(BIN_DIR)/$(TARGET); \
	fi

clean:
	-rm -rf $(BUILD_DIR)

install-deps:
	@echo "Проверка и установка системных зависимостей для $(DETECTED_OS)..."
	@if [ "$(DETECTED_OS)" = "Windows" ]; then \
		./scripts/install_deps_windows.bat; \
	elif [ "$(DETECTED_OS)" = "Linux" ]; then \
		chmod +x ./scripts/install_deps_linux.sh && ./scripts/install_deps_linux.sh; \
	elif [ "$(DETECTED_OS)" = "macOS" ]; then \
		chmod +x ./scripts/install_deps_macos.sh && ./scripts/install_deps_macos.sh; \
	else \
		echo "Неизвестная ОС: $(DETECTED_OS)"; \
		exit 1; \
	fi

setup: install-deps all

.PHONY: all configure build debug configure-debug build-debug run clean install-deps setup
