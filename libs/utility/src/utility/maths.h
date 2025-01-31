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

#ifndef __UTILITY_MATHS_H__
#define __UTILITY_MATHS_H__

#if __SSE__ || __AVX2__
#define MATH_USE_SSE3 1
#include <pmmintrin.h>
#endif
#if __ARM_NEON
#define MATH_USE_NEON 1
#include <arm_neon.h>
#endif

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define MAX(a, b) a > b ? a : b
#define MIN(a, b) a < b ? a : b

#define CLAMP(x, low, high)                                                                        \
    ({                                                                                             \
        __typeof__(x) __x = (x);                                                                   \
        __typeof__(low) __low = (low);                                                             \
        __typeof__(high) __high = (high);                                                          \
        __x > __high ? __high : (__x < __low ? __low : __x);                                       \
    })

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef union Vec2
{
    struct
    {
        float x, y;
    };
    struct
    {
        float r, g;
    };
    struct
    {
        float u, v;
    };
    float data[2];
} math_vec2f;

typedef union Vec3
{
    struct
    {
        float x, y, z;
    };
    struct
    {
        float r, g, b;
    };
    struct
    {
        float u, v, w;
    };
    float data[3];
} math_vec3f;

typedef union Vec4
{
    struct
    {
        float x, y, z, w;
    };
    struct
    {
        float r, g, b, a;
    };
    float data[4];
#ifdef MATH_USE_SSE3
    __m128 sse_data;
#endif
} math_vec4f;

typedef union Mat3
{
    float data[3][3];
    union Vec3 cols[3];
} math_mat3f;

typedef union Mat4
{
    float data[4][4];
    union Vec4 cols[4];
} math_mat4f;

typedef union Quat
{
    struct
    {
        float x, y, z, w;
    };
    float data[4];
#ifdef MATH_USE_SSE3
    __m128 sse_data;
#endif
} math_quatf;

// ============================== Utility functions ===================================== **/

static inline float math_to_radians(float deg) { return deg * 2.0f * (float)M_PI / 360.0f; }

/** ================================ Vector functions ==================================== **/

/** Initialise **/
static inline math_vec2f math_vec2f_init(float x, float y)
{
    math_vec2f out = {.x = x, .y = y};
    return out;
}

static inline math_vec3f math_vec3f_init(float x, float y, float z)
{
    math_vec3f out = {.x = x, .y = y, .z = z};
    return out;
}

static inline math_vec4f math_vec4f_init(float x, float y, float z, float w)
{
    math_vec4f out = {.x = x, .y = y, .z = z, .w = w};
#ifdef MATH_USE_SSE3
    out.sse_data = _mm_load_ps(out.data);
#endif
    return out;
}

static inline math_vec4f math_vec4f_init_vec3(math_vec3f v, float s)
{
    math_vec4f out = {v.x, v.y, v.z, s};
#ifdef MATH_USE_SSE3
    out.sse_data = _mm_load_ps(out.data);
#endif
    return out;
}

/** Addition **/
static inline math_vec2f math_vec2f_add(math_vec2f a, math_vec2f b)
{
    math_vec2f out = {.x = a.x + b.x, .y = a.y + b.y};
    return out;
}

