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
#include <backend/objects.h>
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
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_VARARGS
#include <nuklear.h>

void set_ui_style(struct nk_context* ctx)
{
    struct nk_color table[NK_COLOR_COUNT] = {0};
    table[NK_COLOR_TEXT] = nk_rgba(190, 190, 190, 255);
    table[NK_COLOR_WINDOW] = nk_rgba(30, 33, 40, 215);
    table[NK_COLOR_HEADER] = nk_rgba(181, 45, 69, 220);
    table[NK_COLOR_BORDER] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_BUTTON] = nk_rgba(181, 45, 69, 255);
    table[NK_COLOR_BUTTON_HOVER] = nk_rgba(190, 50, 70, 255);
    table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(195, 55, 75, 255);
    table[NK_COLOR_TOGGLE] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 60, 60, 255);
    table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(181, 45, 69, 255);
    table[NK_COLOR_SELECT] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(181, 45, 69, 255);
    table[NK_COLOR_SLIDER] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(181, 45, 69, 255);
    table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(186, 50, 74, 255);
    table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(191, 55, 79, 255);
    table[NK_COLOR_PROPERTY] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_EDIT] = nk_rgba(51, 55, 67, 225);
    table[NK_COLOR_EDIT_CURSOR] = nk_rgba(190, 190, 190, 255);
    table[NK_COLOR_COMBO] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_CHART] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_CHART_COLOR] = nk_rgba(170, 40, 60, 255);
    table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
    table[NK_COLOR_SCROLLBAR] = nk_rgba(30, 33, 40, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
    table[NK_COLOR_TAB_HEADER] = nk_rgba(181, 45, 69, 220);
    nk_style_from_table(ctx, table);
}

