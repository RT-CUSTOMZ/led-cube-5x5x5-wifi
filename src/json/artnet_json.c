#include "user_interface.h"

#include "artnet/artnet.h"
#include "config.h"
#include <stdint.h>
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

static const char* artnet_get_json(void* user);
static bool artnet_set_json(const char* json, uint32_t len, void* user);

json_state_t artnet_json_state = {
  .get_json = artnet_get_json,
  .set_json = artnet_set_json,
  .user = NULL,
};

static const char* ICACHE_FLASH_ATTR
artnet_get_json(void* user)
{
	ARTNET_config * config =  ARTNET_get_config();

	static char buffer[100];
	os_sprintf(buffer,
    "{\"enabled\": %s,\"nodeName\":\"%s\",\"universe\" : %d,\"offset\" : %d}",
    config->enabled ? "true" : "false",
		config->nodeName, config->universe + 1, config->offset);
  os_printf("%s\n",buffer);
	return &(buffer[0]);
}

static bool ICACHE_FLASH_ATTR
artnet_set_json(const char* json, uint32_t len, void* user)
{
  ARTNET_config* config =  ARTNET_get_config();

  bool persistent = false;

	jsmn_parser p;
	jsmntok_t token[20];

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

  os_printf("Json: %s\n", json);
  os_printf("Json count: %d\n", jsmn_result);

	for(uint32_t i = 0; i < jsmn_result; i++) {
    if( (JSMN_PRIMITIVE == token[i].type) || (JSMN_STRING == token[i].type) ) {
      if( 0 == strncmp("nodeName", json + token[i].start, token[i].end - token[i].start)) {
        i++;
        if(! (i < jsmn_result) )
          return false;
        if(JSMN_STRING == token[i].type) {
					static char nodeNameBuffer[40];
          uint32_t len = min(
            sizeof(nodeNameBuffer), 
            token[i].end - token[i].start
            );
          strncpy(
            (char*)       nodeNameBuffer,
            (const char*) json + token[i].start,
            len
            );
						config->nodeName = nodeNameBuffer;
            os_printf("%s\n",config->nodeName);
        }
      }
			else if( 0 == strncmp("universe", json + token[i].start, token[i].end - token[i].start)) {
        i++;
        if(! (i < jsmn_result) )
          return false;
        if(JSMN_PRIMITIVE == token[i].type) {
					char buffer[10];
          uint32_t len = min(
            sizeof(buffer) -1, 
            token[i].end - token[i].start
            );
          strncpy(
            (char*)       buffer,
            (const char*) json + token[i].start,
            len
            );
          buffer[len] = '\0';
						config->universe = atoi(buffer);
        }
      }
			else if( 0 == strncmp("offset", json + token[i].start, token[i].end - token[i].start)) {
        i++;
        if(! (i < jsmn_result) )
          return false;
        if(JSMN_PRIMITIVE == token[i].type) {
					char buffer[10];
          uint32_t len = min(
            sizeof(buffer) -1, 
            token[i].end - token[i].start
            );
          strncpy(
            (char*)       buffer,
            (const char*) json + token[i].start,
            len
            );
          buffer[len] = '\0';
					config->offset = atoi(buffer);
        }
      }
			else if( 0 == strncmp("enabled", json + token[i].start, token[i].end - token[i].start)) {
        i++;
        if(! (i < jsmn_result) )
          return false;
        if(JSMN_PRIMITIVE == token[i].type) {
					uint32_t len = token[i].end - token[i].start;
					config->enabled = 
						(0 == strncmp(
							(const char*) json + token[i].start,
							"true",
							len)
						);
        }
      }
			else if( 0 == strncmp("persistent", json + token[i].start, token[i].end - token[i].start)) {
        i++;
        if(! (i < jsmn_result) )
          return false;
        if(JSMN_PRIMITIVE == token[i].type) {
					uint32_t len = token[i].end - token[i].start;
					persistent = 
						(0 == strncmp(
							(const char*) json + token[i].start,
							"true",
							len)
						);
        }
      }
    }
  }

  if(persistent)
	 config_save();
	return true;
}