static inline math_vec3f math_vec3f_add(math_vec3f a, math_vec3f b)
{
    math_vec3f out = {.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
    return out;
}

static inline math_vec4f math_vec4f_add(math_vec4f a, math_vec4f b)
{
#ifdef MATH_USE_SSE3
    math_vec4f out;
    __m128 res = _mm_add_ps(a.sse_data, b.sse_data);
    _mm_store_ps(out.data, res);
#else
    math_vec4f out = {.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z, .w = a.w + b.w};
#endif
    return out;
}

/** Subtraction **/
static inline math_vec2f math_vec2f_sub(math_vec2f a, math_vec2f b)
{
    math_vec2f out = {.x = a.x - b.x, .y = a.y - b.y};
    return out;
}

static inline math_vec3f math_vec3f_sub(math_vec3f a, math_vec3f b)
{
    math_vec3f out = {.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
    return out;
}

static inline math_vec4f math_vec4f_sub(math_vec4f a, math_vec4f b)
{
#ifdef MATH_USE_SSE3
    math_vec4f out;
    __m128 res = _mm_sub_ps(a.sse_data, b.sse_data);
    _mm_store_ps(out.data, res);
#else
    math_vec4f out = {.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z, .w = a.w - b.w};
#endif
    return out;
}

/** Multiplication **/
static inline math_vec2f math_vec2f_mul(math_vec2f a, math_vec2f b)
{
    math_vec2f out = {.x = a.x * b.x, .y = a.y * b.y};
    return out;
}

static inline math_vec3f math_vec3f_mul(math_vec3f a, math_vec3f b)
{
    math_vec3f out = {.x = a.x * b.x, .y = a.y * b.y, .z = a.z * b.z};
    return out;
}

static inline math_vec4f math_vec4f_mul(math_vec4f a, math_vec4f b)
{
#ifdef MATH_USE_SSE3
    math_vec4f out;
    __m128 res = _mm_mul_ps(a.sse_data, b.sse_data);
    _mm_store_ps(out.data, res);
#else
    math_vec4f out = {.x = a.x * b.x, .y = a.y * b.y, .z = a.z * b.z, .w = a.w * b.w};
#endif
    return out;
}

static inline math_vec2f math_vec2f_mul_sca(math_vec2f a, float s)
{
    math_vec2f out = {.x = a.x * s, .y = a.y * s};
    return out;
}

static inline math_vec3f math_vec3f_mul_sca(math_vec3f a, float s)
{
    math_vec3f out = {.x = a.x * s, .y = a.y * s, .z = a.z * s};
    return out;
}

static inline math_vec3f math_vec3f_min(math_vec3f a, math_vec3f b)
{
    math_vec3f out;
    out.x = MIN(a.x, b.x);
    out.y = MIN(a.y, b.y);
    out.z = MIN(a.z, b.z);
    return out;
}

static inline math_vec3f math_vec3f_max(math_vec3f a, math_vec3f b)
{
    math_vec3f out;
    out.x = MAX(a.x, b.x);
    out.y = MAX(a.y, b.y);
    out.z = MAX(a.z, b.z);
    return out;
}

static inline math_vec4f math_vec4f_mul_sca(math_vec4f a, float s)
{
#ifdef MATH_USE_SSE3
    math_vec4f out;
    __m128 sca = _mm_load_ps1(&s);
    __m128 res = _mm_mul_ps(a.sse_data, sca);
    _mm_store_ps(out.data, res);
#else
    math_vec4f out = {.x = a.x * s, .y = a.y * s, .z = a.z * s, .w = a.w * s};
#endif
    return out;
}

/** Division **/
static inline math_vec2f math_vec2f_div(math_vec2f a, math_vec2f b)
{
    math_vec2f out = {.x = a.x / b.x, .y = a.y / b.y};
    return out;
}

static inline math_vec3f math_vec3f_div(math_vec3f a, math_vec3f b)
{
    math_vec3f out = {.x = a.x / b.x, .y = a.y / b.y, .z = a.z / b.z};
    return out;
}

static inline math_vec4f math_vec4f_div(math_vec4f a, math_vec4f b)
{
#ifdef MATH_USE_SSE3
    math_vec4f out;
    __m128 res = _mm_div_ps(a.sse_data, b.sse_data);
    _mm_store_ps(out.data, res);
#else
    math_vec4f out = {.x = a.x / b.x, .y = a.y / b.y, .z = a.z / b.z, .w = a.w / b.w};
#endif
    return out;
}

static inline math_vec2f math_vec2f_div_sca(math_vec2f a, float s)
{
    math_vec2f out = {.x = a.x / s, .y = a.y / s};
    return out;
}

static inline math_vec3f math_vec3f_div_sca(math_vec3f a, float s)
{
    math_vec3f out = {.x = a.x / s, .y = a.y / s, .z = a.z / s};
    return out;
}

static inline math_vec4f math_vec4f_div_sca(math_vec4f a, float s)
{
#ifdef MATH_USE_SSE3
    math_vec4f out;
    __m128 sca = _mm_load_ps1(&s);
    __m128 res = _mm_div_ps(a.sse_data, sca);
    _mm_store_ps(out.data, res);
#else
    math_vec4f out = {.x = a.x / s, .y = a.y / s, .z = a.z / s, .w = a.w / s};
#endif
    return out;
}

/** Equality **/
static inline bool math_vec2f_eq(math_vec2f a, math_vec2f b) { return a.x == b.x && a.y == b.y; }

static inline bool math_vec3f_eq(math_vec3f a, math_vec3f b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

static inline bool math_vec4f_eq(math_vec4f a, math_vec4f b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

/** Dot product **/
static inline float math_vec2f_dot(math_vec2f a, math_vec2f b) { return a.x * b.x + a.y * b.y; }

static inline float math_vec3f_dot(math_vec3f a, math_vec3f b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float math_vec4f_dot(math_vec4f a, math_vec4f b)
{
    float out;
#ifdef MATH_USE_SSE3
    __m128 res = _mm_mul_ps(a.sse_data, b.sse_data);
    res = _mm_hadd_ps(res, res);
    res = _mm_hadd_ps(res, res);
    out = _mm_cvtss_f32(res);
#else
    out = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
#endif
    return out;
}

/** Cross product **/
static inline math_vec3f math_vec3f_cross(math_vec3f a, math_vec3f b)
{
    math_vec3f out;
    out.x = a.y * b.z - a.z * b.y;
    out.y = a.z * b.x - a.x * b.z;
    out.z = a.x * b.y - a.y * b.x;
    return out;
}

/** Vector length **/
static inline float math_vec2f_len(math_vec2f a) { return math_vec2f_dot(a, a); }

static inline float math_vec3f_len(math_vec3f a) { return math_vec3f_dot(a, a); }

static inline float math_vec4f_len(math_vec4f a) { return math_vec4f_dot(a, a); }

/** Vector norn (L2) **/
static inline float math_vec2f_norm(math_vec2f a) { return sqrtf(math_vec2f_len(a)); }

static inline float math_vec3f_norm(math_vec3f a) { return sqrtf(math_vec3f_len(a)); }

static inline float math_vec4f_norm(math_vec4f a) { return sqrtf(math_vec4f_len(a)); }

/** Normalise vector **/
static inline math_vec2f math_vec2f_normalise(math_vec2f a)
{
    return math_vec2f_div_sca(a, math_vec2f_norm(a));
}

static inline math_vec3f math_vec3f_normalise(math_vec3f a)
{
    return math_vec3f_div_sca(a, math_vec3f_norm(a));
}

static inline math_vec4f math_vec4f_normalise(math_vec4f a)
{
    return math_vec4f_div_sca(a, math_vec4f_norm(a));
}

/** ==================================== Matrix functions ==================================== **/
static inline math_mat3f math_mat3f_identity()
{
    math_mat3f out = {0};
    out.data[0][0] = 1.0f;
    out.data[1][1] = 1.0f;
    out.data[2][2] = 1.0f;
    return out;
}

static inline math_mat3f math_mat3f_diagonal(float d)
{
    math_mat3f out = {0};
    out.data[0][0] = d;
    out.data[1][1] = d;
    out.data[2][2] = d;
    return out;
}

static inline bool math_mat3f_eq(math_mat3f m1, math_mat3f m2)
{
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            if (m1.data[i][j] != m2.data[i][j])
            {
                return false;
            }
        }
    }
    return true;
}

static inline math_mat3f math_mat3f_abs(math_mat3f m)
{
    math_mat3f out;

    out.data[0][0] = fabsf(m.data[0][0]);
    out.data[1][0] = fabsf(m.data[1][0]);
    out.data[2][0] = fabsf(m.data[2][0]);

    out.data[0][1] = fabsf(m.data[0][1]);
    out.data[1][1] = fabsf(m.data[1][1]);
    out.data[2][1] = fabsf(m.data[2][1]);

    out.data[0][2] = fabsf(m.data[0][2]);
    out.data[1][2] = fabsf(m.data[1][2]);
    out.data[2][2] = fabsf(m.data[2][2]);

    return out;
}

static inline math_mat3f math_mat3f_transpose(math_mat3f m)
{
    math_mat3f out;
    out.data[0][0] = m.data[0][0];
    out.data[0][1] = m.data[1][0];
    out.data[0][2] = m.data[2][0];
    out.data[1][0] = m.data[0][1];
    out.data[1][1] = m.data[1][1];
    out.data[1][2] = m.data[2][1];
    out.data[2][0] = m.data[0][2];
    out.data[2][1] = m.data[1][2];
    out.data[2][2] = m.data[2][2];
    return out;
}

static inline math_mat3f math_mat3f_add(math_mat3f a, math_mat3f b)
{
    math_mat3f out;
    out.data[0][0] = a.data[0][0] + b.data[0][0];
    out.data[1][0] = a.data[1][0] + b.data[1][0];
    out.data[2][0] = a.data[2][0] + b.data[2][0];

    out.data[0][1] = a.data[0][1] + b.data[0][1];
    out.data[1][1] = a.data[1][1] + b.data[1][1];
    out.data[2][1] = a.data[2][1] + b.data[2][1];

    out.data[0][2] = a.data[0][2] + b.data[0][2];
    out.data[1][2] = a.data[1][2] + b.data[1][2];
    out.data[2][2] = a.data[2][2] + b.data[2][2];
    return out;
}

static inline math_mat3f math_mat3f_sub(math_mat3f a, math_mat3f b)
{
    math_mat3f out;
    out.data[0][0] = a.data[0][0] - b.data[0][0];
    out.data[1][0] = a.data[1][0] - b.data[1][0];
    out.data[2][0] = a.data[2][0] - b.data[2][0];

    out.data[0][1] = a.data[0][1] - b.data[0][1];
    out.data[1][1] = a.data[1][1] - b.data[1][1];
    out.data[2][1] = a.data[2][1] - b.data[2][1];

    out.data[0][2] = a.data[0][2] - b.data[0][2];
    out.data[1][2] = a.data[1][2] - b.data[1][2];
    out.data[2][2] = a.data[2][2] - b.data[2][2];
    return out;
}

static inline math_vec3f math_mat3f_mul_vec(math_mat3f m, math_vec3f v)
{
    math_vec3f out;
    out.x = v.x * m.cols[0].x;
    out.y = v.x * m.cols[0].y;
    out.z = v.x * m.cols[0].z;

    out.x += v.y * m.cols[1].x;
    out.y += v.y * m.cols[1].y;
    out.z += v.y * m.cols[1].z;

    out.x += v.z * m.cols[2].x;
    out.y += v.z * m.cols[2].y;
    out.z += v.z * m.cols[2].z;
    return out;
}

static inline math_mat3f math_mat3f_mul_sca(math_mat3f m, float s)
{
    math_mat3f out;
    out.data[0][0] = m.data[0][0] * s;
    out.data[1][0] = m.data[1][0] * s;
    out.data[2][0] = m.data[2][0] * s;

    out.data[0][1] = m.data[0][1] * s;
    out.data[1][1] = m.data[1][1] * s;
    out.data[2][1] = m.data[2][1] * s;

    out.data[0][2] = m.data[0][2] * s;
    out.data[1][2] = m.data[1][2] * s;
    out.data[2][2] = m.data[2][2] * s;
    return out;
}

static inline math_mat3f math_mat3f_mul_mat(math_mat3f m1, math_mat3f m2)
{
    math_mat3f out;
    out.cols[0] = math_mat3f_mul_vec(m1, m2.cols[0]);
    out.cols[1] = math_mat3f_mul_vec(m1, m2.cols[1]);
    out.cols[2] = math_mat3f_mul_vec(m1, m2.cols[2]);
    return out;
}

static inline math_mat4f math_mat4f_identity()
{
    math_mat4f out = {0};
    out.data[0][0] = 1.0f;
    out.data[1][1] = 1.0f;
    out.data[2][2] = 1.0f;
    out.data[3][3] = 1.0f;
    return out;
}

static inline bool math_mat4f_eq(math_mat4f m1, math_mat4f m2)
{
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            if (m1.data[i][j] != m2.data[i][j])
            {
                return false;
            }
        }
    }
    return true;
}

static inline math_mat4f math_mat4f_diagonal(float d)
{
    math_mat4f out = {0};
    out.data[0][0] = d;
    out.data[1][1] = d;
    out.data[2][2] = d;
    out.data[3][3] = d;
    return out;
}

static inline math_mat4f math_mat4f_abs(math_mat4f m)
{
    math_mat4f out;

    out.data[0][0] = fabsf(m.data[0][0]);
    out.data[1][0] = fabsf(m.data[1][0]);
    out.data[2][0] = fabsf(m.data[2][0]);
    out.data[3][0] = fabsf(m.data[3][0]);

    out.data[0][1] = fabsf(m.data[0][1]);
    out.data[1][1] = fabsf(m.data[1][1]);
    out.data[2][1] = fabsf(m.data[2][1]);
    out.data[3][1] = fabsf(m.data[3][1]);

    out.data[0][2] = fabsf(m.data[0][2]);
    out.data[1][2] = fabsf(m.data[1][2]);
    out.data[2][2] = fabsf(m.data[2][2]);
    out.data[3][2] = fabsf(m.data[3][2]);

    out.data[0][3] = fabsf(m.data[0][3]);
    out.data[1][3] = fabsf(m.data[1][3]);
    out.data[2][3] = fabsf(m.data[2][3]);
    out.data[3][3] = fabsf(m.data[3][3]);

    return out;
}

static inline math_mat4f math_mat4f_transpose(math_mat4f m)
{
    math_mat4f out;
    out.data[0][1] = m.data[1][0];
    out.data[0][2] = m.data[2][0];
    out.data[0][3] = m.data[3][0];

    out.data[1][0] = m.data[0][1];
    out.data[1][2] = m.data[2][1];
    out.data[1][3] = m.data[3][1];

    out.data[2][0] = m.data[0][2];
    out.data[2][1] = m.data[1][2];
    out.data[2][3] = m.data[3][2];

    out.data[3][0] = m.data[0][3];
    out.data[3][1] = m.data[1][3];
    out.data[3][2] = m.data[2][3];
    return out;
}

static inline math_mat4f math_mat4f_add(math_mat4f a, math_mat4f b)
{
    math_mat4f out;
    out.data[0][0] = a.data[0][0] + b.data[0][0];
    out.data[1][0] = a.data[1][0] + b.data[1][0];
    out.data[2][0] = a.data[2][0] + b.data[2][0];
    out.data[3][0] = a.data[3][0] + b.data[3][0];

    out.data[0][1] = a.data[0][1] + b.data[0][1];
    out.data[1][1] = a.data[1][1] + b.data[1][1];
    out.data[2][1] = a.data[2][1] + b.data[2][1];
    out.data[3][1] = a.data[3][1] + b.data[3][1];

    out.data[0][2] = a.data[0][2] + b.data[0][2];
    out.data[1][2] = a.data[1][2] + b.data[1][2];
    out.data[2][2] = a.data[2][2] + b.data[2][2];
    out.data[3][2] = a.data[3][2] + b.data[3][2];

    out.data[0][3] = a.data[0][3] + b.data[0][3];
    out.data[1][3] = a.data[1][3] + b.data[1][3];
    out.data[2][3] = a.data[2][3] + b.data[2][3];
    out.data[3][3] = a.data[3][3] + b.data[3][3];
    return out;
}

static inline math_mat4f math_mat4f_sub(math_mat4f a, math_mat4f b)
{
    math_mat4f out;
    out.data[0][0] = a.data[0][0] - b.data[0][0];
    out.data[1][0] = a.data[1][0] - b.data[1][0];
    out.data[2][0] = a.data[2][0] - b.data[2][0];
    out.data[3][0] = a.data[3][0] - b.data[3][0];

    out.data[0][1] = a.data[0][1] - b.data[0][1];
    out.data[1][1] = a.data[1][1] - b.data[1][1];
    out.data[2][1] = a.data[2][1] - b.data[2][1];
    out.data[3][1] = a.data[3][1] - b.data[3][1];

    out.data[0][2] = a.data[0][2] - b.data[0][2];
    out.data[1][2] = a.data[1][2] - b.data[1][2];
    out.data[2][2] = a.data[2][2] - b.data[2][2];
    out.data[3][2] = a.data[3][2] - b.data[3][2];

    out.data[0][3] = a.data[0][3] - b.data[0][3];
    out.data[1][3] = a.data[1][3] - b.data[1][3];
    out.data[2][3] = a.data[2][3] - b.data[2][3];
    out.data[3][3] = a.data[3][3] - b.data[3][3];
    return out;
}

static inline math_mat4f math_mat4f_mul_sca(math_mat4f m, float s)
{
    math_mat4f out;

#ifdef MATH_USE_SSE3
    __m128 scalar = _mm_load_ps1(&s);

    __m128 res = _mm_mul_ps(m.cols[0].sse_data, scalar);
    _mm_store_ps(out.data[0], res);
    res = _mm_mul_ps(m.cols[1].sse_data, scalar);
    _mm_store_ps(out.data[1], res);
    res = _mm_mul_ps(m.cols[2].sse_data, scalar);
    _mm_store_ps(out.data[2], res);
    res = _mm_mul_ps(m.cols[3].sse_data, scalar);
    _mm_store_ps(out.data[3], res);
#endif

    return out;
}

static inline math_mat4f math_mat4f_mul(math_mat4f a, math_mat4f b)
{
    math_mat4f out;

#ifdef MATH_USE_SSE3
    float* mat_data_a = (float*)a.data;
    float* mat_data_b = (float*)b.data;
    float* mat_data_c = (float*)out.data;
    __m128 row1 = _mm_loadu_ps(mat_data_a);
    __m128 row2 = _mm_loadu_ps(mat_data_a + 4);
    __m128 row3 = _mm_loadu_ps(mat_data_a + 8);
    __m128 row4 = _mm_loadu_ps(mat_data_a + 12);
    for (int i = 0; i < 4; i++)
    {
        __m128 brod1 = _mm_set1_ps(mat_data_b[4 * i + 0]);
        __m128 brod2 = _mm_set1_ps(mat_data_b[4 * i + 1]);
        __m128 brod3 = _mm_set1_ps(mat_data_b[4 * i + 2]);
        __m128 brod4 = _mm_set1_ps(mat_data_b[4 * i + 3]);
        __m128 row = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(brod1, row1), _mm_mul_ps(brod2, row2)),
            _mm_add_ps(_mm_mul_ps(brod3, row3), _mm_mul_ps(brod4, row4)));
        _mm_storeu_ps(mat_data_c + 4 * i, row);
    }
