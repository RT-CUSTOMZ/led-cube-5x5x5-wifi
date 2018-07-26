#include "user_interface.h"
#include "artnet/artnet.h"
#include "config.h"
#include "json_state.h"

#include "jsmn.h"

#define max(a,b) \
	({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a > _b ? _a : _b; })

#define min(a,b) \
	({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a < _b ? _a : _b; })

static const char* get_json(void* user);
static bool set_json(const char* json, uint32_t len, void* user);

json_state_t cube_cmd_json_state = {
  .get_json = NULL,
  .set_json = set_json,
  .user = NULL,
};

#define CMD_NEXT_EFFECT 0x0001
#define CMD_PREV_EFFECT 0x0002
#define CMD_NEXT_MODULE 0x0003
#define CMD_PREV_MODULE 0x0004
#define CMD_RECORD_ON   0x0101
#define CMD_RECORD_OFF  0x0102
void cube_cmd_next_effect(uint16_t cmd_number);

static uint8_t cmd_buffer[20];

static bool ICACHE_FLASH_ATTR
set_json(const char* json, uint32_t len, void* user)
{
	jsmn_parser p;
	jsmntok_t token[10];

	jsmn_init(&p);

	uint32_t jsmn_result = jsmn_parse(
		&p, 
		json, 
		len, 
		(jsmntok_t*) &token, 
		sizeof(token)/sizeof(token[0]
		));

	if(jsmn_result < 0)
		return false;

	for(uint32_t i = 0; i < jsmn_result; i++) {
		if( (JSMN_PRIMITIVE == token[i].type) || (JSMN_STRING == token[i].type) ) {
			if( 0 == strncmp("cmd", json + token[i].start, token[i].end - token[i].start)) {
			i++;
				if(! (i < jsmn_result) )
					return false;
				if(JSMN_STRING == token[i].type) {
					if( 0 == strncmp("next_effect", json + token[i].start, token[i].end - token[i].start)) {
						cube_cmd_next_effect(CMD_NEXT_EFFECT);
									os_printf("next effect\n");
					}
					else if( 0 == strncmp("next_module", json + token[i].start, token[i].end - token[i].start)) {
						cube_cmd_next_effect(CMD_NEXT_MODULE);
						os_printf("next module\n");
					}
					else if( 0 == strncmp("record_on", json + token[i].start, token[i].end - token[i].start)) {
						cube_cmd_next_effect(CMD_RECORD_ON);
						os_printf("record on\n");
					}
					else if( 0 == strncmp("record_off", json + token[i].start, token[i].end - token[i].start)) {
						cube_cmd_next_effect(CMD_RECORD_OFF);
						os_printf("record off\n");
					}
					else{
						os_printf("Fehler: cmd unbekannt!\n");
					}
				}
			}
		}
	}

	return true;
}
