/**
 * @file test_app_biphasic.c
 * @brief Test the biphasic stimulation waveform command handling
 * @version 0.1.00
 */
#include "test_app_biphasic.h"

#include "app_cmd.h"
#include "unity.h"
#include "crc16.h"
#include "nrf_delay.h"

#include <string.h>

#define OP_SET_BIPHASIC_PARAMETERS	0x4C
#define OP_GET_BIPHASIC_PARAMETERS	0x4D

#define BIPHASIC_PAYLOAD_LEN		24

#define STATUS_CHARGE_IMBALANCE_WARNING	0x02

/**
 * @brief Helper to build a command request with opcode, payload, and CRC
 * 
 * @param buf Output buffer for the command
 * @param opcode The opcode byte
 * @param payload Pointer to the payload data (may be NULL)
 * @param payload_len Length of the payload
 * @return uint16_t Total length of the built command
 */
static uint16_t build_cmd_request(uint8_t* buf, uint8_t opcode, uint8_t* payload, uint8_t payload_len)
{
	buf[0] = opcode;
	buf[1] = payload_len;
	if (payload != NULL && payload_len > 0)
		memcpy(buf + REQ_HEADER_LEN, payload, payload_len);

	uint16_t total_len = REQ_HEADER_LEN + payload_len + CRC_LEN;
	uint16_t crc = crc16_compute(buf, total_len - CRC_LEN, NULL);
	memcpy(buf + total_len - CRC_LEN, (uint8_t*)&crc, CRC_LEN);
	return total_len;
}

/**
 * @brief Helper to write a uint32_t in little-endian into a buffer
 * 
 * @param buf Destination buffer
 * @param val The uint32_t value to write
 */
static void write_u32_le(uint8_t* buf, uint32_t val)
{
	memcpy(buf, (uint8_t*)&val, sizeof(uint32_t));
}

/**
 * @brief Test that a valid SET_BIPHASIC_PARAMETERS command returns STATUS_SUCCESS and correct CRC
 * 
 */
static void test_biphasic_set_parameters_valid(void)
{
	uint8_t payload[BIPHASIC_PAYLOAD_LEN];
	write_u32_le(&payload[0],  500);       /* cathodic_width_us */
	write_u32_le(&payload[4],  500);       /* anodic_width_us */
	write_u32_le(&payload[8],  100);       /* interphase_gap_us */
	write_u32_le(&payload[12], 20000);     /* pulse_period_us */
	write_u32_le(&payload[16], 30000);     /* train_on_duration_ms */
	write_u32_le(&payload[20], 60000);     /* train_off_duration_ms */

	uint8_t cmd_buf[64];
	uint16_t cmd_len = build_cmd_request(cmd_buf, OP_SET_BIPHASIC_PARAMETERS, payload, BIPHASIC_PAYLOAD_LEN);

	app_cmd_resp_t* p_resp = app_cmd_parse_request(cmd_buf, cmd_len);

	uint16_t exp_crc16 = crc16_compute(p_resp->cmd_resp, p_resp->cmd_resp_len - CRC_LEN, NULL);
	uint16_t act_crc16 = *((uint16_t*)&p_resp->cmd_resp[p_resp->cmd_resp_len - CRC_LEN]);
	uint8_t exp_len = p_resp->cmd_resp_len;
	uint8_t act_len = p_resp->cmd_resp[1] + RESP_HEADER_LEN + CRC_LEN;
	uint8_t exp_status = STATUS_SUCCESS;
	uint8_t act_status = p_resp->cmd_resp[2];

	TEST_ASSERT_EQUAL_UINT16(exp_crc16, act_crc16);
	TEST_ASSERT_EQUAL_UINT8(exp_len, act_len);
	TEST_ASSERT_EQUAL_UINT8(exp_status, act_status);
	nrf_delay_ms(500);
}