#endif
    return out;
}

static inline math_vec4f math_mat4f_mul_vec(math_mat4f m, math_vec4f v)
{
    math_vec4f out;
    out.x = v.data[0] * m.cols[0].x;
    out.y = v.data[0] * m.cols[0].y;
    out.z = v.data[0] * m.cols[0].z;
    out.w = v.data[0] * m.cols[0].w;

    out.x += v.data[1] * m.cols[1].x;
    out.y += v.data[1] * m.cols[1].y;
    out.z += v.data[1] * m.cols[1].z;
    out.w += v.data[1] * m.cols[1].w;

    out.x += v.data[2] * m.cols[2].x;
    out.y += v.data[2] * m.cols[2].y;
    out.z += v.data[2] * m.cols[2].z;
    out.w += v.data[2] * m.cols[2].w;

    out.x += v.data[3] * m.cols[3].x;
    out.y += v.data[3] * m.cols[3].y;
    out.z += v.data[3] * m.cols[3].z;
    out.w += v.data[3] * m.cols[3].w;
    return out;
}

static inline math_mat4f math_mat4f_inverse(math_mat4f m)
{
    math_mat4f out = math_mat4f_identity();

    for (size_t i = 0; i < 4; ++i)
    {
        // Find the largest element in the i'th column.
        size_t pivot = i;
        float t = fabsf(m.data[i][i]);
        for (size_t j = i + 1; j < 4; ++j)
        {
            float tmp = fabsf(m.data[j][i]);
            if (tmp > t)
            {
                pivot = j;
                t = tmp;
            }
        }
        // Matrix is singular so don't continue.
        if (t == 0)
        {
            return out;
        }

        if (pivot != i)
        {
            // Swap columns.
            math_vec4f tmp = m.cols[i];
            m.cols[i] = m.cols[pivot];
            m.cols[pivot] = tmp;

            tmp = out.cols[i];
            out.cols[i] = out.cols[pivot];
            out.cols[pivot] = tmp;
        }

        float denom = m.data[i][i];
        for (size_t k = 0; k < 4; ++k)
        {
            m.data[i][k] /= denom;
            out.data[i][k] /= denom;
        }

        // Factor out the lower triangle.
        for (size_t j = 0; j < 4; ++j)
        {
            if (j != i)
            {
                float t = m.data[j][i];
                for (size_t k = 0; k < 4; ++k)
                {
                    m.data[j][k] -= m.data[i][k] * t;
                    out.data[j][k] -= out.data[i][k] * t;
                }
            }
        }
    }

    return out;
}

