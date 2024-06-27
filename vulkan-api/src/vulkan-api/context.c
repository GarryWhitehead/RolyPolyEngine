/* Copyright (c) 2024 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "context.h"

#include "error_codes.h"

#include <log.h>
#include <string.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT obj_type,
    uint64_t obj,
    size_t loc,
    int32_t code,
    const char* layer_prefix,
    const char* msg,
    void* data)
{
    RPE_UNUSED(obj_type);
    RPE_UNUSED(obj);
    RPE_UNUSED(loc);
    RPE_UNUSED(data);

    // Ignore access mask false positive.
    if (strcmp(layer_prefix, "DS") == 0 && code == 10)
    {
        return VK_FALSE;
    }

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        log_info("Vulkan Error: %s: %s\n", layer_prefix, msg);
        return VK_FALSE;
    }
    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        log_info("Vulkan Warning: %s: %s\n", layer_prefix, msg);
        return VK_FALSE;
    }
    else
    {
        // Just output this as a log.
        log_info("Vulkan Information: %s: %s\n", layer_prefix, msg);
    }
    return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessenger(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data)
{
    RPE_UNUSED(user_data);

    switch (severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            {
                log_info("Validation Error: %s\n", data->pMessage);
            }
            else
            {
                log_info("Other Error: %s\n", data->pMessage);
            }
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            {
                log_info("Validation Warning: %s\n", data->pMessage);
            }
            else
            {
                log_info("Other Warning: %s\n", data->pMessage);
            }
            break;

        default:
            break;
    }

    bool log_obj_names = false;
    for (uint32_t i = 0; i < data->objectCount; i++)
    {
        const char* name = data->pObjects[i].pObjectName;
        if (name)
        {
            log_obj_names = true;
            break;
        }
    }

    if (log_obj_names)
    {
        for (uint32_t i = 0; i < data->objectCount; i++)
        {
            const char* name = data->pObjects[i].pObjectName;
            log_info("  Object #%u: %s\n", i, name ? name : "N/A");
        }
    }

    return VK_FALSE;
}

void vkapi_context_init(arena_t* perm_arena, vkapi_context_t* new_context)
{
    memset(new_context, 0, sizeof(vkapi_context_t));

    new_context->queue_info.compute = VK_QUEUE_FAMILY_IGNORED;
    new_context->queue_info.graphics = VK_QUEUE_FAMILY_IGNORED;
    new_context->queue_info.present = VK_QUEUE_FAMILY_IGNORED;

    new_context->extensions.has_debug_utils = false;
    new_context->extensions.has_external_capabilities = false;
    new_context->extensions.has_multi_view = false;
    new_context->extensions.has_physical_dev_props2 = false;

    new_context->instance = VK_NULL_HANDLE;
    new_context->physical = VK_NULL_HANDLE;
    new_context->device = VK_NULL_HANDLE;

    MAKE_DYN_ARRAY(char*, perm_arena, 10, &new_context->req_layers);
}

void vkapi_context_shutdown(vkapi_context_t* context)
{
    vkDestroyDevice(context->device, VK_NULL_HANDLE);
    vkDestroyInstance(context->instance, VK_NULL_HANDLE);
}

bool vkapi_find_ext_props(const char* name, const VkExtensionProperties* props, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        if (strcmp(props[i].extensionName, name) == 0)
        {
            return true;
        }
    }
    return false;
};

int vkapi_context_prep_extensions(
    vkapi_context_t* context,
    arena_dyn_array_t* ext_array,
    const char** glfw_exts,
    uint32_t glfw_ext_count,
    VkExtensionProperties* dev_ext_props,
    uint32_t dev_ext_prop_count,
    arena_t* arena)
{
    for (uint32_t i = 0; i < glfw_ext_count; ++i)
    {
        if (!vkapi_find_ext_props(glfw_exts[i], dev_ext_props, dev_ext_prop_count))
        {
            return VKAPI_ERROR_MISSING_GFLW_EXT;
        }
        DYN_ARRAY_APPEND_CHAR(arena, ext_array, glfw_exts[i]);
    }

    if (vkapi_find_ext_props(
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
            dev_ext_props,
            dev_ext_prop_count))
    {
        DYN_ARRAY_APPEND_CHAR(
            arena, ext_array, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        context->extensions.has_physical_dev_props2 = true;

        if (vkapi_find_ext_props(
                VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
                dev_ext_props,
                dev_ext_prop_count) &&
            vkapi_find_ext_props(
                VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
                dev_ext_props,
                dev_ext_prop_count))
        {
            DYN_ARRAY_APPEND_CHAR(
                arena, ext_array, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
            DYN_ARRAY_APPEND_CHAR(
                arena, ext_array, VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
            context->extensions.has_external_capabilities = true;
        }
    }
    if (vkapi_find_ext_props(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, dev_ext_props, dev_ext_prop_count))
    {
        DYN_ARRAY_APPEND_CHAR(arena, ext_array, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        context->extensions.has_debug_utils = true;
    }
    if (vkapi_find_ext_props(VK_KHR_MULTIVIEW_EXTENSION_NAME, dev_ext_props, dev_ext_prop_count))
    {
        DYN_ARRAY_APPEND_CHAR(arena, ext_array, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        context->extensions.has_multi_view = true;
    }

#ifdef VULKAN_VALIDATION_DEBUG
    // if debug utils isn't supported, try debug report
    if (!context->extensions.has_debug_utils &&
        vkapi_find_ext_props(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, dev_ext_props, dev_ext_prop_count))
    {
        DYN_ARRAY_APPEND_CHAR(arena, ext_array, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
#endif
    return VKAPI_SUCCESS;
}

bool vkapi_find_layer_ext(const char* name, VkLayerProperties* layer_props, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        if (strcmp(layer_props[i].layerName, name) == 0)
        {
            return true;
        }
    }
    return false;
}

int vkapi_context_create_instance(
    vkapi_context_t* context,
    const char** glfw_ext,
    uint32_t glfw_ext_count,
    arena_t* arena,
    arena_t* scratch_arena)
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "RolyPolyEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
    appInfo.pEngineName = "RolyPolyEngine";
    appInfo.apiVersion = VK_API_VERSION_1_2;

    // glfw extensions
    arena_dyn_array_t ext_arr;
    MAKE_DYN_ARRAY(char[VK_MAX_DESCRIPTION_SIZE], arena, 30, &ext_arr);

    // extension properties
    uint32_t ext_count = 0;
    vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &ext_count, VK_NULL_HANDLE);
    VkExtensionProperties* ext_prop_arr =
        ARENA_MAKE_ARRAY(scratch_arena, VkExtensionProperties, ext_count, 0);
    vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &ext_count, ext_prop_arr);

    int ret = vkapi_context_prep_extensions(
        context, &ext_arr, glfw_ext, glfw_ext_count, ext_prop_arr, ext_count, arena);
    if (ret != VKAPI_SUCCESS)
    {
        return ret;
    }

    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, VK_NULL_HANDLE);
    VkLayerProperties* layer_prop_arr =
        ARENA_MAKE_ARRAY(scratch_arena, VkLayerProperties, layer_count, 0);
    vkEnumerateInstanceLayerProperties(&layer_count, layer_prop_arr);

#ifdef VULKAN_VALIDATION_DEBUG
    if (vkapi_find_layer_ext("VK_LAYER_KHRONOS_validation", layer_prop_arr, layer_count))
    {
        arena_dyn_array_t* req_layers_ptr = &context->req_layers;
        DYN_ARRAY_APPEND_CHAR(arena, req_layers_ptr, "VK_LAYER_KHRONOS_validation");
    }
    else
    {
        log_warn("Unable to find validation standard layers.");
    }
#endif
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = context->req_layers.size;
    createInfo.ppEnabledLayerNames = context->req_layers.data;
    createInfo.enabledExtensionCount = ext_arr.size;
    createInfo.ppEnabledExtensionNames = (const char**)ext_arr.data;
    VK_CHECK_RESULT(vkCreateInstance(&createInfo, VK_NULL_HANDLE, &context->instance));

#ifdef VULKAN_VALIDATION_DEBUG
    // For some reason, on windows the instance needs to be NULL to successfully
    // get the proc address, only Mac and Linux the VkInstance needs to be
    // passed.
#ifdef WIN32
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback =
        (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
            VK_NULL_HANDLE, "vkCreateDebugReportCallbackEXT");
    PFN_vkCreateDebugUtilsMessengerEXT DebugUtilsMessengerCallback =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            VK_NULL_HANDLE, "vkCreateDebugUtilsMessengerEXT");
#else
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback =
        (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
            context->instance, "vkCreateDebugReportCallbackEXT");
    PFN_vkCreateDebugUtilsMessengerEXT DebugUtilsMessengerCallback =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            context->instance, "vkCreateDebugUtilsMessengerEXT");
#endif

    if (context->extensions.has_debug_utils)
    {
        VkDebugUtilsMessengerCreateInfoEXT dbg_create_info = {};
        dbg_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dbg_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        dbg_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        dbg_create_info.pfnUserCallback = DebugMessenger;

        VK_CHECK_RESULT(DebugUtilsMessengerCallback(
            context->instance, &dbg_create_info, VK_NULL_HANDLE, &context->debug_messenger))
    }
    else if (vkapi_find_ext_props(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, ext_prop_arr, ext_count))
    {
        VkDebugReportCallbackCreateInfoEXT cb_create_info = {};
        cb_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        cb_create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        cb_create_info.pfnCallback = DebugCallback;

        VK_CHECK_RESULT(CreateDebugReportCallback(
            context->instance, &cb_create_info, VK_NULL_HANDLE, &context->debug_callback));
    }
#endif
    arena_reset(scratch_arena);
    return VKAPI_SUCCESS;
}

int vkapi_context_prepare_device(
    vkapi_context_t* context, VkSurfaceKHR win_surface, arena_t* scratch_arena)
{
    if (!context->instance)
    {
        return VKAPI_ERROR_NO_VK_INSTANCE;
    }

    // Find a suitable gpu - at the moment this is pretty basic - find a gpu and
    // that will do. In the future, find the best match.
    uint32_t phys_dev_count = 0;
    vkEnumeratePhysicalDevices(context->instance, &phys_dev_count, VK_NULL_HANDLE);
    VkPhysicalDevice* phys_dev_arr =
        ARENA_MAKE_ARRAY(scratch_arena, VkPhysicalDevice, phys_dev_count, 0);
    vkEnumeratePhysicalDevices(context->instance, &phys_dev_count, phys_dev_arr);

    for (uint32_t i = 0; i < phys_dev_count; ++i)
    {
        VkPhysicalDevice* gpu = &phys_dev_arr[i];
        if (gpu)
        {
            // Prefer discrete GPU over integrated.
            // TODO: make this an option.
            VkPhysicalDeviceProperties prop;
            vkGetPhysicalDeviceProperties(*gpu, &prop);
            if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                context->physical = *gpu;
                break;
            }
        }
    }

    if (!context->physical)
    {
        return VKAPI_ERROR_NO_SUPPORTED_GPU;
    }

    // Also get all the device extensions for querying later.
    uint32_t dev_ext_prop_count = 0;
    vkEnumerateDeviceExtensionProperties(
        context->physical, VK_NULL_HANDLE, &dev_ext_prop_count, VK_NULL_HANDLE);
    VkExtensionProperties* dev_ext_prop_arr =
        ARENA_MAKE_ARRAY(scratch_arena, VkExtensionProperties, dev_ext_prop_count, 0);
    vkEnumerateDeviceExtensionProperties(
        context->physical, VK_NULL_HANDLE, &dev_ext_prop_count, dev_ext_prop_arr);

    // Find queues for this gpu.
    uint32_t queue_prop_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context->physical, &queue_prop_count, VK_NULL_HANDLE);
    VkQueueFamilyProperties* queue_prop_arr =
        ARENA_MAKE_ARRAY(scratch_arena, VkQueueFamilyProperties, queue_prop_count, 0);
    vkGetPhysicalDeviceQueueFamilyProperties(context->physical, &queue_prop_count, queue_prop_arr);

    // Graphics queue setup.
    for (uint32_t c = 0; c < queue_prop_count; ++c)
    {
        if (queue_prop_arr[c].queueCount && queue_prop_arr[c].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            context->queue_info.graphics = c;
            break;
        }
    }

    if (context->queue_info.graphics == VK_QUEUE_FAMILY_IGNORED)
    {
        return VKAPI_ERROR_NO_GRAPHIC_QUEUE;
    }

    float queue_priority = 1.0f;
    uint32_t queue_count = 1;
    VkDeviceQueueCreateInfo queue_info[3] = {};
    queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[0].queueFamilyIndex = context->queue_info.graphics;
    queue_info[0].queueCount = 1;
    queue_info[0].pQueuePriorities = &queue_priority;

    if (win_surface)
    {
        // The ideal situation is if the graphics and presentation queues are the
        // same.
        VkBool32 has_present_queue = false;
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(
            context->physical, context->queue_info.graphics, win_surface, &has_present_queue))
        if (has_present_queue)
        {
            context->queue_info.present = context->queue_info.graphics;
        }
        else
        {
            // Else use a separate presentation queue.
            for (uint32_t c = 0; c < queue_prop_count; ++c)
            {
                VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(
                    context->physical, c, win_surface, &has_present_queue))
                if (queue_prop_arr[c].queueCount > 0 && has_present_queue)
                {
                    context->queue_info.present = c;
                    break;
                }
            }

            // Presentation queue is compulsory if a swapchain is specified.
            if (context->queue_info.present == VK_QUEUE_FAMILY_IGNORED)
            {
                return VKAPI_ERROR_PRESENT_QUEUE_NOT_SUPPORTED;
            }

            queue_info[queue_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[queue_count].queueFamilyIndex = context->queue_info.present;
            queue_info[queue_count].queueCount = 1;
            queue_info[queue_count].pQueuePriorities = &queue_priority;
            ++queue_count;
        }
    }

    // Compute queue setup.
    for (uint32_t c = 0; c < queue_prop_count; ++c)
    {
        if (queue_prop_arr[c].queueCount > 0 && c != context->queue_info.present &&
            c != context->queue_info.graphics &&
            queue_prop_arr[c].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            context->queue_info.compute = c;
            if (context->queue_info.compute != context->queue_info.graphics)
            {
                queue_info[queue_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_info[queue_count].queueFamilyIndex = context->queue_info.compute;
                queue_info[queue_count].queueCount = 1;
                queue_info[queue_count].pQueuePriorities = &queue_priority;
                ++queue_count;
                break;
            }
        }
    }

    // Enable required device features.
    VkPhysicalDeviceMultiviewFeatures mv_features = {};
    mv_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
    mv_features.multiview = VK_TRUE;
    mv_features.multiviewGeometryShader = VK_FALSE;
    mv_features.multiviewTessellationShader = VK_FALSE;

    VkPhysicalDeviceFeatures2 req_features2 = {};
    req_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    req_features2.pNext = &mv_features;

    VkPhysicalDeviceFeatures dev_features;
    vkGetPhysicalDeviceFeatures(context->physical, &dev_features);
    if (dev_features.textureCompressionETC2)
    {
        req_features2.features.textureCompressionETC2 = VK_TRUE;
    }
    if (dev_features.textureCompressionBC)
    {
        req_features2.features.textureCompressionBC = VK_TRUE;
    }
    if (dev_features.samplerAnisotropy)
    {
        req_features2.features.samplerAnisotropy = VK_TRUE;
    }
    if (dev_features.tessellationShader)
    {
        req_features2.features.tessellationShader = VK_TRUE;
    }
    if (dev_features.geometryShader)
    {
        req_features2.features.geometryShader = VK_TRUE;
    }
    if (dev_features.shaderStorageImageExtendedFormats)
    {
        req_features2.features.shaderStorageImageExtendedFormats = VK_TRUE;
    }
    if (dev_features.multiViewport)
    {
        req_features2.features.multiViewport = VK_TRUE;
    }

    const char* req_extensions = NULL;
    uint32_t req_ext_count = 0;
    if (win_surface)
    {
        // A swapchain extension must be present.
        if (!vkapi_find_ext_props(
                VK_KHR_SWAPCHAIN_EXTENSION_NAME, dev_ext_prop_arr, dev_ext_prop_count))
        {
            return VKAPI_ERROR_SWAPCHAIN_NOT_FOUND;
        }
        req_extensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        ++req_ext_count;
    }

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = queue_count;
    create_info.pQueueCreateInfos = queue_info;
    create_info.enabledLayerCount = context->req_layers.size;
    create_info.ppEnabledLayerNames = context->req_layers.data;
    create_info.enabledExtensionCount = req_ext_count;
    create_info.ppEnabledExtensionNames = &req_extensions;
    create_info.pEnabledFeatures = VK_NULL_HANDLE;
    create_info.pNext = (void*)&req_features2;

    VK_CHECK_RESULT(
        vkCreateDevice(context->physical, &create_info, VK_NULL_HANDLE, &context->device));

    // Print out the specifications of the selected device.
    VkPhysicalDeviceProperties phys_dev_props;
    vkGetPhysicalDeviceProperties(context->physical, &phys_dev_props);

    const uint32_t driverVersion = phys_dev_props.driverVersion;
    const uint32_t vendorID = phys_dev_props.vendorID;
    const char* deviceName = phys_dev_props.deviceName;

    log_info(
        "\n\nDevice name: %s\nDriver version: %u\nVendor ID: %i\n",
        deviceName,
        driverVersion,
        vendorID);

    vkGetDeviceQueue(context->device, context->queue_info.graphics, 0, &context->graphics_queue);
    vkGetDeviceQueue(context->device, context->queue_info.compute, 0, &context->compute_queue);
    if (context->queue_info.present != VK_QUEUE_FAMILY_IGNORED)
    {
        vkGetDeviceQueue(context->device, context->queue_info.present, 0, &context->present_queue);
    }
    arena_reset(scratch_arena);

    return VKAPI_SUCCESS;
}

uint32_t
vkapi_context_select_mem_type(vkapi_context_t* context, uint32_t flags, VkMemoryPropertyFlags reqs)
{
    VkPhysicalDeviceMemoryProperties phys_dev_mem_props;
    vkGetPhysicalDeviceMemoryProperties(context->physical, &phys_dev_mem_props);

    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
    {
        if (flags & 1)
        {
            if ((phys_dev_mem_props.memoryTypes[i].propertyFlags & reqs) == reqs)
            {
                return i;
            }
        }
        flags >>= 1;
    }

    log_error("Unable to find required memory type.");
    return UINT32_MAX;
}
