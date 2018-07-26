#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"

#include "driver/uart.h"

#include "lwipopts.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/mem.h"

#if LWIP_TCP

static struct tcp_pcb *echo_pcb;


err_t adc_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
void adc_error(void *arg, err_t err);
err_t adc_poll(void *arg, struct tcp_pcb *tpcb);
void adc_close(struct tcp_pcb *tpcb, void *es);

void
adc_init(void)
{
	echo_pcb = tcp_new();
	if (echo_pcb != NULL) {
		err_t err;

		err = tcp_bind(echo_pcb, IP_ADDR_ANY, 7);
		if (err == ERR_OK) {
			echo_pcb = tcp_listen(echo_pcb);
			tcp_accept(echo_pcb, adc_accept);
		}else {
			/* abort? output diagnostic? */
		}
	}else {
		/* abort? output diagnostic? */
	}
}


err_t
adc_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	err_t ret_err;
	struct echo_state *es;

	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(err);

	//tcp_arg(newpcb, es);
	//tcp_recv(newpcb, echo_recv);
	tcp_err(newpcb, adc_error);
	tcp_poll(newpcb, adc_poll, 0);

	/* commonly observed practive to call tcp_setprio(), why? */
	tcp_setprio(newpcb, TCP_PRIO_MIN);
	return ERR_OK;
}

void
adc_error(void *arg, err_t err)
{
	LWIP_UNUSED_ARG(err);
}

err_t
adc_poll(void *arg, struct tcp_pcb *tpcb)
{
	char buffer[15];

	uint16 adc_value =  system_adc_read();

	os_sprintf(buffer, "Adc: %d \r\n", adc_value);
	tcp_write(tpcb, buffer, strlen(buffer), TCP_WRITE_FLAG_COPY);
	return ERR_OK;
}



void
adc_close(struct tcp_pcb *tpcb, void *es)
{
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);

	tcp_close(tpcb);
}

#endif /* LWIP_TCP */