/** Useful graphic functions **/

static inline void math_mat4f_translate(math_vec3f v, math_mat4f* m)
{
    m->cols[3].x = v.x;
    m->cols[3].y = v.y;
    m->cols[3].z = v.z;
}

static inline void math_mat4f_scale(math_vec3f s, math_mat4f* m)
{
    m->data[0][0] = s.x;
    m->data[1][1] = s.y;
    m->data[2][2] = s.z;
}

static inline void math_mat4f_from_mat3f(math_mat3f r, math_mat4f* m)
{
    m->data[0][0] = r.data[0][0];
    m->data[0][1] = r.data[0][1];
    m->data[0][2] = r.data[0][2];
    m->data[1][0] = r.data[1][0];
    m->data[1][1] = r.data[1][1];
    m->data[1][2] = r.data[1][2];
    m->data[2][0] = r.data[2][0];
    m->data[2][1] = r.data[2][1];
    m->data[2][2] = r.data[2][2];
}

static inline math_mat4f math_mat4f_lookat(math_vec3f target, math_vec3f eye, math_vec3f up)
{
    math_vec3f dir = math_vec3f_normalise(math_vec3f_sub(target, eye));
    math_vec3f right = math_vec3f_normalise(math_vec3f_cross(up, dir));
    math_vec3f cam_up = math_vec3f_cross(dir, right);

    math_mat4f m = {0.0f};
    m.data[0][0] = right.x;
    m.data[1][0] = right.y;
    m.data[2][0] = right.z;

    m.data[0][1] = cam_up.x;
    m.data[1][1] = cam_up.y;
    m.data[2][1] = cam_up.z;

    m.data[0][2] = dir.x;
    m.data[1][2] = dir.y;
    m.data[2][2] = dir.z;

    m.data[3][0] = -math_vec3f_dot(right, eye);
    m.data[3][1] = -math_vec3f_dot(cam_up, eye);
    m.data[3][2] = -math_vec3f_dot(dir, eye);
    m.data[3][3] = 1.0f;

    return m;
}

