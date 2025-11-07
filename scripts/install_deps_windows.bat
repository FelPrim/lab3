@echo off
echo Проверка и установка системных зависимостей для Windows...

:: Переходим в корень проекта
cd /d "%~dp0.."

:: Проверка наличия MSYS2
where pacman >nul 2>nul
if %errorlevel% neq 0 (
    echo Ошибка: MSYS2 не найден. Установите MSYS2 с https://www.msys2.org/
    pause
    exit /b 1
)

:: Обновление пакетов (только если не в CI)
if "%CI%"=="" (
    echo Обновление пакетов MSYS2...
    pacman -Syu --noconfirm
)

:: Установка системных зависимостей
echo Установка системных зависимостей...
pacman -S --noconfirm --needed \
    mingw-w64-x86_64-toolchain \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-pkg-config \
    git \
    make \
    mingw-w64-x86_64-qt6-base \
    mingw-w64-x86_64-qt6-tools \
    mingw-w64-x86_64-opencv

echo Все системные зависимости установлены успешно!
echo Примечание: FFmpeg уже установлен статически в deps/Windows/ffmpeg_static/
