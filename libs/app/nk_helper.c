/* Copyright (c) 2024-2025 Garry Whitehead
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

#include "nk_helper.h"

#include "window.h"

#include <backend/enums.h>
#include <rpe/engine.h>
#include <rpe/material.h>
#include <rpe/object_manager.h>
#include <rpe/renderable_manager.h>
#include <rpe/scene.h>
#include <rpe/transform_manager.h>
#include <string.h>
#include <utility/arena.h>

#define NK_IMPLEMENTATION
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_STANDARD_IO
#include <nuklear.h>

nk_instance_t*
nk_helper_init(const char* font_path, float font_size, rpe_engine_t* engine, arena_t* arena)
{
    rpe_rend_manager_t* rm = rpe_engine_get_rend_manager(engine);
    rpe_obj_manager_t* om = rpe_engine_get_obj_manager(engine);

    nk_instance_t* nk = ARENA_MAKE_ZERO_STRUCT(arena, nk_instance_t);

    nk_font_atlas_init_default(&nk->atlas);
    nk_font_atlas_begin(&nk->atlas);

    const struct nk_font_config config = nk_font_config(font_size);
    nk->atlas.default_font = nk_font_atlas_add_from_file(&nk->atlas, font_path, font_size, &config);
    if (!nk->atlas.default_font)
    {
        log_error("Error loading font from path: %s", font_path);
        return NULL;
    }

    int w, h;
    const void* image = nk_font_atlas_bake(&nk->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_init_default(&nk->ctx, &nk->atlas.default_font->handle);
    nk_font_atlas_end(&nk->atlas, nk_handle_ptr((void*)image), &nk->tex_null);

    // Upload the font to the device.
    rpe_mapped_texture_t tex = {
        .width = w,
        .height = h,
        .image_data = (void*)image,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .array_count = 1,
        .mip_levels = 1,
        .image_data_size = w * h * 4};
    sampler_params_t sampler = {
        .addr_u = RPE_SAMPLER_ADDR_MODE_REPEAT,
        .addr_v = RPE_SAMPLER_ADDR_MODE_REPEAT,
        .min = RPE_SAMPLER_FILTER_LINEAR,
        .mag = RPE_SAMPLER_FILTER_LINEAR};
    rpe_material_map_texture(engine, &tex, &sampler, false);

    // Create a material for the font and upload font bitmap to the device.
    nk->font_mat = rpe_rend_manager_create_material(rm);
    rpe_material_set_view_layer(nk->font_mat, 0x5);
    rpe_material_set_blend_factor_preset(nk->font_mat, RPE_BLEND_FACTOR_PRESET_TRANSLUCENT);
    rpe_material_set_type(nk->font_mat, RPE_MATERIAL_UI);
    rpe_material_set_shadow_caster_state(nk->font_mat, false);
    rpe_material_set_front_face(nk->font_mat, RPE_FRONT_FACE_COUNTER_CLOCKWISE);

    nk_buffer_init_default(&nk->v_buffer);
    nk_buffer_init_default(&nk->i_buffer);
    nk_buffer_init_default(&nk->cmds);

    nk->scene = rpe_engine_create_scene(engine);
    nk->camera = rpe_engine_create_camera(
        engine, 90.0f, 1, 1, 1.0f, 512.0f, RPE_PROJECTION_TYPE_PERSPECTIVE);
    rpe_scene_set_current_camera(nk->scene, engine, nk->camera);

    rpe_model_transform_t mt = rpe_model_transform_init();
    nk->transform_obj = rpe_obj_manager_create_obj(om);
    rpe_transform_manager_add_local_transform(
        rpe_engine_get_transform_manager(engine), &mt, &nk->transform_obj);

    for (int i = 0; i < RPE_NK_HELPER_MAX_RENDERABLES; ++i)
    {
        nk->rend_objs[i] = rpe_obj_manager_create_obj(om);
    }

    return nk;
}

void nk_helper_destroy(nk_instance_t* nk)
{
    nk_buffer_clear(&nk->cmds);
    nk_buffer_clear(&nk->v_buffer);
    nk_buffer_clear(&nk->i_buffer);
    nk_free(&nk->ctx);
}

void nk_helper_new_frame(nk_instance_t* nk, app_window_t* app_win)
{
    double x, y;
    struct nk_context* ctx = &nk->ctx;
    struct GLFWwindow* win = app_win->glfw_window;

    float delta_time_now = (float)glfwGetTime();
    nk->ctx.delta_time_seconds = delta_time_now - nk->delta_time_seconds_last;
    nk->delta_time_seconds_last = delta_time_now;

    int display_width, display_height;
    glfwGetWindowSize(win, (int*)&app_win->width, (int*)&app_win->height);
    glfwGetFramebufferSize(win, &display_width, &display_height);
    nk->fb_scale.x = (float)display_width / (float)app_win->width;
    nk->fb_scale.y = (float)display_height / (float)app_win->height;

    nk_input_begin(ctx);
    for (int i = 0; i < nk->text_len; ++i)
    {
        nk_input_unicode(ctx, nk->text[i]);
    }

    nk_char* k_state = nk->key_events;
    if (k_state[NK_KEY_DEL] >= 0)
        nk_input_key(ctx, NK_KEY_DEL, k_state[NK_KEY_DEL]);
    if (k_state[NK_KEY_ENTER] >= 0)
        nk_input_key(ctx, NK_KEY_ENTER, k_state[NK_KEY_ENTER]);

    if (k_state[NK_KEY_TAB] >= 0)
        nk_input_key(ctx, NK_KEY_TAB, k_state[NK_KEY_TAB]);
    if (k_state[NK_KEY_BACKSPACE] >= 0)
        nk_input_key(ctx, NK_KEY_BACKSPACE, k_state[NK_KEY_BACKSPACE]);
    if (k_state[NK_KEY_UP] >= 0)
        nk_input_key(ctx, NK_KEY_UP, k_state[NK_KEY_UP]);
    if (k_state[NK_KEY_DOWN] >= 0)
        nk_input_key(ctx, NK_KEY_DOWN, k_state[NK_KEY_DOWN]);
    if (k_state[NK_KEY_SCROLL_UP] >= 0)
        nk_input_key(ctx, NK_KEY_SCROLL_UP, k_state[NK_KEY_SCROLL_UP]);
    if (k_state[NK_KEY_SCROLL_DOWN] >= 0)
        nk_input_key(ctx, NK_KEY_SCROLL_DOWN, k_state[NK_KEY_SCROLL_DOWN]);

    if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
    {
        /* Note these are physical keys and won't respect any layouts/key mapping */
        if (k_state[NK_KEY_COPY] >= 0)
            nk_input_key(ctx, NK_KEY_COPY, k_state[NK_KEY_COPY]);
        if (k_state[NK_KEY_PASTE] >= 0)
            nk_input_key(ctx, NK_KEY_PASTE, k_state[NK_KEY_PASTE]);
        if (k_state[NK_KEY_CUT] >= 0)
            nk_input_key(ctx, NK_KEY_CUT, k_state[NK_KEY_CUT]);
        if (k_state[NK_KEY_TEXT_UNDO] >= 0)
            nk_input_key(ctx, NK_KEY_TEXT_UNDO, k_state[NK_KEY_TEXT_UNDO]);
        if (k_state[NK_KEY_TEXT_REDO] >= 0)
            nk_input_key(ctx, NK_KEY_TEXT_REDO, k_state[NK_KEY_TEXT_REDO]);
        if (k_state[NK_KEY_TEXT_LINE_START] >= 0)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_START, k_state[NK_KEY_TEXT_LINE_START]);
        if (k_state[NK_KEY_TEXT_LINE_END] >= 0)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_END, k_state[NK_KEY_TEXT_LINE_END]);
        if (k_state[NK_KEY_TEXT_SELECT_ALL] >= 0)
            nk_input_key(ctx, NK_KEY_TEXT_SELECT_ALL, k_state[NK_KEY_TEXT_SELECT_ALL]);
        if (k_state[NK_KEY_LEFT] >= 0)
            nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, k_state[NK_KEY_LEFT]);
        if (k_state[NK_KEY_RIGHT] >= 0)
            nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, k_state[NK_KEY_RIGHT]);
    }
    else
    {
        if (k_state[NK_KEY_LEFT] >= 0)
            nk_input_key(ctx, NK_KEY_LEFT, k_state[NK_KEY_LEFT]);
        if (k_state[NK_KEY_RIGHT] >= 0)
            nk_input_key(ctx, NK_KEY_RIGHT, k_state[NK_KEY_RIGHT]);
        nk_input_key(ctx, NK_KEY_COPY, 0);
        nk_input_key(ctx, NK_KEY_PASTE, 0);
        nk_input_key(ctx, NK_KEY_CUT, 0);
    }

    glfwGetCursorPos(win, &x, &y);
    nk_input_motion(ctx, (int)x, (int)y);
    nk_input_button(
        ctx,
        NK_BUTTON_LEFT,
        (int)x,
        (int)y,
        glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    nk_input_button(
        ctx,
        NK_BUTTON_MIDDLE,
        (int)x,
        (int)y,
        glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
    nk_input_button(
        ctx,
        NK_BUTTON_RIGHT,
        (int)x,
        (int)y,
        glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    nk_input_button(
        ctx,
        NK_BUTTON_DOUBLE,
        (int)nk->double_click_pos.x,
        (int)nk->double_click_pos.y,
        nk->is_double_click_down);

    nk_input_scroll(ctx, nk->scroll);
    nk_input_end(&nk->ctx);

    memset(nk->key_events, -1, sizeof(nk->key_events));

    nk->text_len = 0;
    nk->scroll = nk_vec2(0, 0);
}

void nk_helper_render(
    nk_instance_t* nk, rpe_engine_t* engine, app_window_t* win, UiCallback ui_callback, arena_t* arena)
{
    assert(ui_callback);
    assert(nk);

    rpe_rend_manager_t* rm = rpe_engine_get_rend_manager(engine);

    for (int i = 0; i < RPE_NK_HELPER_MAX_RENDERABLES; ++i)
    {
        if (nk->renderables[i])
        {
            rpe_engine_destroy_renderable(engine, nk->renderables[i]);
            nk->renderables[i] = NULL;
            rpe_scene_remove_object(nk->scene, nk->rend_objs[i]);
        }
    }

    // Draw the widgets for this frame via the callback.
    ui_callback(engine, win);

    struct nk_convert_config config = {0};
    struct nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(rpe_vertex_t, position)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(rpe_vertex_t, uv0)},
        {NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, offsetof(rpe_vertex_t, colour)},
        {NK_VERTEX_LAYOUT_END}};

    config.vertex_layout = vertex_layout;
    config.vertex_size = sizeof(rpe_vertex_t);
    config.vertex_alignment = __alignof(rpe_vertex_t);
    config.tex_null = nk->tex_null;
    config.circle_segment_count = 22;
    config.curve_segment_count = 22;
    config.arc_segment_count = 22;
    config.global_alpha = 1.0f;
    config.shape_AA = NK_ANTI_ALIASING_ON;
    config.line_AA = NK_ANTI_ALIASING_ON;

    uint8_t* vertex_tmp = ARENA_MAKE_ZERO_ARRAY(arena, uint8_t, sizeof(rpe_vertex_t) * 1000);
    uint8_t* index_tmp = ARENA_MAKE_ZERO_ARRAY(arena, uint8_t, sizeof(uint16_t) * 1000);

    nk_buffer_init_fixed(&nk->v_buffer, vertex_tmp, sizeof(rpe_vertex_t) * 1000);
    nk_buffer_init_fixed(&nk->i_buffer, index_tmp, sizeof(uint16_t) * 1000);

    memset(nk->v_buffer.memory.ptr, 0, nk->v_buffer.allocated);
    nk_flags res = nk_convert(&nk->ctx, &nk->cmds, &nk->v_buffer, &nk->i_buffer, &config);
    assert(res == NK_CONVERT_SUCCESS);

    rpe_mesh_t* mesh = rpe_rend_manager_create_mesh(
        rm,
        (rpe_vertex_t*)vertex_tmp,
        nk->ctx.draw_list.vertex_count,
        index_tmp,
        nk->ctx.draw_list.element_count,
        RPE_RENDERABLE_INDICES_U16,
        RPE_MESH_ATTRIBUTE_UV0 | RPE_MESH_ATTRIBUTE_POSITION | RPE_MESH_ATTRIBUTE_COLOUR);

    const struct nk_draw_command* cmd;
    uint32_t index_offset = 0;
    uint32_t idx = 0;

    nk_draw_foreach(cmd, &nk->ctx, &nk->cmds)
    {
        if (!cmd->elem_count)
        {
            continue;
        }

        rpe_mesh_t* new_mesh =
            rpe_rend_manager_offset_indices(rm, mesh, index_offset, cmd->elem_count);
        nk->renderables[idx] = rpe_engine_create_renderable(engine, nk->font_mat, new_mesh);

        int32_t x = (int32_t)MAX(cmd->clip_rect.x, 0.0f) * nk->fb_scale.x;
        int32_t y = (int32_t)MAX(cmd->clip_rect.y, 0.0f) * nk->fb_scale.y;
        uint32_t w = (uint32_t)(cmd->clip_rect.w - cmd->clip_rect.x) * nk->fb_scale.x;
        uint32_t h = (uint32_t)(cmd->clip_rect.h - cmd->clip_rect.y) * nk->fb_scale.y;
        rpe_renderable_set_scissor(nk->renderables[idx], x, y, w, h);

        rpe_rend_manager_add(rm, nk->renderables[idx], nk->rend_objs[idx], nk->transform_obj);
        rpe_scene_add_object(nk->scene, nk->rend_objs[idx]);

        index_offset += cmd->elem_count;
        ++idx;
    }

    nk_clear(&nk->ctx);
    arena_reset(arena);
}