# Agents

Read `README.md` first. It defines the architecture, layer dependencies,
conventions and coding rules, and they all apply here.

## Building

- Configure: `cmake --preset ninja`
- Build: `cmake --build --preset ninja-debug`
- Format: run `clang-format` on changed files before finishing.
- Lint: run `clang-tidy` on changed files and fix new warnings.

## Reference material

Third-party libraries live in `External/`, with their source and, where
available, their documentation or specification. Papers for the techniques
the engine implements live in `Research/`. Treat both as the only source of
truth.

- Before answering a question or writing code that involves a third-party
  API or an implemented technique, read the relevant files in `External/`
  or `Research/`. Do not rely on memory: APIs change between versions, and
  techniques are often described differently from how they appear in the
  paper the engine follows.
- When implementing or changing code based on a paper, follow the paper's
  notation, equations and terminology, and reference the paper and section
  in a comment.
- If the answer can't be found there, say so instead of guessing.
- Mention which file the answer came from.
- Slang looks like HLSL but isn't identical. Check the Slang specification
  before assuming an HLSL feature is supported, and point out differences.
- Where a library has a separate specification or documentation folder,
  prefer it over reading the source code.

## Working in this codebase

- Respect the layer order in `README.md`. If a change seems to need a
  lower layer to depend on a higher one, stop and ask.
- Keep changes small and focused. Do not refactor unrelated code without asking.
- Only use comments when they provide meaningful information. Avoid verbose
  comments and duplicated information.

## Boundaries

- Do NOT add third-party libraries to `External/` without asking.
- Do NOT modify `Research/`.
- Do NOT commit. Commits are made by hand.
- Do NOT use GIT commands to explore diffs. Version control is SVN.
- Do NOT suggest deferred rendering.
- Do NOT suggest RAII wrappers around Vulkan handles.
- Do NOT suggest switching to GLSL/HLSL.
- Do NOT compile individual files, always use the project build command.
