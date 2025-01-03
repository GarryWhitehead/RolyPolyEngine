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

#ifndef __VKAPI_DRIVER_H__
#define __VKAPI_DRIVER_H__

#include "commands.h"
#include "common.h"
#include "context.h"
#include "renderpass.h"
#include "resource_cache.h"
#include "staging_pool.h"

#define VKAPI_DRIVER_MAX_DRAW_COUNT 1000

#define VKAPI_SCRATCH_ARENA_SIZE 1 << 20
#define VKAPI_PERM_ARENA_SIZE 1 << 30

// Forward declarations.
typedef struct VkApiResourceCache vkapi_res_cache_t;
typedef struct VkApiDriver vkapi_driver_t;
typedef struct ProgramCache program_cache_t;
typedef struct FrameBufferCache vkapi_fb_cache_t;
typedef struct PipelineCache vkapi_pipeline_cache_t;
typedef struct DescriptorCache vkapi_desc_cache_t;
typedef struct SamplerCache vkapi_sampler_cache_t;
typedef struct VkApiSwapchain vkapi_swapchain_t;
typedef struct ShaderProgramBundle shader_prog_bundle_t;

enum BarrierType
{
    VKAPI_BARRIER_COMPUTE_TO_INDIRECT_CMD_READ,
    VKAPI_BARRIER_INDIRECT_CMD_READ_TO_COMPUTE
};

typedef struct VkApiDriver
{
    /// Current device context (instance, physical device, device).
    vkapi_context_t* context;
    /// VMA instance.
    VmaAllocator vma_allocator;
    /// Semaphore used to signal that the image is ready for presentation.
    VkSemaphore image_ready_signal;
    uint32_t image_index;

    vkapi_staging_pool_t* staging_pool;

    // Graphic queue commands.
    vkapi_commands_t* commands;
    // Compute queue commands (if the graphics and compute queue are the same, commited commands
    // will be in the same queue).
    vkapi_commands_t* compute_commands;

    /** Private **/
    /// Permanent arena space for the lifetime of this driver.
    arena_t _perm_arena;
    /// Small scratch arena for limited lifetime allocations. Should be passed as a copy to
    /// functions for scoping only for the lifetime of that function.
    arena_t _scratch_arena;

    vkapi_res_cache_t* res_cache;
    arena_dyn_array_t render_targets;

    program_cache_t* prog_manager;

    vkapi_fb_cache_t* framebuffer_cache;
    vkapi_pipeline_cache_t* pline_cache;
    vkapi_desc_cache_t* desc_cache;
    vkapi_sampler_cache_t* sampler_cache;

    uint64_t current_frame;

} vkapi_driver_t;

/**
 initialise the Vulkan driver - includes creating the abstract device,
 physical device, queues, etc.
 @param driver A pointer to the driver instance.
 @param surface An opaque pointer to a window surface. If NULL, then headless mode is assumed..
 @returns an error code specifying whether device creation was successful.
 */
int vkapi_driver_create_device(vkapi_driver_t* driver, VkSurfaceKHR surface);

/**
 Create a new driver instance - creates a Vulkan instance for this device.
 @param driver A pointer to the driver instance.
 @param instance_ext Vulkan instance extensions parsed from GLFW.
 @param ext_count The instance extension count.
 @param [out] driver The new initialised driver instance.
 @returns The error code returned by the call. If not VKAPI_SUCCESS, then the driver instance will
 be uninitialised.
 */
vkapi_driver_t* vkapi_driver_init(const char** instance_ext, uint32_t ext_count, int* errror_code);

/**
 Deallocate all resources associated with the vulkan api.
 @param driver A pointer to the driver instance.
 */
void vkapi_driver_shutdown(vkapi_driver_t* driver, VkSurfaceKHR surface);

/**
 Get the supported Vk depth format for this device.
 @param driver A pointer to the driver instance.
 @return The supported depth format.
 */
