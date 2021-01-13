#ifndef __RT_HPP_
#define __RT_HPP_

#if defined(__linux__)
#define LINUX
#include <unistd.h>
#include <dlfcn.h>
#elif defined(_WIN32) || defined(_WIN64)
#define WINDOWS
#include <Windows.h>
#endif

#include "utils.hpp"
#include "vec3.hpp"
#include "color.hpp"
#include "sphere.hpp"
#include "camera.hpp"
#include "material.hpp"

#include "window.hpp"

#endif // !__RT_HPP_
