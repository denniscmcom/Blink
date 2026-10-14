# VMA Vulkan allocator

Based on some information on the internet, the maximum number of allocations
from `vkAllocateMemory` could be as little as 4096. Right now, I'm not hitting
that limit but eventually I will. A solution for that in the use/implement an
allocator for Vulkan. One commonly used allocator is
<https://gpuopen.com/vulkan-memory-allocator/>
