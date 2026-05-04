#include "utils.hpp"

#include <unordered_map>

uint8_t* read_file(const char* path, size_t* size) {
#if _WIN32
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (f == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Cannot open file !\n");
        return nullptr;
    }
#elif LINUX
    FILE* f = fopen(path, "rb");

    if (!f) {
        fprintf(stderr, "Cannot open file !\n");
        return {};
    }
#endif

    LARGE_INTEGER file_size = { .QuadPart = 0ULL };
    if (!GetFileSizeEx(f, &file_size)) {
        return nullptr;
    }
    *size = file_size.LowPart;

    void* content = malloc(*size);

    DWORD bread = 0U;
    if (!ReadFile(f, content, static_cast<DWORD>(*size), &bread, NULL) || bread != *size) {
        fprintf(stderr, "Reading file content failed\n");
        return nullptr;
    }

    CloseHandle(f);

    return (uint8_t*)content;
}

#if _WIN32
void log_last_error() {
    DWORD error_code = GetLastError();
    const char* error_message = nullptr;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &error_message,
        0,
        nullptr
    );

    fprintf(stderr, "%s\n", error_message);
}

struct watch_data {
    HANDLE      dir_handle;
    OVERLAPPED  overlapped;
    DWORD       buffer[512];
};

static std::unordered_map<const char*, func> callbacks;
static std::unordered_map<const char*, watch_data> watched_dirs;

bool watch_file(const char* filepath, func callback) {
    DWORD attrib = GetFileAttributes(filepath);
    if (attrib == INVALID_FILE_ATTRIBUTES)
        return false;

    size_t buffer_size = 2048U;
    char* fullpath = static_cast<char*>(malloc(buffer_size));
    char* filename = nullptr;
    GetFullPathNameA( filepath, 2028, fullpath, &filename);

    size_t dir_len = filename - fullpath;
    char* parent_dir = static_cast<char*>(malloc(dir_len));
    strncpy(parent_dir, fullpath, dir_len);

    free(fullpath);

    if (!watch_dir(parent_dir)) {
        return false;
    }

    auto [_, is_inserted] = callbacks.emplace(filepath, callback);

    return is_inserted;
}

bool watch_dir(const char* dir_path) {
    DWORD attrib = GetFileAttributes(dir_path);
    if (attrib == INVALID_FILE_ATTRIBUTES || (attrib & FILE_ATTRIBUTE_DIRECTORY) == 0U )
        return false;

    if (watched_dirs.find(dir_path) != watched_dirs.end())
        return true;

    HANDLE dir_handle = CreateFileA(
        dir_path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );

    if(dir_handle == INVALID_HANDLE_VALUE) {
        log_last_error();
        return false;
    }

    auto [dir, _] = watched_dirs.emplace(dir_path, watch_data{ .dir_handle = dir_handle, .overlapped{}, .buffer{} });

    if (FAILED(ReadDirectoryChangesW(dir_handle, dir->second.buffer, sizeof(watch_data::buffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, nullptr, &dir->second.overlapped, nullptr))) {
        log_last_error();
        return false;
    }

    return true;
}

void pull_changes() {
    for (auto& dir : watched_dirs) {
        auto& watch_data = dir.second;
        if (!HasOverlappedIoCompleted(&watch_data.overlapped))
            continue;

        auto* dir_event = (PFILE_NOTIFY_INFORMATION)watch_data.buffer;
        do {
            if (dir_event->Action == FILE_ACTION_MODIFIED) {
                int len = WideCharToMultiByte(CP_UTF8, 0, dir_event->FileName, -1, NULL, 0, NULL, NULL);

                char* filename = static_cast<char*>(alloca(len));
                WideCharToMultiByte(CP_UTF8, 0, dir_event->FileName, -1, filename, len, NULL, NULL);
                if (callbacks.contains(filename))
                    callbacks[filename]();
                // auto w_filename = std::wstring{ dir_event->FileName, dir_event->FileNameLength / sizeof(wchar_t) };
                // auto filename = std::string{w_filename.begin(), w_filename.end()};
                // if (callbacks.contains(filename.c_str()))
                //     callbacks[filename.c_str()]();
            }

            dir_event += dir_event->NextEntryOffset;
        } while(dir_event->NextEntryOffset != 0);

        if (FAILED(ReadDirectoryChangesW(watch_data.dir_handle, watch_data.buffer, sizeof(watch_data::buffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, nullptr, &watch_data.overlapped, nullptr))) {
            log_last_error();
        }
    }
}
#endif
