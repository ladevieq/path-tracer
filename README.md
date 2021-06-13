# Vulkan path-tracer
Initially a CPU path tracer based on [_Ray Tracing in One Weekend_](https://raytracing.github.io/books/RayTracingInOneWeekend.html)

Rewritten using Vulkan API compute shaders to make it lighting fast.

![image](https://user-images.githubusercontent.com/7492041/121815427-05256780-cc66-11eb-9958-7c6b4e705375.png)

## Features

* [x] Vulkan compute shader implementation
* [x] Rendering API over Vulkan
* [x] Support different window systems (Win32, xcb)
* [x] Realtime rendering
* [x] Multisampling (temporal accumulation)

![accumulation](https://user-images.githubusercontent.com/7492041/121815317-5719bd80-cc65-11eb-8d07-3e3645fc098e.gif)
* [x] Camera controller 

![camera](https://user-images.githubusercontent.com/7492041/121815035-bc6caf00-cc63-11eb-95da-c88796f9055a.gif)
* [x] ImGui backend
* [x] Debugging window
* [x] Tonemapping
* [x] BVH with debugging tools (Graphviz exporter, visual debugger)

![bvh_debug](https://user-images.githubusercontent.com/7492041/121815003-8a5b4d00-cc63-11eb-85b4-6d82957fd935.gif)

## Doing

* [ ] Optimizing BVH implementation (optimal splitting axis, cache coherent memory layout)

## TODO

* [ ] Add material types
* [ ] Triangulated meshes rendering (explore Vulkan raytracing pipeline)
* [ ] Loading GLTF 2.0 scenes
