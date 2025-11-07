#!/bin/bash
echo "Проверка и установка системных зависимостей для Linux..."

# Переходим в корень проекта
cd "$(dirname "$0")/.."
PROJECT_ROOT="$(pwd)"
echo "Рабочая директория: $PROJECT_ROOT"

# Определение дистрибутива
if command -v apt >/dev/null 2>&1; then
    # Debian/Ubuntu
    echo "Обнаружен Debian/Ubuntu"
    
    if [ -z "$CI" ]; then
        sudo apt update
    fi
    
    echo "Установка системных зависимостей..."
    sudo apt install -y \
        build-essential \
        cmake \
        pkg-config \
        git \
        libopencv-dev \
        libavcodec-dev \
        libavformat-dev \
        libavutil-dev \
        libswscale-dev \
        qt6-base-dev \
        qt6-tools-dev

elif command -v dnf >/dev/null 2>&1; then
    # Fedora
    echo "Обнаружен Fedora"
    
    echo "Установка системных зависимостей..."
    sudo dnf install -y \
        gcc-c++ \
        cmake \
        pkg-config \
        git \
        opencv-devel \
        ffmpeg-devel \
        qt6-qtbase-devel \
        qt6-qttools-devel

elif command -v pacman >/dev/null 2>&1; then
    # Arch Linux
    echo "Обнаружен Arch Linux"
    
    echo "Установка системных зависимостей..."
    sudo pacman -Syu --noconfirm \
        base-devel \
        cmake \
        pkg-config \
        git \
        opencv \
        ffmpeg \
        qt6-base

else
    echo "Неизвестный дистрибутив. Установите зависимости вручную."
    exit 1
fi

echo "Все системные зависимости установлены успешно!"
