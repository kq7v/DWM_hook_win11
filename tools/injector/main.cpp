#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cwchar>
#include <string>

namespace {
    bool debug_privilege( ) {
        HANDLE token = nullptr;
        if( !OpenProcessToken( GetCurrentProcess( ), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token ) ) {
            return false;
        }
        TOKEN_PRIVILEGES privileges { };
        privileges.PrivilegeCount = 1;
        if( !LookupPrivilegeValueW( nullptr, SE_DEBUG_NAME, &privileges.Privileges[ 0 ].Luid ) ) {
            CloseHandle( token );
            return false;
        }
        privileges.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;
        SetLastError( ERROR_SUCCESS );
        const bool adjusted =
            AdjustTokenPrivileges( token, FALSE, &privileges, 0, nullptr, nullptr ) && GetLastError( ) == ERROR_SUCCESS;
        CloseHandle( token );
        return adjusted;
    }

    DWORD find_dwm( ) {
        DWORD session = 0;
        ProcessIdToSessionId( GetCurrentProcessId( ), &session );
        if( session == 0 ) {
            session = WTSGetActiveConsoleSessionId( );
        }
        const HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
        if( snapshot == INVALID_HANDLE_VALUE ) {
            return 0;
        }
        PROCESSENTRY32W entry { sizeof( entry ) };
        DWORD result = 0;
        if( Process32FirstW( snapshot, &entry ) ) {
            do {
                DWORD candidate_session = 0;
                if( _wcsicmp( entry.szExeFile, L"dwm.exe" ) == 0 &&
                    ProcessIdToSessionId( entry.th32ProcessID, &candidate_session ) && candidate_session == session ) {
                    result = entry.th32ProcessID;
                    break;
                }
            } while( Process32NextW( snapshot, &entry ) );
        }
        CloseHandle( snapshot );
        return result;
    }

    LPTHREAD_START_ROUTINE remote_loader( DWORD pid ) {
        const auto local = GetProcAddress( GetModuleHandleW( L"kernel32.dll" ), "LoadLibraryW" );
        HMODULE owner = nullptr;
        if( !local || !GetModuleHandleExW(
                          GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>( local ), &owner
                      ) ) {
            return nullptr;
        }
        wchar_t name[ MAX_PATH ] { };
        if( !GetModuleFileNameW( owner, name, MAX_PATH ) ) {
            return nullptr;
        }
        const auto* leaf = wcsrchr( name, L'\\' );
        const auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid );
        if( snapshot == INVALID_HANDLE_VALUE ) {
            return nullptr;
        }
        MODULEENTRY32W entry { sizeof( entry ) };
        LPTHREAD_START_ROUTINE result = nullptr;
        if( Module32FirstW( snapshot, &entry ) ) {
            do {
                if( _wcsicmp( entry.szModule, leaf ? leaf + 1 : name ) == 0 ) {
                    const auto offset =
                        reinterpret_cast<std::uintptr_t>( local ) - reinterpret_cast<std::uintptr_t>( owner );
                    result = reinterpret_cast<LPTHREAD_START_ROUTINE>( entry.modBaseAddr + offset );
                    break;
                }
            } while( Module32NextW( snapshot, &entry ) );
        }
        CloseHandle( snapshot );
        return result;
    }
}

int wmain( int argc, wchar_t** argv ) {
    const bool restart = argc == 3 && _wcsicmp( argv[ 1 ], L"--restart" ) == 0;
    if( argc != 2 && !restart ) {
        std::puts( "Usage: injector.exe [--restart] <absolute DLL path> (run elevated)" );
        return 2;
    }
    wchar_t resolved[ 32768 ] { };
    const auto length = GetFullPathNameW( argv[ restart ? 2 : 1 ], 32768, resolved, nullptr );
    const auto attributes = length && length < 32768 ? GetFileAttributesW( resolved ) : INVALID_FILE_ATTRIBUTES;
    if( attributes == INVALID_FILE_ATTRIBUTES || ( attributes & FILE_ATTRIBUTE_DIRECTORY ) ) {
        std::puts( "DLL path does not name an existing file" );
        return 2;
    }
    const std::wstring path = resolved;
    if( !debug_privilege( ) ) {
        std::printf( "Enable SeDebugPrivilege failed: %lu\n", GetLastError( ) );
        return 3;
    }
    DWORD pid = find_dwm( );
    if( restart ) {
        const auto old = OpenProcess( PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid );
        if( !old || !TerminateProcess( old, 0 ) ) {
            std::printf( "DWM restart failed: %lu\n", GetLastError( ) );
            if( old ) {
                CloseHandle( old );
            }
            return 8;
        }
        const auto stopped = WaitForSingleObject( old, 10000 );
        CloseHandle( old );
        if( stopped != WAIT_OBJECT_0 ) {
            return 8;
        }
        const DWORD previous = pid;
        for( unsigned attempt = 0; attempt != 100; ++attempt ) {
            Sleep( 100 );
            pid = find_dwm( );
            if( pid && pid != previous ) {
                Sleep( 1000 );
                break;
            }
        }
        if( !pid || pid == previous ) {
            return 8;
        }
    }
    const auto process = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid
    );
    if( !process ) {
        std::printf( "OpenProcess(%lu) failed: %lu\n", pid, GetLastError( ) );
        return 4;
    }
    const SIZE_T size = ( path.size( ) + 1 ) * sizeof( wchar_t );
    const auto remote = VirtualAllocEx( process, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
    if( !remote || !WriteProcessMemory( process, remote, path.c_str( ), size, nullptr ) ) {
        std::printf( "Remote DLL path failed: %lu\n", GetLastError( ) );
        if( remote ) {
            VirtualFreeEx( process, remote, 0, MEM_RELEASE );
        }
        CloseHandle( process );
        return 5;
    }
    const auto load = remote_loader( pid );
    const auto thread = load ? CreateRemoteThread( process, nullptr, 0, load, remote, 0, nullptr ) : nullptr;
    if( !thread ) {
        std::printf( "CreateRemoteThread failed: %lu\n", GetLastError( ) );
        VirtualFreeEx( process, remote, 0, MEM_RELEASE );
        CloseHandle( process );
        return 6;
    }
    const auto wait = WaitForSingleObject( thread, 10000 );
    DWORD result = 0;
    GetExitCodeThread( thread, &result );
    std::printf( "DWM pid=%lu loader wait=%lu result=0x%08lX\n", pid, wait, result );
    if( wait == WAIT_OBJECT_0 ) {
        VirtualFreeEx( process, remote, 0, MEM_RELEASE );
    }
    CloseHandle( thread );
    CloseHandle( process );
    return wait == WAIT_OBJECT_0 && result != 0 ? 0 : 7;
}