static inline math_mat4f
math_mat4f_frustum(float left, float right, float bottom, float top, float near, float far)
{
    math_mat4f m = {0};
    m.data[0][0] = (2.0f * near) / (right - left);
    m.data[1][1] = -(2.0f * near) / (top - bottom);
    m.data[2][0] = (right + left) / (right - left);
    m.data[2][1] = (top + bottom) / (top - bottom);
    m.data[2][2] = (far + near) / (far - near);
    m.data[2][3] = 1.0f;
    m.data[3][2] = -(2.0f * far * near) / (far - near);
    return m;
}

static inline math_mat4f
math_mat4f_projection(float fov_y, float aspect_ratio, float near_z, float far_z)
{
    float w = tanf(fov_y) * near_z;
    float h = w / aspect_ratio;
    return math_mat4f_frustum(-w, w, -h, h, near_z, far_z);
}

static inline math_mat4f math_mat4f_rotate_rh(float angle, math_vec3f axis)
{
    math_mat4f out = math_mat4f_identity();

    float sin_theta = sinf(angle);
    float cos_theta = cosf(angle);
    float cos_val = 1.0f - cos_theta;

    out.data[0][0] = (axis.x * axis.x * cos_val) + cos_theta;
    out.data[0][1] = (axis.x * axis.y * cos_val) + (axis.z * sin_theta);
    out.data[0][1] = (axis.x * axis.z * cos_val) + (axis.y * sin_theta);

    out.data[1][0] = (axis.y * axis.x * cos_val) - (axis.z * sin_theta);
    out.data[1][1] = (axis.y * axis.y * cos_val) + cos_theta;
    out.data[1][2] = (axis.y * axis.z * cos_val) + (axis.x * sin_theta);

    out.data[2][0] = (axis.z * axis.x * cos_val) + (axis.y * sin_theta);
    out.data[2][1] = (axis.z * axis.y * cos_val) - (axis.x * sin_theta);
    out.data[2][2] = (axis.z * axis.z * cos_val) + cos_theta;
    return out;
}

