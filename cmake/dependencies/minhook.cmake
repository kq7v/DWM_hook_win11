include( FetchContent )

FetchContent_Declare( minhook
    URL https://codeload.github.com/TsudaKageyu/minhook/tar.gz/refs/tags/v1.3.4
    URL_HASH SHA256=1aebeae4ca898330c507860acc2fca2eb335fe446a3a2b8444c3bf8b2660a14e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR src
)
FetchContent_MakeAvailable( minhook )

add_library( minhook STATIC
    ${minhook_SOURCE_DIR}/src/buffer.c
    ${minhook_SOURCE_DIR}/src/hook.c
    ${minhook_SOURCE_DIR}/src/trampoline.c
    ${minhook_SOURCE_DIR}/src/hde/hde64.c
)

target_include_directories( minhook SYSTEM PUBLIC ${minhook_SOURCE_DIR}/include )
