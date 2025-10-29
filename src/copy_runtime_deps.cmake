# copy_runtime_deps.cmake
# Copies OpenCV and related DLLs from MSYS2 mingw64/bin to build/bin

if(NOT DEFINED DEPLOY_BIN)
    message(FATAL_ERROR "DEPLOY_BIN not set")
endif()
if(NOT DEFINED MSYS_BIN)
    message(FATAL_ERROR "MSYS_BIN not set")
endif()

message(STATUS "Copying runtime DLLs from: ${MSYS_BIN} → ${DEPLOY_BIN}")

file(GLOB _opencv_dlls "${MSYS_BIN}/libopencv_*.dll")
file(GLOB _ffmpeg_dlls "${MSYS_BIN}/av*.dll" "${MSYS_BIN}/swscale*.dll")
file(GLOB _gstreamer_dlls "${MSYS_BIN}/libgst*.dll" "${MSYS_BIN}/libgobject-2.0-0.dll" "${MSYS_BIN}/libglib-2.0-0.dll")
file(GLOB _runtime_dlls "${MSYS_BIN}/libgcc_s_seh-1.dll" "${MSYS_BIN}/libstdc++-6.dll" "${MSYS_BIN}/libwinpthread-1.dll" "${MSYS_BIN}/libtbb*.dll" "${MSYS_BIN}/zlib1.dll")

set(_all_dlls ${_opencv_dlls} ${_ffmpeg_dlls} ${_gstreamer_dlls} ${_runtime_dlls})

foreach(_dll IN LISTS _all_dlls)
    get_filename_component(_name "${_dll}" NAME)
    file(COPY "${_dll}" DESTINATION "${DEPLOY_BIN}")
    message(STATUS "Copied ${_name}")
endforeach()
