#include <render_graph/dependency_graph.h>
#include <render_graph/render_graph.h>
#include <render_graph/render_pass_node.h>
#include <render_graph/rendergraph_resource.h>
#include <unity_fixture.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/error_codes.h>

TEST_GROUP(RenderGraphGroup);

TEST_SETUP(RenderGraphGroup) {}

TEST_TEAR_DOWN(RenderGraphGroup) {}

TEST(RenderGraphGroup, RenderGraph_DepGraph_Tests1)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 15;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    rg_dep_graph_t* dg = rg_dep_graph_init(&arena);
    rg_node_t* n1 = rg_node_init(dg, "node1", &arena);
    rg_node_t* n2 = rg_node_init(dg, "node2", &arena);
    rg_node_t* n3 = rg_node_init(dg, "node3", &arena);
    rg_node_declare_side_effect(n3);

    rg_edge_init(dg, n1, n2, &arena);
    rg_edge_init(dg, n2, n3, &arena);

    rg_dep_graph_cull(dg, &arena);

    TEST_ASSERT_FALSE(rg_node_is_culled(n1));
    TEST_ASSERT_FALSE(rg_node_is_culled(n2));
    TEST_ASSERT_FALSE(rg_node_is_culled(n3));

    TEST_ASSERT_EQUAL_UINT(1, n1->ref_count);
    TEST_ASSERT_EQUAL_UINT(1, n2->ref_count);
    TEST_ASSERT_EQUAL_UINT(0x7FFF, n3->ref_count);

    arena_release(&arena);
}

TEST(RenderGraphGroup, RenderGraph_DepGraph_Tests2)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 20;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    rg_dep_graph_t* dg = rg_dep_graph_init(&arena);
    rg_node_t* n1 = rg_node_init(dg, "node1", &arena);
    rg_node_t* n2 = rg_node_init(dg, "node2", &arena);
    rg_node_t* n3 = rg_node_init(dg, "node3", &arena);
    rg_node_t* n4 = rg_node_init(dg, "node4", &arena);
    rg_node_t* n5 = rg_node_init(dg, "node5", &arena);
    rg_node_t* n6 = rg_node_init(dg, "node6", &arena);
    rg_node_t* n7 = rg_node_init(dg, "node7", &arena);
    rg_node_t* n8 = rg_node_init(dg, "node8", &arena);
    rg_node_declare_side_effect(n6);

    rg_edge_init(dg, n1, n2, &arena);
    rg_edge_init(dg, n1, n3, &arena);
    rg_edge_init(dg, n2, n4, &arena);
    rg_edge_init(dg, n4, n7, &arena);
    rg_edge_init(dg, n3, n5, &arena);
    rg_edge_init(dg, n5, n6, &arena);
    rg_edge_init(dg, n2, n8, &arena);

    rg_dep_graph_cull(dg, &arena);

    TEST_ASSERT_FALSE(rg_node_is_culled(n1));
    TEST_ASSERT_FALSE(rg_node_is_culled(n2));
    TEST_ASSERT_FALSE(rg_node_is_culled(n3));
    TEST_ASSERT_FALSE(rg_node_is_culled(n4));
    TEST_ASSERT_TRUE(rg_node_is_culled(n7));
    TEST_ASSERT_FALSE(rg_node_is_culled(n5));
    TEST_ASSERT_FALSE(rg_node_is_culled(n6));
    TEST_ASSERT_TRUE(rg_node_is_culled(n8));

    TEST_ASSERT_EQUAL_UINT(2, n1->ref_count);
    TEST_ASSERT_EQUAL_UINT(2, n2->ref_count);
    TEST_ASSERT_EQUAL_UINT(1, n3->ref_count);
    TEST_ASSERT_EQUAL_UINT(1, n4->ref_count);
    TEST_ASSERT_EQUAL_UINT(1, n5->ref_count);
    TEST_ASSERT_EQUAL_UINT(0x7FFF, n6->ref_count);
    TEST_ASSERT_EQUAL_UINT(0, n7->ref_count);
    TEST_ASSERT_EQUAL_UINT(0, n8->ref_count);

    arena_release(&arena);
}

