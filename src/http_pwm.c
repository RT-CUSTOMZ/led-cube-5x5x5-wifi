#include "lwipopts.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"

// #include "driver/uart.h"
// #include "ets_sys.h"
// #include "osapi.h"
#include "user_interface.h"
#include "json/jsonparse.h"

#include "http11/http11_parser.h"
#include "http_server.h"

// #include "mine_type_lookup.h"

union Color {
	struct {
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};
	uint8_t raw[3];
};

void ws2811dma_put(uint8_t *buffer, uint16_t channels);
void ws2811dma_put_offset(uint8_t *buffer, uint16_t channels, uint16_t offset);

static union Color color = { 0, 0, 0 };

#define CRLF "\r\n"

static const char http_response_pre_string[] = {
	"HTTP/1.1 %d"CRLF
//		"Server: nginx/1.9.4"CRLF
//		"Date: Fri, 04 Dec 2015 18:18:38 GMT"CRLF
	// "Content-Type: %s"CRLF
	"Access-Control-Allow-Origin: *"CRLF
	"Access-Control-Allow-Methods: GET, PUT"CRLF
	"Content-Length: 0"CRLF
	"Connection: close"CRLF
	//"Connection: keep-alive"CRLF
	CRLF
};

static int ICACHE_FLASH_ATTR
send_head_response(HttpReqeuest* httpReqeuest, int responseCode)
{
	uint8_t buffer[300];

	os_sprintf(buffer, http_response_pre_string, responseCode
	           // , mineTyp, length
		   );
	tcp_write(httpReqeuest->pcb, buffer, strlen(buffer), TCP_WRITE_FLAG_COPY);
}

void user_pwm_set(uint8_t r, uint8_t g, uint8_t b);

void ICACHE_FLASH_ATTR
transmit_pwm(HttpReqeuest* httpReqeuest, const char *at, size_t length)
{
	switch (httpReqeuest->requestMethod) {
	case GET:
	{
		httpReqeuest->requestCb = NULL;
		send_response(httpReqeuest, 404, "text/html", 0);
		break;
	}
	case PUT:
	{
		struct jsonparse_state state;
		jsonparse_setup(&state, at, length);

		if (jsonparse_next(&state) != '{')
			goto error;
		int jsonType = jsonparse_next(&state);
		while ( jsonType != JSON_TYPE_ERROR) {
			if (jsonType == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&state, "r") == 0) {
					if (jsonparse_next(&state) != JSON_TYPE_PAIR)
						goto error;
					if (jsonparse_next(&state) != JSON_TYPE_NUMBER)
						goto error;
					color.r = jsonparse_get_value_as_int(&state);
				}else if (jsonparse_strcmp_value(&state, "g") == 0) {
					if (jsonparse_next(&state) != JSON_TYPE_PAIR)
						goto error;
					if (jsonparse_next(&state) != JSON_TYPE_NUMBER)
						goto error;
					color.g = jsonparse_get_value_as_int(&state);
				} else if (jsonparse_strcmp_value(&state, "b") == 0) {
					if (jsonparse_next(&state) != JSON_TYPE_PAIR)
						goto error;
					if (jsonparse_next(&state) != JSON_TYPE_NUMBER)
						goto error;
					color.b = jsonparse_get_value_as_int(&state);
				}
			}
			jsonType = jsonparse_next(&state);
		}

		os_printf("Color r: %d g: %d b: %d\n",
			  color.r, color.g, color.b);
		uint8_t temp = color.g;
		color.g = color.r;
		color.r = temp;
		user_pwm_set(color.r, color.g, color.b);
		uint8_t i = 0;
		for ( i = 0; i <= 30; i++)
			ws2811dma_put_offset((uint8_t*)&(color.raw), 3, i * 3);

error:
		httpReqeuest->requestCb = NULL;
		send_response(httpReqeuest, 200, "", 0);

		break;
	}
	case HEAD:
	case OPTIONS:
	{
		httpReqeuest->requestCb = NULL;
		send_head_response(httpReqeuest, 200);
		break;
	}
	}

}
