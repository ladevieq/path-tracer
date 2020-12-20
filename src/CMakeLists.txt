include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(path-tracer main.cpp vec3.cpp color.cpp ray.cpp sphere.cpp hittable_list.cpp camera.cpp)
