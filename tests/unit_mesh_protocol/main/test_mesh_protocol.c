#include "unity.h"
#include "unity_test_runner.h"
#include "mesh_protocol.h"
#include "cJSON.h"
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

TEST_CASE("mesh_protocol_validate_structure accepts valid JSON", "[mesh_protocol]")
{
    const char *json = "{\"type\":\"telemetry\",\"node_id\":\"node-01\",\"data\":{\"value\":42}}";

    TEST_ASSERT_TRUE(mesh_protocol_validate_structure(json, NULL));
}

TEST_CASE("mesh_protocol_validate_structure detects invalid JSON", "[mesh_protocol]")
{
    const char *json = "{\"type\":\"telemetry\",\"node_id\":\"node-01\"";
    const char *error_ptr = NULL;

    TEST_ASSERT_FALSE(mesh_protocol_validate_structure(json, &error_ptr));
    TEST_ASSERT_NOT_NULL(error_ptr);
}

TEST_CASE("mesh_protocol_validate_structure handles NULL pointer", "[mesh_protocol]")
{
    TEST_ASSERT_FALSE(mesh_protocol_validate_structure(NULL, NULL));
}

TEST_CASE("mesh_protocol_parse fails when type field missing", "[mesh_protocol]")
{
    const char *json = "{\"node_id\":\"node-02\"}";
    mesh_message_t msg;
    memset(&msg, 0xAA, sizeof(msg));

    TEST_ASSERT_FALSE(mesh_protocol_parse(json, &msg));
    TEST_ASSERT_EQUAL_UINT32(0, msg.type);
    TEST_ASSERT_EQUAL_CHAR('\0', msg.node_id[0]);
    TEST_ASSERT_EQUAL_CHAR('\0', msg.root_node_id[0]);
    TEST_ASSERT_EQUAL_UINT64(0, msg.timestamp);
    TEST_ASSERT_NULL(msg.data);
}

TEST_CASE("mesh_protocol_parse populates message for telemetry", "[mesh_protocol]")
{
    const char *json = "{\"type\":\"telemetry\",\"node_id\":\"sensor-01\",\"root_node_id\":\"root-1\",\"timestamp\":1700000000,\"data\":{\"temperature\":23.5}}";
    mesh_message_t msg;
    memset(&msg, 0xAA, sizeof(msg));

    TEST_ASSERT_TRUE(mesh_protocol_parse(json, &msg));
    TEST_ASSERT_EQUAL_UINT32(MESH_MSG_TELEMETRY, msg.type);
    TEST_ASSERT_EQUAL_STRING("sensor-01", msg.node_id);
    TEST_ASSERT_EQUAL_STRING("root-1", msg.root_node_id);
    TEST_ASSERT_EQUAL_UINT64(1700000000ULL, msg.timestamp);
    TEST_ASSERT_NOT_NULL(msg.data);

    const cJSON *temp = cJSON_GetObjectItem(msg.data, "temperature");
    TEST_ASSERT_NOT_NULL(temp);
    TEST_ASSERT_TRUE(cJSON_IsNumber(temp));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 23.5f, (float)temp->valuedouble);

    mesh_protocol_free_message(&msg);
}

void app_main(void)
{
    unity_run_menu();
}

