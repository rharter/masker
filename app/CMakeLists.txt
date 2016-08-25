cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions -Wall")
set(OPENGL_LIB GLESv3)
set(graphics_SRCS src/main/cpp/masker.cpp)

if (${TARGET_PLATFORM_VERSION} LESS 11)
    MESSAGE(ERROR " OpenGL ES 2  is not supported")
    return()
elseif (${TARGET_PLATFORM_VERSION} LESS 18)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDYNAMIC_ES3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDYNAMIC_ES3")
    set(graphics_SRCS ${graphics_SRCS} gl3stub_wrapper.c)
    set(OPENGL_LIB GLESv2)
endif (${TARGET_PLATFORM_VERSION} LESS 11)

add_library(graphics SHARED ${graphics_SRCS})

target_link_libraries(graphics android log m jnigraphics EGL ${OPENGL_LIB})
