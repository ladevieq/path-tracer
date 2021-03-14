#ifndef __DEFINES_HPP_
#define __DEFINES_HPP_

#if defined(__linux__)
#define LINUX
#elif defined(_WIN32) || defined(_WIN64)
#define WINDOWS
#elif defined(__APPLE__)
#define MACOS
#endif

#endif // !__DEFINES_HPP_