struct DataRW
{
    rg_handle_t rw;
};

void setup1(render_graph_t* rg, rg_pass_node_t* node, void* data)
{
    struct DataRW* d = (struct DataRW*)data;
    TEST_ASSERT_TRUE(d);
    rg_texture_desc_t desc = {
        .width = 100,
        .height = 100,
        .mip_levels = 1,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .depth = 1};
    rg_texture_resource_t* r = rg_tex_resource_init(
        "InputTex", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, desc, rg_get_arena(rg));
    d->rw = rg_add_resource(rg, (rg_resource_t*)r, rg_get_arena(rg), NULL);
    d->rw = rg_add_write(rg, d->rw, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    rg_add_read(rg, d->rw, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
}

TEST(RenderGraphGroup, RenderGraph_Tests1)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 20;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    vkapi_driver_t driver;
    int error_code = vkapi_driver_init(NULL, 0, &driver);
    TEST_ASSERT(error_code == VKAPI_SUCCESS);
    error_code = vkapi_driver_create_device(&driver, NULL);
    TEST_ASSERT(error_code == VKAPI_SUCCESS);

    render_graph_t* rg = rg_init(&arena);
    rg_pass_t* p = rg_add_pass(rg, "Pass1", setup1, NULL, sizeof(struct DataRW));
    TEST_ASSERT_TRUE(p);
    rg_compile(rg);
    TEST_ASSERT_TRUE(rg_node_is_culled((rg_node_t*)p->node));
    rg_execute(rg, p, &driver);

    vkapi_driver_shutdown(&driver);
    arena_release(&arena);
}

struct DataBasic
{
    rg_handle_t depth;
};

void setup_basic(render_graph_t* rg, rg_pass_node_t* node, void* data)
{
    struct DataBasic* d = (struct DataBasic*)data;
    TEST_ASSERT_TRUE(d);
    rg_texture_desc_t t_desc = {
        .width = 100,
        .height = 100,
        .mip_levels = 1,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .depth = 1};
    rg_texture_resource_t* r = rg_tex_resource_init(
        "DepthImage", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, t_desc, rg_get_arena(rg));
    d->depth = rg_add_resource(rg, (rg_resource_t*)r, rg_get_arena(rg), NULL);
    d->depth = rg_add_write(rg, d->depth, node, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    rg_pass_desc_t desc = { .attachments.attach.depth = d->depth };
    rg_rpass_node_create_rt((rg_render_pass_node_t*)node, rg, "DepthPass", desc, rg_get_arena(rg));
}

void execute_basic(vkapi_driver_t* driver, rg_render_graph_resource_t* res, void* data)
{
    TEST_ASSERT_TRUE(data);
    struct DataBasic* d = (struct DataBasic*)data;
    TEST_ASSERT_TRUE(rg_handle_is_valid(d->depth));
    rg_resource_info_t info = rg_res_get_render_pass_info(res, d->depth);
    TEST_ASSERT_EQUAL_UINT(100, info.data.height);
    TEST_ASSERT_EQUAL_UINT(100, info.data.width);
}

TEST(RenderGraphGroup, RenderGraph_TestsBasic)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 20;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    vkapi_driver_t driver;
    int error_code = vkapi_driver_init(NULL, 0, &driver);
    TEST_ASSERT(error_code == VKAPI_SUCCESS);
    error_code = vkapi_driver_create_device(&driver, NULL);
    TEST_ASSERT(error_code == VKAPI_SUCCESS);

    render_graph_t* rg = rg_init(&arena);
    rg_pass_t* p = rg_add_pass(rg, "Pass1", setup_basic, execute_basic, sizeof(struct DataBasic));
    TEST_ASSERT_TRUE(p);
    rg_compile(rg);
    TEST_ASSERT_TRUE(rg_node_is_culled((rg_node_t*)p->node));
    rg_execute(rg, p, &driver);

    vkapi_driver_shutdown(&driver);
    arena_release(&arena);
}