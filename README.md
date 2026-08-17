# The Blink Game Engine

Game engine written in C++ for Windows with a Vulkan Physically Based Renderer
(PBR) renderer. Currently in active development.

![Blink Editor](https://public.denniscm.com/repositories/blink/editor_1.png)

## Features
 
- PBR material pipeline (albedo, normal map, occlusion, roughness, metallic).
- Support for point lights.
- Hierarchical scene graph with parent-child node relationships.
- Asset caching system for meshes, materials, textures, and shaders.
- Platform layer for Windows using the Win32 API.
- Integrated editor with two modes (world, and material) built with ImGui.
- Custom math library.
- Asset compilation to custom binary formats.
- Data-oriented approach.
- Focused in code readability, maintenance, and performance.

## Building
 
Requires CMake, a Vulkan SDK, and a C++20 compiler (tested with MSVC and Clang
on Windows).
 
```
cmake --preset <preset-name> 
cmake --build --preset <preset-name>
```
 
## Blog
 
I write about the development process and technical decisions on my blog:
[denniscm.com/blog](https://denniscm.com/blog/)
 
## Note

This repository is a mirror of an SVN repository. Commits are periodic syncs and
may contain multiple revisions. Assets, binary files, and game code are not
included in this repository, so the engine will not compile or run out of the
box. The purpose of this repository is to share the engine source code.
