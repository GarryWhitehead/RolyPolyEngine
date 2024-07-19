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

#include "private_engine.h"

#include "rpe/engine.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/error_codes.h"

#include <log.h>
#include <stdlib.h>

rpe_engine_t* rpe_engine_create(vkapi_driver_t* driver)
{
    assert(driver);

    rpe_engine_t* instance = calloc(1, sizeof(struct Engine));
    assert(instance);

    instance->driver = driver;
    instance->swap_chain_count = 0;
    int err = arena_new(RPE_ENGINE_SCRATCH_ARENA_SIZE, &instance->scratch_arena);
    assert(err == ARENA_SUCCESS);
    err = arena_new(RPE_ENGINE_PERM_ARENA_SIZE, &instance->perm_arena);
    assert(err == ARENA_SUCCESS);

    return instance;
}

void rpe_engine_shutdown(rpe_engine_t* engine)
{
    vkapi_driver_shutdown(engine->driver);

    for (uint32_t i = 0; i < engine->swap_chain_count; ++i)
    {
        vkapi_swapchain_destroy(&engine->driver->context, &engine->swap_chains[i]);
    }
    arena_release(&engine->perm_arena);
    arena_release(&engine->scratch_arena);
    free(engine);
    engine = NULL;
}

swapchain_handle_t* rpe_engine_create_swapchain(
    rpe_engine_t* engine, VkSurfaceKHR surface, uint32_t width, uint32_t height)
{
    if (engine->swap_chain_count >= RPE_ENGINE_MAX_SWAPCHAIN_COUNT)
    {
        log_error("Max swapchain limit reached.");
        return NULL;
    }

    engine->swap_chains[engine->swap_chain_count] = vkapi_swapchain_init();
    int err = vkapi_swapchain_create(
        &engine->driver->context,
        &engine->swap_chains[engine->swap_chain_count],
        surface,
        width,
        height,
        &engine->scratch_arena);

    if (err != VKAPI_SUCCESS)
    {
        log_error("Error creating swapchain.");
        return NULL;
    }

    swapchain_handle_t* handle = calloc(1, sizeof(struct SwapchainHandle));
    handle->idx = engine->swap_chain_count++;
    return handle;
}
