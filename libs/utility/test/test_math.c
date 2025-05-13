#include "unity.h"
#include "unity_fixture.h"
#include "utility/maths.h"

#include <string.h>

TEST_GROUP(MathGroup);

TEST_SETUP(MathGroup) {}

TEST_TEAR_DOWN(MathGroup) {}

TEST(MathGroup, MathTests_Vector2)
{
    math_vec2f a = math_vec2f_init(3.0f, 3.0f);
    math_vec2f b = math_vec2f_init(9.0f, 2.0f);
    math_vec2f a_plus_b = math_vec2f_add(a, b);
    TEST_ASSERT_EQUAL_FLOAT(12.0f, a_plus_b.x);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, a_plus_b.y);

    math_vec2f a_sub_b = math_vec2f_sub(a, b);
    TEST_ASSERT_EQUAL_FLOAT(-6.0f, a_sub_b.x);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, a_sub_b.y);

    math_vec2f a_mul_b = math_vec2f_mul(a, b);
    TEST_ASSERT_EQUAL_FLOAT(27.0f, a_mul_b.x);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, a_mul_b.y);

    math_vec2f a_div_b = math_vec2f_div(a, b);
    TEST_ASSERT_EQUAL_FLOAT(0.3333334f, a_div_b.x);
    TEST_ASSERT_EQUAL_FLOAT(1.5f, a_div_b.y);

    float len = math_vec2f_len(a);
    TEST_ASSERT_EQUAL_FLOAT(18.0f, len);
    float norm = math_vec2f_norm(a);
    TEST_ASSERT_EQUAL_FLOAT(4.24264f, norm);

    TEST_ASSERT(math_vec2f_eq(a, b) == false);
    TEST_ASSERT(math_vec2f_eq(a, a) == true);
}

TEST(MathGroup, MathTests_Vector3)
{
    math_vec3f a = math_vec3f_init(3.0f, 5.0f, 1.0f);
    math_vec3f b = math_vec3f_init(4.0f, 2.0f, 3.0f);
    math_vec3f a_plus_b = math_vec3f_add(a, b);
    TEST_ASSERT_EQUAL_FLOAT(7.0f, a_plus_b.x);
    TEST_ASSERT_EQUAL_FLOAT(7.0f, a_plus_b.y);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, a_plus_b.z);

    math_vec3f a_sub_b = math_vec3f_sub(a, b);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, a_sub_b.x);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, a_sub_b.y);
    TEST_ASSERT_EQUAL_FLOAT(-2.0f, a_sub_b.z);

    math_vec3f a_mul_b = math_vec3f_mul(a, b);
    TEST_ASSERT_EQUAL_FLOAT(12.0f, a_mul_b.x);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, a_mul_b.y);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, a_mul_b.z);

    math_vec3f a_div_b = math_vec3f_div(a, b);
    TEST_ASSERT_EQUAL_FLOAT(0.75f, a_div_b.x);
    TEST_ASSERT_EQUAL_FLOAT(2.5f, a_div_b.y);
    TEST_ASSERT_EQUAL_FLOAT(0.3333334f, a_div_b.z);

    float len = math_vec3f_len(a);
    TEST_ASSERT_EQUAL_FLOAT(35.0f, len);
    float norm = math_vec3f_norm(a);
    TEST_ASSERT_EQUAL_FLOAT(5.91608f, norm);

    TEST_ASSERT(math_vec3f_eq(a, b) == false);
    TEST_ASSERT(math_vec3f_eq(a, a) == true);

    math_vec3f c = math_vec3f_init(1.0f, 2.0f, 3.0f);
    math_vec3f d = math_vec3f_init(4.0f, 5.0f, 6.0f);
    math_vec3f result = math_vec3f_cross(c, d);
    TEST_ASSERT_EQUAL_FLOAT(-3.0f, result.x);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, result.y);
    TEST_ASSERT_EQUAL_FLOAT(-3.0f, result.z);
}

TEST(MathGroup, MathTests_Vector4)
{
    math_vec4f a = math_vec4f_init(3.0f, 5.0f, 1.0f, 8.0f);
    math_vec4f b = math_vec4f_init(4.0f, 2.0f, 3.0f, 4.0f);
    math_vec4f a_plus_b = math_vec4f_add(a, b);
    TEST_ASSERT_EQUAL_FLOAT(7.0f, a_plus_b.x);
    TEST_ASSERT_EQUAL_FLOAT(7.0f, a_plus_b.y);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, a_plus_b.z);
    TEST_ASSERT_EQUAL_FLOAT(12.0f, a_plus_b.w);

    math_vec4f a_sub_b = math_vec4f_sub(a, b);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, a_sub_b.x);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, a_sub_b.y);
    TEST_ASSERT_EQUAL_FLOAT(-2.0f, a_sub_b.z);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, a_sub_b.w);

    math_vec4f a_mul_b = math_vec4f_mul(a, b);
    TEST_ASSERT_EQUAL_FLOAT(12.0f, a_mul_b.x);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, a_mul_b.y);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, a_mul_b.z);
    TEST_ASSERT_EQUAL_FLOAT(32.0f, a_mul_b.w);

    math_vec4f a_div_b = math_vec4f_div(a, b);
    TEST_ASSERT_EQUAL_FLOAT(0.75f, a_div_b.x);
    TEST_ASSERT_EQUAL_FLOAT(2.5f, a_div_b.y);
    TEST_ASSERT_EQUAL_FLOAT(0.3333334f, a_div_b.z);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, a_div_b.w);

    float d = math_vec4f_dot(a, b);
    TEST_ASSERT_EQUAL_FLOAT(57.0f, d);

    float len = math_vec4f_len(a);
    TEST_ASSERT_EQUAL_FLOAT(99.0f, len);
    float norm = math_vec4f_norm(a);
    TEST_ASSERT_EQUAL_FLOAT(9.949874f, norm);

    TEST_ASSERT(math_vec4f_eq(a, b) == false);
    TEST_ASSERT(math_vec4f_eq(a, a) == true);
}

