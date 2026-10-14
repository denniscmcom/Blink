# String module boundaries

There are systems that depend heavily to each other, so changes in one forces
changes in the other. For example, the renderer and editor depend on structures
defined by the world module.

Also, the game should access the renderer. This could be solved by compiling
each module as an independent static library.
