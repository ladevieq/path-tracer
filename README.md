# path-tracer
Initially a CPU path tracer based on ray tracing in one weekend book

The new implementation is using Vulkan compute shaders to generate an image of a path traced scene


## Features
* PPM image format
* Three types of material (lambertian, metalic, dielectric)
* Customisable camera (focus plan, aperture)
* Compute everything on GPU

## Result
![](https://github.com/ladevieq/path-tracer/blob/master/result.jpg?raw=true)

## TODO
* [ ] More material types
* [ ] Support GLTF format
* [ ] Triangulated meshes rendering
* [x] GPU implementation
* [ ] Realtime
