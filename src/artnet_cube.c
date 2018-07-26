/*
 * artnet_cube.c
 *
 *  Created on: Feb 3, 2015
 *      Author: esp8266
 */

#include "artnet/artnet.h"
#include "driver/uart.h"
#include "osapi.h"
#include "string.h"

#include "crc16ibm.h"


static void print_string(const char* message)
{
	uart0_tx_buffer((uint8*)message, strlen(message));
}

char buffer[97];

#define COMMAND_FRAME 0x0001
#define COMMAND_CMD 0x0002
typedef struct Uart_Header_s {
	uint16_t command;
	uint16_t len;
} Uart_header_t;

#define CMD_NEXT_EFFECT 0x0001
#define CMD_PREV_EFFECT 0x0002
#define CMD_NEXT_MODULE 0x0003
#define CMD_PREV_MODULE 0x0004
typedef struct Uart_Cmd_s {
	uint16_t cmd;
} Uart_cmd_t;


void ICACHE_FLASH_ATTR
cube_cmd_next_effect(uint16_t cmd_number)
{
	uint16_t crc = crc16ibm_init();
	int i;

	const static uint8_t SYNC_SEQUENZ[] = {
		'C', 'U', 'B', 'E', 'S', 'Y', 'N', 'C',
	};
	uart0_tx_buffer((uint8*)SYNC_SEQUENZ, sizeof(SYNC_SEQUENZ));

	const static Uart_header_t header = {
		.command = COMMAND_CMD,
		.len = sizeof(Uart_cmd_t),
	};
	uart0_tx_buffer((uint8*)&header, sizeof(header));
	for(i = 0 ; i < sizeof(header); i++)
		crc = crc16ibm_addValue(crc, ((uint8_t *)&header)[i]);

	Uart_cmd_t cmd = {
		.cmd = cmd_number,
	};
	uart0_tx_buffer((uint8*)&cmd, sizeof(cmd));
	for(i = 0 ; i < sizeof(cmd); i++)
		crc = crc16ibm_addValue(crc, ((uint8_t *)&cmd)[i]);

	uart0_tx_buffer((uint8*)&crc, 2);
}

void ICACHE_FLASH_ATTR
dmxData(const uint8_t* data, uint16_t dmxLen)
{
	if(dmxLen < 375)
	{
		os_printf("Fehler, Artnet frame empfangen mit %d bytes.\n", dmxLen);
		return;
	}

	uint16_t crc = crc16ibm_init();
	int i;

	const static uint8_t SYNC_SEQUENZ[] = {
		'C', 'U', 'B', 'E', 'S', 'Y', 'N', 'C',
	};
	uart0_tx_buffer((uint8*)SYNC_SEQUENZ, sizeof(SYNC_SEQUENZ));
	const static Uart_header_t header = {
		.command = COMMAND_FRAME,
		.len = 375,
	};
	uart0_tx_buffer((uint8*)&header, sizeof(header));
	uart0_tx_buffer((uint8*)data, dmxLen);

	for(i = 0 ; i < sizeof(SYNC_SEQUENZ); i++)
		crc = crc16ibm_addValue(crc, SYNC_SEQUENZ[i]);
	for(i = 0 ; i < sizeof(header); i++)
		crc = crc16ibm_addValue(crc, ((uint8_t *)&header)[i]);
	for(i = 0 ; i < 375; i++)
		crc = crc16ibm_addValue(crc, ((uint8_t *)data)[i]);

	uart0_tx_buffer((uint8*)&crc, 2);

	os_printf("Artnet frame empfangen mit %d bytes.\n", dmxLen);
}

static ARTNET_callbacks callbacks = {
	.dmxData	= dmxData,
};

void ICACHE_FLASH_ATTR
ARTNET_CUBE_init(void)
{
	//uart_init(BIT_RATE_460800, BIT_RATE_115200);
	const char* message = "\n" "effect disable\n" "framebuffer blank\n";

	uart0_tx_buffer((uint8*)message, strlen(message));

	ARTNET_config config = {
		.enabled = true,
		.nodeName = "LedCube",
		.universe = 42 -1,
		.offset = 0,
	};
	ARTNET_init(&callbacks,&config);
}
