CMAKE_BUILD_DIR := build
BIN_DIR := $(CMAKE_BUILD_DIR)/bin
TARGET := VideoCapture

ifeq ($(origin QTDIR), undefined)
  CMAKE_PREFIX_ARG :=
else
  CMAKE_PREFIX_ARG := -DCMAKE_PREFIX_PATH=$(QTDIR)
endif

ifdef CMAKE_PREFIX_PATH
  CMAKE_PREFIX_ARG := -DCMAKE_PREFIX_PATH=$(CMAKE_PREFIX_PATH)
endif

ifdef OpenCV_DIR
  OPENCV_ARG := -DOpenCV_DIR=$(OpenCV_DIR)
endif

all:
	@mkdir -p $(CMAKE_BUILD_DIR)
	cmake -S . -B $(CMAKE_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release $(CMAKE_PREFIX_ARG) $(OPENCV_ARG)
	cmake --build $(CMAKE_BUILD_DIR) --config Release --parallel $(NPROC)

run: all
	@echo "Запуск $(TARGET)..."
	$(BIN_DIR)/$(TARGET) || $(BIN_DIR)/$(TARGET).exe

clean:
	-rm -rf $(CMAKE_BUILD_DIR)

.PHONY: all run clean