/**
 * @brief Test that SET_BIPHASIC_PARAMETERS with incorrect payload length returns STATUS_PAYLOAD_LEN_ERR
 * 
 */
static void test_biphasic_set_parameters_invalid_payload_length(void)
{
	uint8_t payload[10];
	memset(payload, 0, sizeof(payload));

	uint8_t cmd_buf[64];
	uint16_t cmd_len = build_cmd_request(cmd_buf, OP_SET_BIPHASIC_PARAMETERS, payload, 10);

	app_cmd_resp_t* p_resp = app_cmd_parse_request(cmd_buf, cmd_len);

	uint16_t exp_crc16 = crc16_compute(p_resp->cmd_resp, p_resp->cmd_resp_len - CRC_LEN, NULL);
	uint16_t act_crc16 = *((uint16_t*)&p_resp->cmd_resp[p_resp->cmd_resp_len - CRC_LEN]);
	uint8_t exp_status = STATUS_PAYLOAD_LEN_ERR;
	uint8_t act_status = p_resp->cmd_resp[2];

	TEST_ASSERT_EQUAL_UINT16(exp_crc16, act_crc16);
	TEST_ASSERT_EQUAL_UINT8(exp_status, act_status);
	nrf_delay_ms(500);
}

/**
 * @brief Test that setting cathodic width to 0 returns STATUS_INVALID
 * 
 */
static void test_biphasic_set_parameters_zero_cathodic_width(void)
{
	uint8_t payload[BIPHASIC_PAYLOAD_LEN];
	write_u32_le(&payload[0],  0);         /* cathodic_width_us = 0 (invalid) */
	write_u32_le(&payload[4],  500);       /* anodic_width_us */
	write_u32_le(&payload[8],  100);       /* interphase_gap_us */
	write_u32_le(&payload[12], 20000);     /* pulse_period_us */
	write_u32_le(&payload[16], 30000);     /* train_on_duration_ms */
	write_u32_le(&payload[20], 60000);     /* train_off_duration_ms */

	uint8_t cmd_buf[64];
	uint16_t cmd_len = build_cmd_request(cmd_buf, OP_SET_BIPHASIC_PARAMETERS, payload, BIPHASIC_PAYLOAD_LEN);

	app_cmd_resp_t* p_resp = app_cmd_parse_request(cmd_buf, cmd_len);

	uint16_t exp_crc16 = crc16_compute(p_resp->cmd_resp, p_resp->cmd_resp_len - CRC_LEN, NULL);
	uint16_t act_crc16 = *((uint16_t*)&p_resp->cmd_resp[p_resp->cmd_resp_len - CRC_LEN]);
	uint8_t exp_status = STATUS_INVALID;
	uint8_t act_status = p_resp->cmd_resp[2];

	TEST_ASSERT_EQUAL_UINT16(exp_crc16, act_crc16);
	TEST_ASSERT_EQUAL_UINT8(exp_status, act_status);
	nrf_delay_ms(500);
}

/**
 * @brief Test that pulse_period smaller than (cathodic + anodic + interphase_gap) returns STATUS_INVALID
 * 
 */
static void test_biphasic_set_parameters_period_too_small(void)
{
	uint8_t payload[BIPHASIC_PAYLOAD_LEN];
	write_u32_le(&payload[0],  500);       /* cathodic_width_us */
	write_u32_le(&payload[4],  500);       /* anodic_width_us */
	write_u32_le(&payload[8],  100);       /* interphase_gap_us */
	write_u32_le(&payload[12], 800);       /* pulse_period_us (< 500+500+100=1100, invalid) */
	write_u32_le(&payload[16], 30000);     /* train_on_duration_ms */
	write_u32_le(&payload[20], 60000);     /* train_off_duration_ms */

	uint8_t cmd_buf[64];
	uint16_t cmd_len = build_cmd_request(cmd_buf, OP_SET_BIPHASIC_PARAMETERS, payload, BIPHASIC_PAYLOAD_LEN);

	app_cmd_resp_t* p_resp = app_cmd_parse_request(cmd_buf, cmd_len);

	uint16_t exp_crc16 = crc16_compute(p_resp->cmd_resp, p_resp->cmd_resp_len - CRC_LEN, NULL);
	uint16_t act_crc16 = *((uint16_t*)&p_resp->cmd_resp[p_resp->cmd_resp_len - CRC_LEN]);
	uint8_t exp_status = STATUS_INVALID;
	uint8_t act_status = p_resp->cmd_resp[2];

	TEST_ASSERT_EQUAL_UINT16(exp_crc16, act_crc16);
	TEST_ASSERT_EQUAL_UINT8(exp_status, act_status);
	nrf_delay_ms(500);
}