nk_instance_t* nk_helper_init(
    const char* font_path,
    float font_size,
    rpe_engine_t* engine,
    app_window_t* app_win,
    arena_t* arena)
{
    rpe_rend_manager_t* rm = rpe_engine_get_rend_manager(engine);
    rpe_obj_manager_t* om = rpe_engine_get_obj_manager(engine);

    nk_instance_t* nk = ARENA_MAKE_ZERO_STRUCT(arena, nk_instance_t);
    nk_init_default(&nk->ctx, 0);
    nk->delta_time_seconds_last = (float)glfwGetTime();

    nk->scene = rpe_engine_create_scene(engine);
    // Never draw shadows for the UI.
    rpe_scene_set_shadow_status(nk->scene, RPE_SCENE_SHADOW_STATUS_NEVER);
    rpe_scene_skip_lighting_pass(nk->scene);

    nk->camera = rpe_engine_create_camera(engine);
    rpe_camera_set_proj_matrix(
        nk->camera, 90.0f, app_win->width, app_win->height, 0.0f, 1.0f, RPE_PROJECTION_TYPE_ORTHO);
    rpe_scene_set_current_camera(nk->scene, engine, nk->camera);

    nk_font_atlas_init_default(&nk->atlas);
    nk_font_atlas_begin(&nk->atlas);

    const struct nk_font_config config = nk_font_config(font_size);
    struct nk_font* font_atlas =
        nk_font_atlas_add_from_file(&nk->atlas, font_path, font_size, &config);
    if (!font_atlas)
    {
        log_error("Error loading font from path: %s", font_path);
        return NULL;
    }

    int w, h;
    const void* image = nk_font_atlas_bake(&nk->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);

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
        .addr_u = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE,
        .addr_v = RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE,
        .min = RPE_SAMPLER_FILTER_LINEAR,
        .mag = RPE_SAMPLER_FILTER_LINEAR};
    texture_handle_t t = rpe_material_map_texture(engine, &tex, &sampler, false);

    nk_font_atlas_end(&nk->atlas, nk_handle_ptr((void*)image), &nk->tex_null);
    nk_style_set_font(&nk->ctx, &font_atlas->handle);

    // Create a material for the font and upload font bitmap to the device.
    nk->font_mat = rpe_rend_manager_create_material(rm, nk->scene);
    rpe_material_set_blend_factor_preset(nk->font_mat, RPE_BLEND_FACTOR_PRESET_TRANSLUCENT);
    rpe_material_set_type(nk->font_mat, RPE_MATERIAL_UI);
    rpe_material_set_shadow_caster_state(nk->font_mat, false);

    rpe_material_set_device_texture(nk->font_mat, t, RPE_MATERIAL_IMAGE_TYPE_BASE_COLOR, 0);

    nk_buffer_init_default(&nk->v_buffer);
    nk_buffer_init_default(&nk->i_buffer);
    nk_buffer_init_default(&nk->cmds);

    // Setup draw call configuration.
    nk->vertex_layout[0].attribute = NK_VERTEX_POSITION;
    nk->vertex_layout[0].format = NK_FORMAT_FLOAT;
    nk->vertex_layout[0].offset = offsetof(rpe_vertex_t, position);
    nk->vertex_layout[1].attribute = NK_VERTEX_TEXCOORD;
    nk->vertex_layout[1].format = NK_FORMAT_FLOAT;
    nk->vertex_layout[1].offset = offsetof(rpe_vertex_t, uv0);
    nk->vertex_layout[2].attribute = NK_VERTEX_COLOR;
    nk->vertex_layout[2].format = NK_FORMAT_R32G32B32A32_FLOAT;
    nk->vertex_layout[2].offset = offsetof(rpe_vertex_t, colour);
    nk->vertex_layout[3].attribute = NK_VERTEX_LAYOUT_END;

    nk->config.vertex_layout = nk->vertex_layout;
    nk->config.vertex_size = sizeof(rpe_vertex_t);
    nk->config.vertex_alignment = __alignof(rpe_vertex_t);
    nk->config.tex_null = nk->tex_null;
    nk->config.circle_segment_count = 22;
    nk->config.curve_segment_count = 22;
    nk->config.arc_segment_count = 22;
    nk->config.global_alpha = 1.0f;
    nk->config.shape_AA = NK_ANTI_ALIASING_ON;
    nk->config.line_AA = NK_ANTI_ALIASING_ON;

    rpe_model_transform_t mt = rpe_model_transform_init();
    nk->transform_obj = rpe_obj_manager_create_obj(om);
    rpe_transform_manager_add_local_transform(
        rpe_engine_get_transform_manager(engine), &mt, &nk->transform_obj);

    for (int i = 0; i < RPE_NK_HELPER_MAX_RENDERABLES; ++i)
    {
        nk->rend_objs[i] = rpe_obj_manager_create_obj(om);
    }

    nk->vbuffer_handle =
        rpe_rend_manager_alloc_vertex_buffer(rm, RPE_NK_HELPER_MAX_VERTEX_BUFFER_COUNT);
    nk->ibuffer_handle =
        rpe_rend_manager_alloc_index_buffer(rm, RPE_NK_HELPER_MAX_INDEX_BUFFER_COUNT);

    set_ui_style(&nk->ctx);
    return nk;
}

void nk_helper_destroy(nk_instance_t* nk)
{
    nk_buffer_clear(&nk->cmds);
    nk_buffer_clear(&nk->v_buffer);
    nk_buffer_clear(&nk->i_buffer);
    nk_free(&nk->ctx);
}