TEST(MathGroup, MathTests_Mat3)
{
    math_mat3f m1;
    m1.cols[0] = math_vec3f_init(2.0f, 3.0f, 4.0f);
    m1.cols[1] = math_vec3f_init(1.0f, 2.0f, 3.0f);
    m1.cols[2] = math_vec3f_init(3.0f, 3.0f, 5.0f);

    math_mat3f tr = math_mat3f_transpose(m1);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, tr.data[0][0]);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, tr.data[0][1]);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, tr.data[0][2]);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, tr.data[1][0]);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, tr.data[1][1]);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, tr.data[1][2]);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, tr.data[2][0]);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, tr.data[2][1]);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, tr.data[2][2]);

    math_mat3f res = math_mat3f_mul_sca(m1, 2.0f);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, res.data[0][0]);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, res.data[0][1]);
    TEST_ASSERT_EQUAL_FLOAT(8.0f, res.data[0][2]);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, res.data[1][0]);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, res.data[1][1]);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, res.data[1][2]);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, res.data[2][0]);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, res.data[2][1]);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, res.data[2][2]);

    math_mat3f m2;
    m2.cols[0] = math_vec3f_init(4.0f, 5.0f, 1.0f);
    m2.cols[1] = math_vec3f_init(2.0f, 1.0f, 3.0f);
    m2.cols[2] = math_vec3f_init(1.0f, 1.0f, 3.0f);
    res = math_mat3f_add(m1, m2);

    TEST_ASSERT_EQUAL_FLOAT(6.0f, res.data[0][0]);
    TEST_ASSERT_EQUAL_FLOAT(8.0f, res.data[0][1]);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, res.data[0][2]);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, res.data[1][0]);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, res.data[1][1]);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, res.data[1][2]);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, res.data[2][0]);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, res.data[2][1]);
    TEST_ASSERT_EQUAL_FLOAT(8.0f, res.data[2][2]);
}

TEST(MathGroup, MathTests_Mat4)
{
    math_mat4f m1 = math_mat4f_identity();
    math_mat4f m2 = math_mat4f_diagonal(2.0f);
    m2.data[0][1] = 5.0f;
    m2.data[1][3] = 7.0f;
    m2.data[2][3] = 3.0f;
    math_mat4f m3 = math_mat4f_mul(m1, m2);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, m3.data[0][0]);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, m3.data[1][1]);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, m3.data[2][2]);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, m3.data[3][3]);

    memset(&m1, 0, sizeof(math_mat4f));
    memset(&m2, 0, sizeof(math_mat4f));

    int counter = 1;
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            m1.data[col][row] = (float)counter++;
        }
    }
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            m2.data[col][row] = (float)counter++;
        }
    }

    m3 = math_mat4f_mul(m1, m2);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[0][0], 538.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[0][1], 612.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[0][2], 686.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[0][3], 760.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[1][0], 650.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[1][1], 740.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[1][2], 830.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[1][3], 920.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[2][0], 762.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[2][1], 868.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[2][2], 974.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[2][3], 1080.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[3][0], 874.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[3][1], 996.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[3][2], 1118.0f);
    TEST_ASSERT_EQUAL_FLOAT(m3.data[3][3], 1240.0f);

    m1.cols[0] = math_vec4f_init(2.0f, 3.0f, 4.0f, 6.0f);
    m1.cols[1] = math_vec4f_init(1.0f, 2.0f, 3.0f, 1.0f);
    m1.cols[2] = math_vec4f_init(3.0f, 3.0f, 5.0f, 2.0f);
    m1.cols[3] = math_vec4f_init(1.0f, 1.0f, 4.0f, 1.0f);

    math_mat4f res = math_mat4f_mul_sca(m1, 2.0f);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, res.data[0][0]);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, res.data[0][1]);
    TEST_ASSERT_EQUAL_FLOAT(8.0f, res.data[0][2]);
    TEST_ASSERT_EQUAL_FLOAT(12.0f, res.data[0][3]);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, res.data[1][0]);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, res.data[1][1]);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, res.data[1][2]);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, res.data[1][3]);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, res.data[2][0]);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, res.data[2][1]);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, res.data[2][2]);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, res.data[2][3]);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, res.data[3][0]);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, res.data[3][1]);
    TEST_ASSERT_EQUAL_FLOAT(8.0f, res.data[3][2]);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, res.data[3][3]);

    // clang-format off
    math_mat4f m = {
        12.0f, 2.0f, 1.0f, 1.0f,
        0.0f,  0.0f, 1.0f, 1.0f,
        0.0f,  1.0f, 5.0f, 1.0f,
        11.0f, 1.0f, 0.0f, 10.0f};
    // clang-format on

    math_mat4f expected = math_mat4f_identity();
    math_mat4f inverse = math_mat4f_inverse(m);
    res = math_mat4f_mul(m, inverse);
    math_mat4f_print(res);

    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[0][0], expected.data[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[0][1], expected.data[0][1]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[0][2], expected.data[0][2]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[0][3], expected.data[0][3]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[1][0], expected.data[1][0]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[1][1], expected.data[1][1]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[1][2], expected.data[1][2]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[1][3], expected.data[1][3]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[2][0], expected.data[2][0]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[2][1], expected.data[2][1]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[2][2], expected.data[2][2]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[2][3], expected.data[2][3]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[3][0], expected.data[3][0]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[3][1], expected.data[3][1]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[3][2], expected.data[3][2]);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, res.data[3][3], expected.data[3][3]);
}