static inline math_mat4f math_mat4f_rotate_lh(float angle, math_vec3f axis)
{
    return math_mat4f_rotate_rh(-angle, axis);
}

static inline math_mat3f math_mat4f_to_rotation_matrix(math_mat4f m)
{
    math_mat3f out;
    out.data[0][0] = m.data[0][0];
    out.data[0][1] = m.data[0][1];
    out.data[0][2] = m.data[0][2];
    out.data[1][0] = m.data[1][0];
    out.data[1][1] = m.data[1][1];
    out.data[1][2] = m.data[1][2];
    out.data[2][0] = m.data[2][0];
    out.data[2][1] = m.data[2][1];
    out.data[2][2] = m.data[2][2];
    return out;
}

static inline math_vec3f math_mat4f_translation_vec(math_mat4f m)
{
    math_vec3f out = {.x = m.data[3][0], .y = m.data[3][1], .z = m.data[3][2]};
    return out;
}

/** ================================ Quaternion functions ================================= **/

static inline math_quatf math_quatf_init(float x, float y, float z, float w)
{
    math_quatf out = {.x = x, .y = y, .z = z, .w = w};
#ifdef MATH_USE_SSE3
    out.sse_data = _mm_load_ps(out.data);
#endif
    return out;
}

static inline math_quatf math_quatf_norm(math_quatf q)
{
    // Use vector4 version.
    math_vec4f v = {q.x, q.y, q.z, q.w};
    math_vec4f n = math_vec4f_normalise(v);
    math_quatf out = {n.x, n.y, n.z, n.w};
    return out;
}

