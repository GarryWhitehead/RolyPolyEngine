#include "vk_setup.h"

#include <engine.h>
#include <render_graph/backboard.h>
#include <render_graph/dependency_graph.h>
#include <render_graph/render_graph.h>
#include <render_graph/render_pass_node.h>
#include <render_graph/rendergraph_resource.h>
#include <unity_fixture.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/error_codes.h>

#include <string.h>

TEST_GROUP(RenderGraphGroup);

TEST_SETUP(RenderGraphGroup) {}

TEST_TEAR_DOWN(RenderGraphGroup) {}

TEST(RenderGraphGroup, RenderGraph_DepGraph_Tests1)
{
    arena_t* arena = setup_arena(1 << 20);

    rg_dep_graph_t* dg = rg_dep_graph_init(arena);
    rg_node_t* n1 = rg_node_init(dg, "node1", arena);
    rg_node_t* n2 = rg_node_init(dg, "node2", arena);
    rg_node_t* n3 = rg_node_init(dg, "node3", arena);
    rg_node_declare_side_effect(n3);

    rg_edge_init(dg, n1, n2, arena);
    rg_edge_init(dg, n2, n3, arena);

    rg_dep_graph_cull(dg, arena);

    TEST_ASSERT_FALSE(rg_node_is_culled(n1));
    TEST_ASSERT_FALSE(rg_node_is_culled(n2));
    TEST_ASSERT_FALSE(rg_node_is_culled(n3));

    TEST_ASSERT_EQUAL_UINT(1, n1->ref_count);
    TEST_ASSERT_EQUAL_UINT(1, n2->ref_count);
    TEST_ASSERT_EQUAL_UINT(0x7FFF, n3->ref_count);
}

TEST(RenderGraphGroup, RenderGraph_DepGraph_Tests2)
{
    arena_t* arena = setup_arena(1 << 20);

    rg_dep_graph_t* dg = rg_dep_graph_init(arena);
    rg_node_t* n1 = rg_node_init(dg, "node1", arena);
    rg_node_t* n2 = rg_node_init(dg, "node2", arena);
    rg_node_t* n3 = rg_node_init(dg, "node3", arena);
    rg_node_t* n4 = rg_node_init(dg, "node4", arena);
    rg_node_t* n5 = rg_node_init(dg, "node5", arena);
    rg_node_t* n6 = rg_node_init(dg, "node6", arena);
    rg_node_t* n7 = rg_node_init(dg, "node7", arena);
    rg_node_t* n8 = rg_node_init(dg, "node8", arena);
    rg_node_declare_side_effect(n6);

    rg_edge_init(dg, n1, n2, arena);
    rg_edge_init(dg, n1, n3, arena);
    rg_edge_init(dg, n2, n4, arena);
    rg_edge_init(dg, n4, n7, arena);
    rg_edge_init(dg, n3, n5, arena);
    rg_edge_init(dg, n5, n6, arena);
    rg_edge_init(dg, n2, n8, arena);

    rg_dep_graph_cull(dg, arena);

    TEST_ASSERT_FALSE(rg_node_is_culled(n1));
    TEST_ASSERT_TRUE(rg_node_is_culled(n2));
    TEST_ASSERT_FALSE(rg_node_is_culled(n3));
    TEST_ASSERT_TRUE(rg_node_is_culled(n4));
    TEST_ASSERT_TRUE(rg_node_is_culled(n7));
    TEST_ASSERT_FALSE(rg_node_is_culled(n5));
    TEST_ASSERT_FALSE(rg_node_is_culled(n6));
    TEST_ASSERT_TRUE(rg_node_is_culled(n8));

    TEST_ASSERT_EQUAL_UINT(1, n1->ref_count);
    TEST_ASSERT_EQUAL_UINT(0, n2->ref_count);
    TEST_ASSERT_EQUAL_UINT(1, n3->ref_count);
    TEST_ASSERT_EQUAL_UINT(0, n4->ref_count);
    TEST_ASSERT_EQUAL_UINT(1, n5->ref_count);
    TEST_ASSERT_EQUAL_UINT(0x7FFF, n6->ref_count);
    TEST_ASSERT_EQUAL_UINT(0, n7->ref_count);
    TEST_ASSERT_EQUAL_UINT(0, n8->ref_count);
}