/**
 * @brief Test that GET_BIPHASIC_PARAMETERS returns the previously set parameters in the response payload
 * 
 */
static void test_biphasic_get_parameters(void)
{
	/* First, set known parameters */
	uint8_t set_payload[BIPHASIC_PAYLOAD_LEN];
	write_u32_le(&set_payload[0],  500);       /* cathodic_width_us */
	write_u32_le(&set_payload[4],  500);       /* anodic_width_us */
	write_u32_le(&set_payload[8],  100);       /* interphase_gap_us */
	write_u32_le(&set_payload[12], 20000);     /* pulse_period_us */
	write_u32_le(&set_payload[16], 30000);     /* train_on_duration_ms */
	write_u32_le(&set_payload[20], 60000);     /* train_off_duration_ms */

	uint8_t set_cmd[64];
	uint16_t set_cmd_len = build_cmd_request(set_cmd, OP_SET_BIPHASIC_PARAMETERS, set_payload, BIPHASIC_PAYLOAD_LEN);
	app_cmd_parse_request(set_cmd, set_cmd_len);

	/* Now, get parameters and verify they match */
	uint8_t get_cmd[64];
	uint16_t get_cmd_len = build_cmd_request(get_cmd, OP_GET_BIPHASIC_PARAMETERS, NULL, 0);
	app_cmd_resp_t* p_resp = app_cmd_parse_request(get_cmd, get_cmd_len);

	uint16_t exp_crc16 = crc16_compute(p_resp->cmd_resp, p_resp->cmd_resp_len - CRC_LEN, NULL);
	uint16_t act_crc16 = *((uint16_t*)&p_resp->cmd_resp[p_resp->cmd_resp_len - CRC_LEN]);
	uint8_t exp_status = STATUS_SUCCESS;
	uint8_t act_status = p_resp->cmd_resp[2];

	TEST_ASSERT_EQUAL_UINT16(exp_crc16, act_crc16);
	TEST_ASSERT_EQUAL_UINT8(exp_status, act_status);

	/* Verify the response payload contains the biphasic parameters */
	uint8_t exp_resp_payload_len = BIPHASIC_PAYLOAD_LEN;
	uint8_t act_resp_payload_len = p_resp->cmd_resp[1];
	TEST_ASSERT_EQUAL_UINT8(exp_resp_payload_len, act_resp_payload_len);

	TEST_ASSERT_EQUAL_UINT8_ARRAY(set_payload, &p_resp->cmd_resp[RESP_HEADER_LEN], BIPHASIC_PAYLOAD_LEN);
	nrf_delay_ms(500);
}

/**
 * @brief Test that charge imbalance (anodic width != cathodic width) includes a warning status flag
 * 
 */
