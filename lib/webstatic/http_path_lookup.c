/* C code produced by gperf version 3.0.4 */
/* Command-line: gperf -L C user/http_path_lookup.gperf  */
/* Computed positions: -k'' */

#line 1 "user/http_path_lookup.gperf"

#include <stdint.h>
#include "http_server.h"
#include "http_path_lookup.h"
#include "static_web_files.h"
#include "json_state.h"

void transmit_wifi_connect(HttpReqeuest* httpReqeuest, const char *at, size_t length);
void transmit_wifi_reset(HttpReqeuest* httpReqeuest, const char *at, size_t length);
void transmit_wifi_status(HttpReqeuest* httpReqeuest, const char *at, size_t length);
void transmit_wifi_ipInfo(HttpReqeuest* httpReqeuest, const char *at, size_t length);
void http_wifi_aplist(HttpReqeuest* httpReqeuest, const char *at, size_t length);

void transmit_pwm(HttpReqeuest* httpReqeuest, const char *at, size_t length);
void transmit_adc(HttpReqeuest* httpReqeuest, const char *at, size_t length);

void http_artnet(HttpReqeuest* httpReqeuest, const char *at, size_t length);


void transmit_json(HttpReqeuest* httpReqeuest, const char *at, size_t length);
extern json_state_t artnet_json_state;
extern json_state_t board_json_state;
extern json_state_t ip_json_state;
extern json_state_t wifi_json_state;
extern json_state_t cube_cmd_json_state;

extern HttpFile fileIndexHtml;
extern HttpFile fileArtnetHtml;
void transmit_file(HttpReqeuest* httpReqeuest, const char *at, size_t length);

#line 41 "user/http_path_lookup.gperf"
struct HttpPathLookup;
#include <string.h>

#define TOTAL_KEYWORDS 9
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 17
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 17
/* maximum key range = 17, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
/*ARGSUSED*/
static unsigned int
hash (str, len)
     register const char *str;
     register unsigned int len;
{
  return len;
}

static const struct HttpPathLookup wordlist[] =
  {
#line 43 "user/http_path_lookup.gperf"
    {"/", transmit_file, (void*) &fileIndexHtml},
#line 50 "user/http_path_lookup.gperf"
    {"/ip", transmit_json, &ip_json_state},
#line 46 "user/http_path_lookup.gperf"
    {"/cmd", transmit_json, &cube_cmd_json_state},
#line 51 "user/http_path_lookup.gperf"
    {"/wifi", transmit_json, &wifi_json_state},
#line 49 "user/http_path_lookup.gperf"
    {"/board", transmit_json, &board_json_state},
#line 47 "user/http_path_lookup.gperf"
    {"/artnet", transmit_json, &artnet_json_state},
#line 44 "user/http_path_lookup.gperf"
    {"/index.html", transmit_file, (void*) &fileIndexHtml},
#line 48 "user/http_path_lookup.gperf"
    {"/artnet.html", transmit_file, (void*) &fileArtnetHtml},
#line 45 "user/http_path_lookup.gperf"
    {"/wifi/connect.cgi", transmit_json, &wifi_json_state}
  };

#ifdef __GNUC__
__inline
#if defined __GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct HttpPathLookup *
http_path_lookup (str, len)
     register const char *str;
     register unsigned int len;
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= MIN_HASH_VALUE)
        {
          register const struct HttpPathLookup *resword;

          switch (key - 1)
            {
              case 0:
                resword = &wordlist[0];
                goto compare;
              case 2:
                resword = &wordlist[1];
                goto compare;
              case 3:
                resword = &wordlist[2];
                goto compare;
              case 4:
                resword = &wordlist[3];
                goto compare;
              case 5:
                resword = &wordlist[4];
                goto compare;
              case 6:
                resword = &wordlist[5];
                goto compare;
              case 10:
                resword = &wordlist[6];
                goto compare;
              case 11:
                resword = &wordlist[7];
                goto compare;
              case 16:
                resword = &wordlist[8];
                goto compare;
            }
          return 0;
        compare:
          {
            register const char *s = resword->path;

            if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
              return resword;
          }
        }
    }
  return 0;
}
