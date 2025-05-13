#include <unity_fixture.h>
#include <vulkan-api/descriptor_cache.h>
#include <vulkan-api/pipeline_cache.h>

TEST_GROUP(CacheGroup);

TEST_SETUP(CacheGroup) {}

TEST_TEAR_DOWN(CacheGroup) {}

TEST(CacheGroup, KeyCompare_Test)
{
    // Graphics pipeline keys.
    graphics_pl_key_t gfx_key1 = {0};
    graphics_pl_key_t gfx_key2 = {0};
    TEST_ASSERT(vkapi_pline_cache_compare_graphic_keys(&gfx_key1, &gfx_key2) == true);
    gfx_key1.tesse_vert_count = 200;
    gfx_key1.colour_attach_count = 5;
    TEST_ASSERT(vkapi_pline_cache_compare_graphic_keys(&gfx_key1, &gfx_key2) == false);
    gfx_key2.tesse_vert_count = 200;
    gfx_key2.colour_attach_count = 5;
    gfx_key2.raster_state.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    TEST_ASSERT(vkapi_pline_cache_compare_graphic_keys(&gfx_key1, &gfx_key2) == false);
    gfx_key1.raster_state.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    TEST_ASSERT(vkapi_pline_cache_compare_graphic_keys(&gfx_key1, &gfx_key2) == true);

    // Compute pipeline keys.
    compute_pl_key_t comp_key1 = {0};
    compute_pl_key_t comp_key2 = {0};
    TEST_ASSERT(vkapi_pline_cache_compare_compute_keys(&comp_key1, &comp_key2) == true);
    comp_key1.shader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    TEST_ASSERT(vkapi_pline_cache_compare_compute_keys(&comp_key1, &comp_key2) == false);
    comp_key2.shader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    TEST_ASSERT(vkapi_pline_cache_compare_compute_keys(&comp_key1, &comp_key2) == true);

    desc_key_t desc_key1 = {0};
    desc_key_t desc_key2 = {0};
    TEST_ASSERT(vkapi_desc_cache_compare_desc_keys(&desc_key1, &desc_key2) == true);
    desc_key1.ssbo_buffer_sizes[0] = 10;
    desc_key1.buffer_sizes[0] = 5;
    TEST_ASSERT(vkapi_desc_cache_compare_desc_keys(&desc_key1, &desc_key2) == false);
    desc_key2.ssbo_buffer_sizes[0] = 10;
    desc_key2.buffer_sizes[0] = 5;
    TEST_ASSERT(vkapi_desc_cache_compare_desc_keys(&desc_key1, &desc_key2) == true);
}