static void test_biphasic_charge_balance_warning(void)
{
	uint8_t payload[BIPHASIC_PAYLOAD_LEN];
	write_u32_le(&payload[0],  500);       /* cathodic_width_us */
	write_u32_le(&payload[4],  300);       /* anodic_width_us (mismatch => charge imbalance) */
	write_u32_le(&payload[8],  100);       /* interphase_gap_us */
	write_u32_le(&payload[12], 20000);     /* pulse_period_us */
	write_u32_le(&payload[16], 30000);     /* train_on_duration_ms */
	write_u32_le(&payload[20], 60000);     /* train_off_duration_ms */

	uint8_t cmd_buf[64];
	uint16_t cmd_len = build_cmd_request(cmd_buf, OP_SET_BIPHASIC_PARAMETERS, payload, BIPHASIC_PAYLOAD_LEN);

	app_cmd_resp_t* p_resp = app_cmd_parse_request(cmd_buf, cmd_len);

	uint16_t exp_crc16 = crc16_compute(p_resp->cmd_resp, p_resp->cmd_resp_len - CRC_LEN, NULL);
	uint16_t act_crc16 = *((uint16_t*)&p_resp->cmd_resp[p_resp->cmd_resp_len - CRC_LEN]);
	uint8_t act_status = p_resp->cmd_resp[2];

	TEST_ASSERT_EQUAL_UINT16(exp_crc16, act_crc16);
	TEST_ASSERT_EQUAL_UINT8(STATUS_CHARGE_IMBALANCE_WARNING, act_status);
	nrf_delay_ms(500);
}

/**
 * @brief Test that the response CRC is correctly calculated for the biphasic parameter response
 * 
 */
static void test_biphasic_set_parameters_crc_check(void)
{
	uint8_t payload[BIPHASIC_PAYLOAD_LEN];
	write_u32_le(&payload[0],  250);       /* cathodic_width_us */
	write_u32_le(&payload[4],  250);       /* anodic_width_us */
	write_u32_le(&payload[8],  50);        /* interphase_gap_us */
	write_u32_le(&payload[12], 10000);     /* pulse_period_us */
	write_u32_le(&payload[16], 15000);     /* train_on_duration_ms */
	write_u32_le(&payload[20], 30000);     /* train_off_duration_ms */

	uint8_t cmd_buf[64];
	uint16_t cmd_len = build_cmd_request(cmd_buf, OP_SET_BIPHASIC_PARAMETERS, payload, BIPHASIC_PAYLOAD_LEN);

	app_cmd_resp_t* p_resp = app_cmd_parse_request(cmd_buf, cmd_len);

	/* Recompute CRC over the response (excluding the CRC bytes) */
	uint16_t computed_crc = crc16_compute(p_resp->cmd_resp, p_resp->cmd_resp_len - CRC_LEN, NULL);
	uint16_t embedded_crc;
	memcpy((uint8_t*)&embedded_crc, &p_resp->cmd_resp[p_resp->cmd_resp_len - CRC_LEN], CRC_LEN);

	TEST_ASSERT_EQUAL_UINT16(computed_crc, embedded_crc);

	/* Verify the response length is consistent */
	uint8_t exp_len = p_resp->cmd_resp_len;
	uint8_t act_len = p_resp->cmd_resp[1] + RESP_HEADER_LEN + CRC_LEN;
	TEST_ASSERT_EQUAL_UINT8(exp_len, act_len);
	nrf_delay_ms(500);
}

/**
 * @brief Run all test items for biphasic stimulation waveform
 * 
 */
void test_run_app_biphasic(void)
{
	UnityPrint("***********************");
	UNITY_PRINT_EOL();

	UnityBegin("[BIPHASIC]");
	RUN_TEST(test_biphasic_set_parameters_valid, __LINE__);
	RUN_TEST(test_biphasic_set_parameters_invalid_payload_length, __LINE__);
	RUN_TEST(test_biphasic_set_parameters_zero_cathodic_width, __LINE__);
	RUN_TEST(test_biphasic_set_parameters_period_too_small, __LINE__);
	RUN_TEST(test_biphasic_get_parameters, __LINE__);
	RUN_TEST(test_biphasic_charge_balance_warning, __LINE__);
	RUN_TEST(test_biphasic_set_parameters_crc_check, __LINE__);
	UNITY_END();
	nrf_delay_ms(500);
}
