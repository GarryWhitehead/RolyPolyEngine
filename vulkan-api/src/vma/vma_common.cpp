#include "vma_common.h"

#define VOLK_IMPLEMENTATION 1
#include <volk.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#undef VMA_VULKAN_VERSION
#define VMA_VULKAN_VERSION 1002000
#define VMA_IMPLEMENTATION 1
#define VK_NO_PROTOTYPE
#include <vk_mem_alloc.h>

