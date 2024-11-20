#include "vk_setup.h"

#include <aabox.h>
#include <frustum.h>
#include <unity_fixture.h>

TEST_GROUP(VisibilityGroup);

TEST_SETUP(VisibilityGroup) {}

TEST_TEAR_DOWN(VisibilityGroup) {}

TEST(VisibilityGroup, AABBox_Test)
{
    rpe_aabox_t box = {.min = {6.0f, 4.0f, 0.0f}, .max = {10.0f, 8.0f, 2.0f}};

    math_vec3f half = rpe_aabox_get_half_extent(&box);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, half.x);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, half.y);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, half.z);

    math_vec3f center = rpe_aabox_get_center(&box);
    TEST_ASSERT_EQUAL_FLOAT(8.0f, center.x);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, center.y);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, center.z);

    math_mat4f world = math_mat4f_identity();
    rpe_aabox_t rigid_box = rpe_aabox_calc_rigid_transform(&box, &world);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, rigid_box.min.x);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, rigid_box.min.y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rigid_box.min.z);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, rigid_box.max.x);
    TEST_ASSERT_EQUAL_FLOAT(8.0f, rigid_box.max.y);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, rigid_box.max.z);
}