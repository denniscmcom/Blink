# Resource reference counting

Right now, a resource is cached the first time it's loaded, so that following
loads won't read the fila and load the data again. Releasing a resource doesn't
release it at the moment because I'm using an arena allocator.

I would like to implement reference counting, so that each load call increases
the counter and each unload call decreases it. When the counter reaches zero,
the resource is deallocated from memory.
