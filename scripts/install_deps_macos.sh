#!/bin/bash
echo "Проверка и установка системных зависимостей для macOS..."

# Переходим в корень проекта
cd "$(dirname "$0")/.."
PROJECT_ROOT="$(pwd)"
echo "Рабочая директория: $PROJECT_ROOT"

# Проверка наличия Homebrew
if ! command -v brew &> /dev/null; then
    echo "Установка Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
    eval "$(/opt/homebrew/bin/brew shellenv)"
else
    echo "Homebrew уже установлен."
    
    if [ -z "$CI" ]; then
        echo "Обновление Homebrew..."
        brew update
    fi
fi

echo "Установка системных зависимостей..."
brew install \
    cmake \
    pkg-config \
    git \
    qt6 \
    opencv \
    ffmpeg

echo "Все системные зависимости установлены успешно!"
echo "Если это первая установка Qt6, перезапустите терминал или выполните: source ~/.zshrc"
