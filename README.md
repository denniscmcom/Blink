# The Blink Game Engine

Game engine written in C++ with a Vulkan-based renderer. Currently in active
development.

![Blink Editor](https://public.denniscm.com/repositories/blink/editor.jpg)

## Features
 
- Renderer: Vulkan renderer with Blinn-Phong lighting and separate pipelines for
  meshes and lights
- Scene Graph: Hierarchical scene graph with parent/child node relationships
- Entity System: Actor, Camera, and Prop entities managed through a pool
  allocator with generational IDs
- Resource System: Caching for meshes, materials, textures, and shaders with
  FNV-1a hash-based deduplication
- Platform Layer: Win32 windowing and input system with raw mouse input and
  keyboard edge detection
- Editor: Integrated editor with outliner, console, stats panel, and editor
  camera (built with ImGui)
- Math Library: Custom vector and matrix types
- Asset Pipeline: Resource loading with OBJ mesh parsing and material support

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
