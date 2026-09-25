cmake_minimum_required(VERSION 3.25)
project(DxRenderDojo LANGUAGES CXX)

file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS Source/*.cpp)
add_executable(DxRenderDojo WIN32 ${SourceFiles})
target_compile_features(DxRenderDojo PRIVATE cxx_std_20)
target_compile_definitions(DxRenderDojo PRIVATE UNICODE _UNICODE)

if(MSVC)
	target_compile_options(DxRenderDojo PRIVATE /W3 /utf-8)
else()
	target_compile_options(DxRenderDojo PRIVATE -Wall)
endif()

if(MINGW)
	target_link_options(DxRenderDojo PRIVATE -municode -static -static-libgcc -static-libstdc++)
endif()
