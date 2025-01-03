#include "vk_setup.h"

#include <commands.h>
#include <unity_fixture.h>

int bucket_test_val1 = 0;

struct BucketTestCommand1
{
    int add_val;
};

struct BucketTestCommand2
{
    void* data;
};

TEST_GROUP(CommandsGroup);

TEST_SETUP(CommandsGroup) {}

TEST_TEAR_DOWN(CommandsGroup) {}

void testBucketFunc1(vkapi_driver_t* driver, void* data)
{
    struct BucketTestCommand1* cmd = (struct BucketTestCommand1*)data;
    bucket_test_val1 += cmd->add_val;
}

void testBucketFunc2(vkapi_driver_t* driver, void* data)
{
    struct BucketTestCommand2* cmd = (struct BucketTestCommand2*)data;
    int val = *(int*)cmd->data;
    bucket_test_val1 *= val;
}

TEST(CommandsGroup, BasicCommands_Test)
{
    arena_t* arena = setup_arena(1 << 20);
    rpe_cmd_bucket_t* bucket = rpe_command_bucket_init(10, arena);
    TEST_ASSERT_NOT_NULL(bucket);
    rpe_cmd_packet_t* pkt0 = rpe_command_bucket_add_command(
        bucket, 0, sizeof(struct BucketTestCommand1), arena, testBucketFunc1);
    struct BucketTestCommand1* cmd = pkt0->cmds;
    cmd->add_val = 5;

    rpe_cmd_packet_t* pkt1 = rpe_command_bucket_append_command(
        bucket, pkt0, 0, sizeof(struct BucketTestCommand1), arena, testBucketFunc1);
    struct BucketTestCommand1* cmd1 = pkt1->cmds;
    cmd1->add_val = 10;

    rpe_cmd_packet_t* pkt2 = rpe_command_bucket_append_command(
        bucket, pkt1, sizeof(int), sizeof(struct BucketTestCommand2), arena, testBucketFunc2);
    struct BucketTestCommand2* cmd2 = pkt2->cmds;
    cmd2->data = pkt2->data;
    int val = 2;
    memcpy(pkt2->data, &val, sizeof(int));

    rpe_command_bucket_submit(bucket, NULL);

    TEST_ASSERT_EQUAL_UINT(30, bucket_test_val1);
}