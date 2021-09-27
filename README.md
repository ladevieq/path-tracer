# Vulkan monte carlo path tracer
Initially a CPU path tracer based on [_Ray Tracing in One Weekend_](https://raytracing.github.io/books/RayTracingInOneWeekend.html)

Rewritten using Vulkan API compute shaders to make it lighting fast.

![bistrot](https://user-images.githubusercontent.com/7492041/132995994-addf8590-2bd8-47a4-a5b8-5aab1e1df19c.png)

## Features

* [x] Vulkan compute shader implementation
* [x] Bindless rendering API over Vulkan
* [x] Support different window systems (Win32, xcb)
* [x] Realtime rendering
* [x] Multisampling (temporal accumulation)

![accumulation](https://user-images.githubusercontent.com/7492041/132996189-84098bfe-130d-44a5-abec-f31466f8da8a.gif)

* [x] GLTF 2.0 scenes![image](https://user-images.githubusercontent.com/7492041/132995553-65a0cfa6-f18f-4375-9996-2695a83dafb2.png)

* [x] BRDF
* [x] Importance sampling 

* [x] Camera controller 

![camera](https://user-images.githubusercontent.com/7492041/121815035-bc6caf00-cc63-11eb-95da-c88796f9055a.gif)
* [x] ImGui backend
* [x] Debugging window
* [x] Tonemapping
* [x] BVH based on SAH with debugging tools (Graphviz exporter, visual debugger)

![bvh_debug](https://user-images.githubusercontent.com/7492041/132996179-d7030ae3-5964-4647-8503-6aae5d129134.gif)


## TODO
* [ ]  Vulkan RT
