#include "lwip/apps/mqtt.h"
#include "mqtt/mqtt.h"
#include "stdio.h"

#include "driver/uart.h"

#include "http11/http11_parser.h"
#include "http_server.h"
#include "json_state.h"
#include "http_path_lookup.h"

extern json_state_t ip_json_state;

static mqtt_client_t * mqtt_client;

static MQTT_config* config;

void static mqtt_connect(void);

static void mqtt_request_cb(void *arg, err_t err)
{

}

void dmxData(const uint8_t* data, uint16_t dmxLen);
static json_state_t * json_state;

typedef enum {
	 json,
	 binary,
 } incomming_data_type_t;

static incomming_data_type_t incomming_data_type = json;
static uint8_t mqtt_incomming_buffer[512];
static uint16_t mqtt_incomming_buffer_position;


static void ICACHE_FLASH_ATTR
mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
	os_printf("incoming data %d\n",len);

	switch (incomming_data_type) {
		case json:
		{
			u16_t i;
			for(i = 0; i < len; i++)
				uart_tx_one_char(UART1,data[i]);
			os_printf("\n");
			os_printf("Json user data.\n");
			json_state->set_json(data, len, json_state->user);
			break;
		}
		case binary:
			os_printf("Binary user data.\n");
			memcpy(&(mqtt_incomming_buffer[mqtt_incomming_buffer_position]), data, len);
			mqtt_incomming_buffer_position += len;
			if( MQTT_DATA_FLAG_LAST == flags)
				dmxData(mqtt_incomming_buffer, mqtt_incomming_buffer_position);
			break;
		default:
			break;
	}

}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
	os_printf("incoming publish %d topic %s\n",tot_len,topic);
	uint32_t slashcounter = 0;
	while(*topic != '\0')
	{
		if(*topic == '/')
		  slashcounter++;

		if(slashcounter == 2)
			break;
		topic++;
	}
	os_printf("Subtopic %s\n",topic);

	if(0 == strcmp(topic,"/framebuffer")) {
		incomming_data_type = binary;
		mqtt_incomming_buffer_position = 0;
	}
	else {
		incomming_data_type = json;
		const struct HttpPathLookup * lookup = http_path_lookup(topic,strlen(topic));
		json_state = (json_state_t *)lookup->requestCbData;
	}
}

err_t mqtt_subscribe_base_topic(char* subTopic)
{
	char topic[100];
	os_sprintf(topic, "%s/%s/%s",
		config->base_topic,
		config->conn_id,
	  subTopic);

	os_printf("%s\n",topic);

	err_t err = mqtt_subscribe(mqtt_client,topic, 0, mqtt_request_cb, NULL);
	if(err != ERR_OK)
		os_printf("Error %d\n",err);
	return err;
}

err_t mqtt_subscribe_broadcast_topic(char* subTopic)
{
	char topic[100];
	os_sprintf(topic, "%s/broadcast/%s",
		config->base_topic,
	  subTopic);

	os_printf("%s\n",topic);

	err_t err = mqtt_subscribe(mqtt_client,topic, 0, mqtt_request_cb, NULL);
	if(err != ERR_OK)
		os_printf("Error %d\n",err);
	return err;
}

static void ICACHE_FLASH_ATTR
mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
	err_t err;
	char buffer[100];

	switch (status) {
	case MQTT_CONNECT_ACCEPTED:
		// snprintf(buffer, sizeof(buffer),"blup %s", "bla");
		err = mqtt_publish(
			mqtt_client,
			config->will_topic,
			"online",
			6,      // strlen("online")
			0,      //qos
			1,
			mqtt_request_cb,
			NULL);
		mqtt_set_inpub_callback(
			mqtt_client,
			mqtt_incoming_publish_cb,
		  mqtt_incoming_data_cb,
			NULL);

		char topic[100];
		os_sprintf(topic, "%s/%s/ip",
			config->base_topic,
			config->conn_id);

		const char *json_string = ip_json_state.get_json(NULL);

		err = mqtt_publish(
			mqtt_client,
			topic,
			json_string,
			strlen(json_string),
			0,      //qos
			1,
			mqtt_request_cb,
			NULL);

		err = mqtt_subscribe_base_topic("framebuffer");
		err = mqtt_subscribe_broadcast_topic("framebuffer");
	  err = mqtt_subscribe_base_topic("artnet");
	  err = mqtt_subscribe_broadcast_topic("artnet");
	  err = mqtt_subscribe_base_topic("cmd");
	  err = mqtt_subscribe_broadcast_topic("cmd");
		break;

	case MQTT_CONNECT_DISCONNECTED:
	case MQTT_CONNECT_TIMEOUT:
		mqtt_connect();
		break;

	default:
		break;
	}

}

static void ICACHE_FLASH_ATTR
mqtt_dns_lookup_cb(const char *name, struct ip_addr *ipaddr, void *arg)
{
	struct mqtt_connect_client_info_t mqtt_connect_client_info = {
		.client_id	= config->conn_id,
		.client_user	= config->user,
		.client_pass	= config->password,
		.keep_alive	= 60,
		.will_topic	= config->will_topic,
		.will_msg	= "offline",
		.will_qos	= 0,
		.will_retain	= 1,
	};

	err_t err;

	if ((ipaddr) && (ipaddr->addr)) {
		err = mqtt_client_connect(
			mqtt_client,
			ipaddr,
			MQTT_PORT,
			mqtt_connection_cb,
			NULL,
			&mqtt_connect_client_info);
	}else{

	}
	//      smtp.lastError = SMTP_UNKNOWN_HOST;
	// smtp.state = SMTP_IDLE;
}

void static mqtt_connect(void)
{
	err_t err;

	struct ip_addr host;

	// IP4_ADDR(&host, 192, 168, 123, 62);

	err = dns_gethostbyname(
		config->uri,
		&host,
		mqtt_dns_lookup_cb,
		mqtt_client);

	switch (err) {
	case ERR_OK:
		// numeric or cached, returned in resolved
		mqtt_dns_lookup_cb(config->uri, &host, mqtt_client);
		break;
	case ERR_INPROGRESS:
		// need to ask, will return data via callback
		break;
	default:
		// bad arguments in function call
		break;
	}
}

void mqtt_init(MQTT_config *new_config)
{
	mqtt_client = mqtt_client_new();
	config = new_config;

	mqtt_connect();
}

//
// // #include "json_state.h"
// extern json_state_t temperature_json_state;
//
// void ICACHE_FLASH_ATTR
// mqtt_temperature(const char* message, void* user)
// {
// 	if(mqtt_client == NULL)
// 		return;
//
// 		char topic[100];
// 		os_sprintf(topic, "%s/%s/temperature",
// 			config->base_topic,
// 			config->conn_id);
//
// 	message = temperature_json_state.get_json(NULL);
//
// 	if(mqtt_client_is_connected(mqtt_client) != 0)
// 	{
// 		err_t err;
// 		err = mqtt_publish(
// 			mqtt_client,
// 			topic,
// 			message,
// 			strlen(message),
// 			0,
// 			1,
// 			mqtt_request_cb,
// 			NULL);
// 	}
// }
