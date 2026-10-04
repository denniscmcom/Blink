# The Blink Game Engine

*In active development.*   

Blink is an experimental, lightweight game engine for Windows, built for dynamic
open-world games. It aims for fully dynamic lighting with global illumination
and support for destruction, with performance as a priority. Above all, it
should stay small, simple and easy to work with rather than trying to do
everything.

## Screenshots

### World streaming with terrain LOD

![Terrain LOD](https://public.denniscm.com/repositories/blink/terrain_lod.png)

### Material preview

![Material Preview](https://public.denniscm.com/repositories/blink/material_preview.png)

### Dynamic sky and atmosphere

![Skybox 1](https://public.denniscm.com/repositories/blink/skybox_1.png)
![Skybox 2](https://public.denniscm.com/repositories/blink/skybox_2.png)
![Skybox 3](https://public.denniscm.com/repositories/blink/skybox_3.png)

## Features

Implemented features are still evolving and may change.

- Physically-based forward rendering
- Dynamic sky and atmosphere
- World streaming with terrain LOD
- Asset compilation to optimized binary formats
- Material authoring tools
- World authoring tools
- Entity and gameplay framework

## Architecture

Each layer may only depend on the layers above it in this list:

- `Engine/`:
  - `Platform/`: platform-specific code, logging, assertions, and allocators
  - `Core/`: math library, data structures, and algorithms
  - `Input/`: high-level input system
  - `Resource/`: asset management and storage
  - `Scene/`: low-level hierarchical representation of a world
  - `World/`: high-level gameplay representation of a world
  - `Renderer/`: Vulkan renderer
    - `Lifetime/`: Vulkan resources grouped by lifetime
    - `Resource/`: high-level resources
    - `Shaders/`: shader code
  - `Editor/`: editor
- `Compiler/`: asset compiler
- `Launcher/`: main entry point that links `Engine` and `Game`.

Engine scripts and libraries:

- `CMake/`: CMake shared modules
- `External/`: third-party libraries and references

Game resources:

- `Assets/`: assets
- `Game/`: gameplay code

Other:

- `Research/`: reference papers for the techniques used
- `Demos/`: small sample projects that show specific features

## Coding standard

The goal of this coding standard is to reduce the mental effort when
implementing new systems.

### Naming, comments and formatting

Names are written in full, correct English words with no abbreviations.
Established acronyms such as GPU, LUT, PBR are fine. 

Booleans read as a question (`is_visible`, `has_shadow`); functions read as an
action (`empty`, `compute_matrix`).

- **Folder**: `My-Folder`
- **Files**: `My_File`
- **Namespace**: `my_namespace`
- **Types**: `My_Type`
- **Functions**: `my_function`
- **Enum values**: `ENUM_VALUE`
- **Macros**: `MY_MACRO`
- **Variables**: `my_variable`
- **Constants**: inside blocks `my_constant`; at file level `MY_CONSTANT`
- **Private fields**: add `m_` preffix

Use Doxygen-style comments to describe the behaviour of functions and types. Use
regular inline comments to explain control flow, design decisions, or anything
else that helps a reader understand what the code does and why.

Formatting and naming style are defined by `.clang-format` and `.clang-tidy` at
the repository root.

### Languages and dependencies

Blink is written in standard C++20 with no compiler extensions, built with
CMake, and targets Windows only.

- **No STL**: containers, strings and utility types are the engine's own
- **No exceptions**: errors are returned as values
- **No inheritance**: behaviour is shared through composition
- **C headers**: use `stdio.h` instead of `cstdio`
- **Limit templates**: acceptable for making the simpler. Prefer a concrete type
    public engine interfaces
- **Const-correctness**: mark everything `const` that does not change
- **Explicit, readable code**: prefer the obvious version over the clever one

### Memory and ownership

Every resource has exactly one owner, and the owner's destructor releases it. 

All memory is allocated through the engine's allocators, which come in two
kinds.

- **Arena allocator**: Reserves its memory up front and frees it all at once,
    with no way to release individual allocations. Used for data that only
    lives for one frame
- **Heap allocator**: Allocates from the operating system as usual. Used for
    data that lives across frames

### Types, functions and RAII

Temporary convention to keep iteration frictionless, to be revisited once the
engine stabilises.

- **Types**: types are plain `struct`'s with public fields an no RAII
- **Functions**: operations on types are free functions
- **RAII**: resource creation and destruction is done manually

### Conventions

- Coordinate system: left-handed, Y-up
- Build system: CMake with presets
- Version control: Subversion (SVN)

## AI

I use AI tools to speed up development and save mental energy for the parts that
matter. Every piece of AI-generated code in this repository has been reviewed
and understood before being committed. Code that nobody understands can't be
maintained, and whoever works on the engine next needs to be able to reason
about it, so understanding it is a hard requirement.

## Contributions

I appreciate your interest in contributing to Blink. However, this is currently
a learning project and I'm not accepting any changes.
 
## Note

This repository is a mirror of an SVN repository. Commits are periodic syncs and
may contain multiple revisions. Assets, binary files, and game code are not
included in this repository, so the engine will not compile or run out of the
box. The purpose of this repository is to share the engine source code.
