#include "utils.hpp"

#if defined(WINDOWS)
#include <windows.h>
#endif

#include <filesystem>
#include <iostream>

std::vector<uint8_t> read_file(const char* path) {
#if defined(WINDOWS)
    FILE* f = nullptr;
    errno_t err = fopen_s(&f, path, "rb");

    if (err != 0) {
        std::cerr << "Cannot open file !" << std::endl;
        return {};
    }
#elif defined(LINUX)
    FILE* f = fopen(path, "rb");

    if (!f) {
        std::cerr << "Cannot open file !" << std::endl;
        return {};
    }
#endif

    if (fseek(f, 0, SEEK_END) != 0) {
        return {};
    }

    std::vector<uint8_t> file_content(ftell(f));

    if (fseek(f, 0, SEEK_SET) != 0) {
        return {};
    }

    if (fread(file_content.data(), sizeof(uint8_t), file_content.size(), f) != file_content.size()) {
        std::cerr << "Reading file content failed" << std::endl;
        return {};
    }

    fclose(f);

    return file_content;
}


#ifdef WINDOWS
void log_last_error() {
    auto error_code = GetLastError();
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

    std::cerr << error_message << std::endl;
}

bool watcher::watch_file(std::filesystem::path&& filepath, std::function<void()> callback) {
    if (!std::filesystem::exists(filepath))
        return false;

    if (!watcher::watch_dir(filepath.parent_path()))
        return false;

    auto [_, is_inserted] = watcher::callbacks.emplace(filepath.filename().string(), callback);

    return is_inserted;
}

bool watcher::watch_dir(std::filesystem::path&& dir_path) {
    if (!std::filesystem::exists(dir_path))
        return false;

    if (watcher::watched_dirs.find(dir_path.string()) != watcher::watched_dirs.end())
        return true;

    HANDLE dir_handle = CreateFileW(
        dir_path.c_str(),
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

    auto [dir, _] = watcher::watched_dirs.emplace(dir_path.string(), watch_data{ .dir_handle = dir_handle });

    if (FAILED(ReadDirectoryChangesW(dir_handle, dir->second.buffer, sizeof(watch_data::buffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, nullptr, &dir->second.overlapped, nullptr))) {
        log_last_error();
        return false;
    }

    return true;
}

void watcher::pull_changes() {
    for (auto& dir : watcher::watched_dirs) {
        auto& watch_data = dir.second;
        if (!HasOverlappedIoCompleted(&watch_data.overlapped))
            continue;

        auto* dir_event = (PFILE_NOTIFY_INFORMATION)watch_data.buffer;
        do {
            if (dir_event->Action == FILE_ACTION_MODIFIED) {
                auto w_filename = std::wstring{ dir_event->FileName, dir_event->FileNameLength / sizeof(wchar_t) };
                auto filename = std::string{w_filename.begin(), w_filename.end()};
                if (callbacks.contains(filename))
                    callbacks[filename]();
            }

            dir_event += dir_event->NextEntryOffset;
        } while(dir_event->NextEntryOffset != 0);

        if (FAILED(ReadDirectoryChangesW(watch_data.dir_handle, watch_data.buffer, sizeof(watch_data::buffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, nullptr, &watch_data.overlapped, nullptr))) {
            log_last_error();
        }
    }
}
#endif
