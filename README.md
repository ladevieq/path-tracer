# path-tracer
Initially a CPU path tracer based on [_Ray Tracing in One Weekend_](https://raytracing.github.io/books/RayTracingInOneWeekend.html)

Rewritten using Vulkan API compute shaders to make it lighting fast.

## Result
![](https://github.com/ladevieq/path-tracer/blob/master/result.jpg?raw=true)

## TODO
* [x] Vulkan compute shader implementation
* [x] Viewer window
* [x] Support Windows
* [x] Realtime progressive rendering (accumalate samples)
* [x] Camera controller
* [ ] Add material types
* [ ] Triangulated meshes rendering (Vulkan raytracing pipeline)