void update_nk_inputs(nk_instance_t* nk, app_window_t* app_win)
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

    if (ctx->input.mouse.grab)
    {
        glfwSetInputMode(app_win->glfw_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else if (ctx->input.mouse.ungrab)
    {
        glfwSetInputMode(app_win->glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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

    if (ctx->input.mouse.grabbed)
    {
        glfwSetCursorPos(app_win->glfw_window, ctx->input.mouse.prev.x, ctx->input.mouse.prev.y);
        ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
        ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
    }

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

void update_nk_draw_calls(
    nk_instance_t* nk, rpe_engine_t* engine, app_window_t* win, arena_t* arena)
{
    assert(nk);

    rpe_rend_manager_t* rm = rpe_engine_get_rend_manager(engine);

    for (int i = 0; i < RPE_NK_HELPER_MAX_RENDERABLES; ++i)
    {
        if (nk->renderables[i])
        {
            bool res = rpe_engine_destroy_renderable(engine, nk->renderables[i]);
            res &= rpe_rend_manager_remove(rm, nk->rend_objs[i]);
            nk->renderables[i] = NULL;
            res &= rpe_scene_remove_object(nk->scene, nk->rend_objs[i]);
            assert(res);
        }
    }

    nk_buffer_clear(&nk->v_buffer);
    nk_buffer_clear(&nk->i_buffer);
    nk_buffer_clear(&nk->cmds);

    uint8_t* vertex_tmp = ARENA_MAKE_ZERO_ARRAY(arena, uint8_t, sizeof(rpe_vertex_t) * 2000);
    uint8_t* index_tmp = ARENA_MAKE_ZERO_ARRAY(arena, uint8_t, sizeof(uint16_t) * 2000);

    nk_buffer_init_fixed(&nk->v_buffer, vertex_tmp, sizeof(rpe_vertex_t) * 2000);
    nk_buffer_init_fixed(&nk->i_buffer, index_tmp, sizeof(uint16_t) * 2000);

    nk_flags res = nk_convert(&nk->ctx, &nk->cmds, &nk->v_buffer, &nk->i_buffer, &nk->config);
    assert(res == NK_CONVERT_SUCCESS);

    rpe_mesh_t* mesh = rpe_rend_manager_create_mesh(
        rm,
        nk->vbuffer_handle,
        (rpe_vertex_t*)vertex_tmp,
        nk->ctx.draw_list.vertex_count,
        nk->ibuffer_handle,
        index_tmp,
        nk->ctx.draw_list.element_count,
        RPE_RENDERABLE_INDICES_U16,
        RPE_MESH_ATTRIBUTE_UV0 | RPE_MESH_ATTRIBUTE_POSITION | RPE_MESH_ATTRIBUTE_COLOUR);

    const struct nk_draw_command* cmd;
    uint32_t index_offset = 0;
    uint32_t idx = 0;

    uint8_t current_layer = 0x5;
    nk_draw_foreach(cmd, &nk->ctx, &nk->cmds)
    {
        if (!cmd->elem_count)
        {
            continue;
        }

        rpe_mesh_t* new_mesh =
            rpe_rend_manager_offset_indices(rm, mesh, index_offset, cmd->elem_count);
        nk->renderables[idx] = rpe_engine_create_renderable(engine, nk->font_mat, new_mesh);
        rpe_renderable_t* rend = nk->renderables[idx];

        int32_t x = (int32_t)MAX(cmd->clip_rect.x, 0.0f) * nk->fb_scale.x;
        int32_t y = (int32_t)MAX(cmd->clip_rect.y, 0.0f) * nk->fb_scale.y;
        uint32_t w = (uint32_t)cmd->clip_rect.w * nk->fb_scale.x;
        uint32_t h = (uint32_t)cmd->clip_rect.h * nk->fb_scale.y;
        rpe_renderable_set_scissor(rend, x, y, w, h);

        // The order of draws needs to be maintained, so we don't draw over the widgets with the
        // window. To achieve this and get round the batching, each ui renderable is placed on a
        // different layer. This does mean more draw calls, but this is unavoidable.
        rpe_renderable_set_view_layer(rend, current_layer);

        rpe_rend_manager_add(rm, rend, nk->rend_objs[idx], nk->transform_obj);
        rpe_scene_add_object(nk->scene, nk->rend_objs[idx]);

        index_offset += cmd->elem_count;
        ++idx;
        current_layer += 0x01;
    }

    nk_clear(&nk->ctx);
    arena_reset(arena);
}

void nk_helper_new_frame(
    nk_instance_t* nk,
    rpe_engine_t* engine,
    app_window_t* app_win,
    UiCallback ui_callback,
    arena_t* arena)
{
    update_nk_inputs(nk, app_win);

    // Draw the widgets for this frame via the callback.
    ui_callback(engine, rpe_engine_get_current_scene(engine), app_win);

    update_nk_draw_calls(nk, engine, app_win, arena);
}