VkFormat vkapi_driver_get_supported_depth_format(vkapi_driver_t* driver);

vkapi_rt_handle_t vkapi_driver_create_rt(
    vkapi_driver_t* driver,
    bool multiView,
    math_vec4f clear_col,
    vkapi_attach_info_t* colours,
    vkapi_attach_info_t depth,
    vkapi_attach_info_t stencil);

bool vkapi_driver_begin_frame(vkapi_driver_t* driver, vkapi_swapchain_t* sc);
void vkapi_driver_end_frame(vkapi_driver_t* driver, vkapi_swapchain_t* sc);

void vkapi_driver_begin_rpass(
    vkapi_driver_t* driver,
    VkCommandBuffer cmds,
    vkapi_render_pass_data_t* data,
    vkapi_rt_handle_t* rt_handle);
void vkapi_driver_end_rpass(VkCommandBuffer cmds);

void vkapi_driver_bind_vertex_buffer(
    vkapi_driver_t* driver, buffer_handle_t vb_handle, uint32_t binding);
void vkapi_driver_bind_index_buffer(vkapi_driver_t* driver, buffer_handle_t ib_handle);

void vkapi_driver_bind_gfx_pipeline(vkapi_driver_t* driver, shader_prog_bundle_t* bundle);

void vkapi_driver_map_gpu_buffer(
    vkapi_driver_t* driver, buffer_handle_t h, size_t size, size_t offset, void* data);

void vkapi_driver_set_push_constant(
    vkapi_driver_t* driver, void* data, size_t size, VkShaderStageFlags stage);

void vkapi_driver_draw(vkapi_driver_t* driver, uint32_t vert_count, int32_t vertex_offset);
void vkapi_driver_draw_indexed(
    vkapi_driver_t* driver, uint32_t index_count, int32_t vertex_offset, int32_t index_offset);
void vkapi_driver_draw_indirect_indexed(
    vkapi_driver_t* driver,
    buffer_handle_t indirect_cmd_buffer,
    uint32_t offset,
    buffer_handle_t cmd_count_buffer,
    uint32_t draw_count_offset,
    uint32_t stride);

void vkapi_driver_begin_cond_render(
    vkapi_driver_t* driver, buffer_handle_t cond_buffer, int32_t offset);

void vkapi_driver_dispatch_compute(
    vkapi_driver_t* driver,
    shader_prog_bundle_t* bundle,
    uint32_t x_work_count,
    uint32_t y_work_count,
    uint32_t z_work_count);

void vkapi_driver_draw_quad(vkapi_driver_t* driver, shader_prog_bundle_t* bundle);

void vkapi_driver_clear_gpu_buffer(
    vkapi_driver_t* driver, vkapi_cmdbuffer_t* cmd_buffer, buffer_handle_t handle);

void vkapi_driver_apply_global_barrier(
    vkapi_driver_t* driver,
    VkPipelineStageFlags src_stage,
    VkPipelineStageFlags dst_stage,
    VkAccessFlags src_access,
    VkAccessFlags dst_access);
void vkapi_driver_acquire_buffer_barrier(
    vkapi_driver_t* driver, vkapi_cmdbuffer_t*, buffer_handle_t handle, enum BarrierType type);
void vkapi_driver_release_buffer_barrier(
    vkapi_driver_t* driver, vkapi_cmdbuffer_t*, buffer_handle_t handle, enum BarrierType type);

void vkapi_driver_flush_compute_cmds(vkapi_driver_t* driver);
void vkapi_driver_flush_gfx_cmds(vkapi_driver_t* driver);
vkapi_cmdbuffer_t* vkapi_driver_get_compute_cmds(vkapi_driver_t* driver);
vkapi_cmdbuffer_t* vkapi_driver_get_gfx_cmds(vkapi_driver_t* driver);

void vkapi_driver_gc(vkapi_driver_t* driver);

#endif