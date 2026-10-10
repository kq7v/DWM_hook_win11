include( FetchContent )

set( LIBHAT_TESTING OFF CACHE BOOL "" FORCE )
set( LIBHAT_EXAMPLES OFF CACHE BOOL "" FORCE )
set( LIBHAT_INSTALL_TARGET OFF CACHE BOOL "" FORCE )

FetchContent_Declare( libhat
    URL https://codeload.github.com/BasedInc/libhat/zip/refs/tags/v0.11.1
    URL_HASH SHA256=966ede9b7fa4918b035d8b076481c3293557495bdb715cce318f61fab459bef0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable( libhat )
