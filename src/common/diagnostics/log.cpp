#include <common/diagnostics/log.hpp>
#include <windows.h>
#include <cstdarg>
#include <cstdio>

namespace diagnostics {
    void log( const char* format, ... ) noexcept {
        static SRWLOCK lock = SRWLOCK_INIT;
        char message[ 2048 ] { };
        va_list args;
        va_start( args, format );
        vsnprintf_s( message, sizeof( message ), _TRUNCATE, format, args );
        va_end( args );
        char line[ 2200 ] { };
        const int length = sprintf_s(
            line, "[%lu:%lu %llu] %s\r\n", GetCurrentProcessId( ), GetCurrentThreadId( ), GetTickCount64( ), message
        );
        OutputDebugStringA( line );
        AcquireSRWLockExclusive( &lock );
        const auto file = CreateFileW(
            L"C:\\Users\\Public\\dwm-hook.log", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr
        );
        if( file != INVALID_HANDLE_VALUE ) {
            DWORD written = 0;
            WriteFile( file, line, static_cast<DWORD>( length ), &written, nullptr );
            CloseHandle( file );
        }
        ReleaseSRWLockExclusive( &lock );
    }
}