static inline math_mat4f math_quatf_to_mat4f(math_quatf q)
{
    math_mat4f out;

    math_quatf nq = math_quatf_norm(q);

    float xx = nq.x * nq.x;
    float yy = nq.y * nq.y;
    float zz = nq.z * nq.z;
    float xy = nq.x * nq.y;
    float xz = nq.x * nq.z;
    float yz = nq.y * nq.z;
    float wx = nq.w * nq.x;
    float wy = nq.w * nq.y;
    float wz = nq.w * nq.z;

    out.data[0][0] = 1.0f - 2.0f * (yy + zz);
    out.data[0][1] = 2.0f * (xy + wz);
    out.data[0][2] = 2.0f * (xz - wy);
    out.data[0][3] = 0.0f;

    out.data[1][0] = 2.0f * (xy - wz);
    out.data[1][1] = 1.0f - 2.0f * (xx + zz);
    out.data[1][2] = 2.0f * (yz + wx);
    out.data[1][3] = 0.0f;

    out.data[2][0] = 2.0f * (xz + wy);
    out.data[2][1] = 2.0f * (yz - wx);
    out.data[2][2] = 1.0f - 2.0f * (xx + yy);
    out.data[2][3] = 0.0f;

    out.data[3][0] = 0.0f;
    out.data[3][1] = 0.0f;
    out.data[3][2] = 0.0f;
    out.data[3][3] = 1.0f;

    return out;
}

#endif
