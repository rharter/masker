cmake_minimum_required(VERSION 3.4.1)

set(masker_SRCS src/main/cpp/masker.cpp)
add_library(masker SHARED ${masker_SRCS})

target_link_libraries(masker log jnigraphics GLESv2)
