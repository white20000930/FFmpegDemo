# 设置ffmpeg依赖库及头文件所在目录，并存进指定变量
set(ffmpeg_libs_DIR /opt/homebrew/Cellar/ffmpeg/7.0.1/lib)
set(ffmpeg_headers_DIR /opt/homebrew/Cellar/ffmpeg/7.0.1/include)

# 设置SDL2依赖库及头文件所在目录，并存进指定变量
set(SDL2_libs_DIR /opt/homebrew/Cellar/sdl2/2.30.3/lib)
set(SDL2_headers_DIR /opt/homebrew/Cellar/sdl2/2.30.3/include/SDL2)

# 添加依赖库
add_library( avcodec SHARED IMPORTED )
set_target_properties( avcodec PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs_DIR}/libavcodec.dylib )
 
add_library( avfilter SHARED IMPORTED )
set_target_properties( avfilter PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs_DIR}/libavfilter.dylib )
 
add_library( swresample SHARED IMPORTED )
set_target_properties( swresample PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs_DIR}/libswresample.dylib )
 
add_library( swscale SHARED IMPORTED )
set_target_properties( swscale PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs_DIR}/libswscale.dylib )
 
add_library( avformat SHARED IMPORTED )
set_target_properties( avformat PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs_DIR}/libavformat.dylib )
 
add_library( avutil SHARED IMPORTED )
set_target_properties( avutil PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs_DIR}/libavutil.dylib )

add_library( SDL2 SHARED IMPORTED )
set_target_properties( SDL2 PROPERTIES IMPORTED_LOCATION ${SDL2_libs_DIR}/libSDL2.dylib )

# 添加源文件
aux_source_directory(. SRC_LIST)
add_library(src_lib ${SRC_LIST})

# 添加头文件路径到编译器的头文件搜索路径下
target_include_directories(src_lib PUBLIC ${ffmpeg_headers_DIR})
target_include_directories(src_lib PUBLIC ${SDL2_headers_DIR})

# 链接目标文件与依赖库
target_link_libraries(src_lib avcodec avformat avutil swresample swscale avfilter SDL2)