struct DataRW
{
    rg_handle_t rw;
};

void setup1(render_graph_t* rg, rg_pass_node_t* node, void* data, void* local_data)
{
    struct DataRW* d = (struct DataRW*)data;
    TEST_ASSERT_TRUE(d);
    rg_texture_desc_t desc = {
        .width = 100,
        .height = 100,
        .mip_levels = 1,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .layers = 1,
        .depth = 1};
    rg_texture_resource_t* r = rg_tex_resource_init(
        "InputTex", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, desc, rg_get_arena(rg));
    d->rw = rg_add_resource(rg, (rg_resource_t*)r, NULL);
    d->rw = rg_add_write(rg, d->rw, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    // rg_add_read(rg, d->rw, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
}

TEST(RenderGraphGroup, RenderGraph_Tests1)
{
    arena_t* arena = setup_arena(1 << 20);
    vkapi_driver_t* driver = setup_driver();

    render_graph_t* rg = rg_init(arena);
    rpe_engine_t* eng = NULL;
    rg_pass_t* p = rg_add_pass(rg, "Pass1", setup1, NULL, sizeof(struct DataRW), NULL);
    TEST_ASSERT_TRUE(p);
    rg_compile(rg);
    TEST_ASSERT_TRUE(rg_node_is_culled((rg_node_t*)p->node));
    rg_execute(rg, driver, eng);

    test_shutdown(driver, arena);
}

struct DataBasic
{
    rg_handle_t depth;
    rg_handle_t rt;
};

void setup_basic(render_graph_t* rg, rg_pass_node_t* node, void* data, void* local_data)
{
    struct DataBasic* d = (struct DataBasic*)data;
    TEST_ASSERT_TRUE(d);
    rg_texture_desc_t t_desc = {
        .width = 100,
        .height = 100,
        .mip_levels = 1,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .layers = 1,
        .depth = 1};
    rg_texture_resource_t* r = rg_tex_resource_init(
        "DepthImage", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg));
    d->depth = rg_add_resource(rg, (rg_resource_t*)r, NULL);
    d->depth = rg_add_write(rg, d->depth, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    rg_pass_desc_t desc = rg_pass_desc_init();
    desc.attachments.attach.depth = d->depth;
    d->rt = rg_rpass_node_create_rt((rg_render_pass_node_t*)node, rg, "DepthPass", desc);
    rg_node_declare_side_effect((rg_node_t*)node);
}

void execute_basic(
    vkapi_driver_t* driver, rpe_engine_t* engine, rg_render_graph_resource_t* res, void* data)
{
    TEST_ASSERT_TRUE(data);
    struct DataBasic* d = (struct DataBasic*)data;
    TEST_ASSERT_TRUE(rg_handle_is_valid(d->rt));
    rg_resource_info_t info = rg_res_get_render_pass_info(res, d->rt);
    TEST_ASSERT_EQUAL_UINT(100, info.data.height);
    TEST_ASSERT_EQUAL_UINT(100, info.data.width);
}

TEST(RenderGraphGroup, RenderGraph_TestsBasic)
{
    arena_t* arena = setup_arena(1 << 20);
    vkapi_driver_t* driver = setup_driver();

    render_graph_t* rg = rg_init(arena);
    rg_pass_t* p =
        rg_add_pass(rg, "Pass1", setup_basic, execute_basic, sizeof(struct DataBasic), NULL);
    TEST_ASSERT_TRUE(p);
    rg_compile(rg);
    TEST_ASSERT_FALSE(rg_node_is_culled((rg_node_t*)p->node));
    rg_execute(rg, driver, NULL);

    test_shutdown(driver, arena);
}

struct DataGBuffer
{
    rg_handle_t pos;
    rg_handle_t normal;
    rg_handle_t emissive;
    rg_handle_t pbr;
    rg_handle_t depth;
    rg_handle_t colour;
    rg_handle_t rt;
};

void setup_gbuffer_test(render_graph_t* rg, rg_pass_node_t* node, void* data, void* local_data)
{
    struct DataGBuffer* d = (struct DataGBuffer*)data;
    TEST_ASSERT_TRUE(d);
    rg_texture_desc_t t_desc = {.width = 100, .height = 100, .mip_levels = 1, .layers = 1, .depth = 1};

    t_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    d->colour = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "Colour", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    t_desc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    d->pos = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "Position", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    t_desc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    d->normal = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "Normal", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    t_desc.format = VK_FORMAT_R16G16_SFLOAT;
    d->pbr = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "PBR", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    t_desc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    d->emissive = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "Emissive", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    t_desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
    d->depth = rg_add_resource(
        rg,
        (rg_resource_t*)rg_tex_resource_init(
            "Depth", VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, t_desc, rg_get_arena(rg)),
        NULL);

    d->colour = rg_add_write(rg, d->colour, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    d->pos = rg_add_write(rg, d->pos, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    d->normal = rg_add_write(rg, d->normal, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    d->pbr = rg_add_write(rg, d->pbr, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    d->emissive = rg_add_write(rg, d->emissive, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    d->depth = rg_add_write(rg, d->depth, node, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    rg_pass_desc_t desc = rg_pass_desc_init();
    desc.attachments.attach.colour[0] = d->colour;
    desc.attachments.attach.colour[1] = d->pos;
    desc.attachments.attach.colour[2] = d->normal;
    desc.attachments.attach.colour[3] = d->emissive;
    desc.attachments.attach.colour[4] = d->pbr;
    desc.attachments.attach.depth = d->depth;
    desc.ds_load_clear_flags[0] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;
    desc.ds_load_clear_flags[1] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;

    d->rt = rg_rpass_node_create_rt((rg_render_pass_node_t*)node, rg, "GBufferPass", desc);
    rg_node_declare_side_effect(node);

    rg_backboard_t* bb = rg_get_backboard(rg);

    rg_backboard_add(bb, "colour", d->colour);
    rg_backboard_add(bb, "position", d->pos);
    rg_backboard_add(bb, "normal", d->normal);
    rg_backboard_add(bb, "emissive", d->emissive);
    rg_backboard_add(bb, "pbr", d->pbr);
    rg_backboard_add(bb, "gbufferDepth", d->depth);
}

void execute_gbuffer_test(
    vkapi_driver_t* driver, rpe_engine_t* engine, rg_render_graph_resource_t* res, void* data)
{
    TEST_ASSERT_TRUE(data);
    struct DataGBuffer* d = (struct DataGBuffer*)data;
    TEST_ASSERT_TRUE(rg_handle_is_valid(d->rt));
    rg_resource_info_t info = rg_res_get_render_pass_info(res, d->rt);
    TEST_ASSERT_EQUAL_UINT(100, info.data.height);
    TEST_ASSERT_EQUAL_UINT(100, info.data.width);

    TEST_ASSERT_EQUAL(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, info.data.final_layouts[0]);
    TEST_ASSERT_EQUAL(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, info.data.final_layouts[1]);
    TEST_ASSERT_EQUAL(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, info.data.final_layouts[2]);
    TEST_ASSERT_EQUAL(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, info.data.final_layouts[3]);
    TEST_ASSERT_EQUAL(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, info.data.final_layouts[4]);

    TEST_ASSERT_EQUAL(VK_FORMAT_UNDEFINED, info.data.init_layouts[0]);
    TEST_ASSERT_EQUAL(VK_FORMAT_UNDEFINED, info.data.init_layouts[1]);
    TEST_ASSERT_EQUAL(VK_FORMAT_UNDEFINED, info.data.init_layouts[2]);
    TEST_ASSERT_EQUAL(VK_FORMAT_UNDEFINED, info.data.init_layouts[3]);
    TEST_ASSERT_EQUAL(VK_FORMAT_UNDEFINED, info.data.init_layouts[4]);
}

TEST(RenderGraphGroup, RenderGraph_TestsGBuffer)
{
    arena_t* arena = setup_arena(1 << 20);
    vkapi_driver_t* driver = setup_driver();

    render_graph_t* rg = rg_init(arena);
    rpe_engine_t* eng = NULL;
    rg_pass_t* p =
        rg_add_pass(rg, "Pass1", setup_gbuffer_test, execute_gbuffer_test, sizeof(struct DataGBuffer), NULL);
    TEST_ASSERT_TRUE(p);
    rg_compile(rg);
    TEST_ASSERT_FALSE(rg_node_is_culled((rg_node_t*)p->node));
    rg_execute(rg, driver, eng);

    test_shutdown(driver, arena);
}

void execute_gbuffer_present(
    vkapi_driver_t* driver, rpe_engine_t* engine, rg_render_graph_resource_t* res, void* data)
{
    TEST_ASSERT_TRUE(data);
    struct DataGBuffer* d = (struct DataGBuffer*)data;
    TEST_ASSERT_TRUE(rg_handle_is_valid(d->rt));
    rg_resource_info_t info = rg_res_get_render_pass_info(res, d->rt);
    TEST_ASSERT_EQUAL_UINT(100, info.data.height);
    TEST_ASSERT_EQUAL_UINT(100, info.data.width);

    TEST_ASSERT_EQUAL(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, info.data.final_layouts[0]);
    TEST_ASSERT_EQUAL(VK_FORMAT_UNDEFINED, info.data.init_layouts[0]);
}

TEST(RenderGraphGroup, RenderGraph_TestsGBuffer_PresentPass)
{
    arena_t* arena = setup_arena(1 << 20);
    vkapi_driver_t* driver = setup_driver();

    math_vec4f col = {0.0f, 0.0f, 0.0f, 1.0f};
    vkapi_attach_info_t col_attach[6];
    memset(col_attach, 0, sizeof(vkapi_attach_info_t) * 6);
    vkapi_attach_info_t depth_attach = {.layer = 0, .level = 0, .handle = 0};

    vkapi_rt_handle_t pp_handle =
        vkapi_driver_create_rt(driver, 0, col, col_attach, depth_attach, depth_attach);

    rg_import_rt_desc_t i_desc;
    i_desc.width = 100;
    i_desc.height = 100;
    i_desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    i_desc.store_clear_flags[0] = RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_DONTCARE;
    i_desc.load_clear_flags[0] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;
    i_desc.final_layouts[0] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    i_desc.init_layouts[0] = VK_IMAGE_LAYOUT_UNDEFINED;
    i_desc.clear_col.r = 0.0f;
    i_desc.clear_col.g = 0.0f;
    i_desc.clear_col.b = 0.0f;
    i_desc.clear_col.a = 1.0f;

    render_graph_t* rg = rg_init(arena);
    rg_pass_t* p = rg_add_pass(
        rg, "Pass1", setup_gbuffer_test, execute_gbuffer_present, sizeof(struct DataGBuffer), NULL);
    TEST_ASSERT_TRUE(p);

    struct DataGBuffer* d = (struct DataGBuffer*)p->data;
    rg_handle_t backbuffer_handle = rg_import_render_target(rg, "BackBuffer", i_desc, pp_handle);
    rg_move_resource(rg, d->colour, backbuffer_handle);
    rg_add_present_pass(rg, backbuffer_handle);

    rg_compile(rg);
    TEST_ASSERT_FALSE(rg_node_is_culled((rg_node_t*)p->node));
    rg_execute(rg, driver, NULL);

    test_shutdown(driver, arena);
}