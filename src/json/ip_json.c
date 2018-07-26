#include "lwipopts.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"

#include "user_interface.h"

#include "artnet/artnet.h"
#include "config.h"
#include "json_state.h"

static const char* ip_get_json(void* user);

json_state_t ip_json_state = {
  .get_json = ip_get_json,
  .set_json = NULL,
  .user = NULL,
};

static const char *ConnectionStatusLut[] = {
	"idle",
	"connecting",
	"wrong password",
	"no ap found",
	"connection fail",
	"got ip",
};

static const char* ICACHE_FLASH_ATTR
ip_get_json(void* user)
{
  struct ip_info info;
  wifi_get_ip_info(STATION_IF, &info);

	static char buffer[100];
  os_sprintf(buffer, "{"
       "\"status\" : \"%s\","
       "\"ip\" : \""IPSTR "\","
       "\"gw\" : \""IPSTR "\","
       "\"netmask\" : \""IPSTR "\""
       "}"
       , ConnectionStatusLut[wifi_station_get_connect_status()]
       , IP2STR(&(info.ip.addr))
       , IP2STR(&(info.gw.addr))
       , IP2STR(&(info.netmask.addr))
       );
  os_printf("%s\n",buffer);
	return &(buffer[0]);
}
