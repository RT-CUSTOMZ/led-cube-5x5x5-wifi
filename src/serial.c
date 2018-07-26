/*
 * artnet_cube.c
 *
 *  Created on: Feb 3, 2015
 *      Author: esp8266
 */

#include "uart.h"
#include "osapi.h"
#include "string.h"


#include "gpio.h"

#include "serial.h"
// #include "driver/uart.h"
//////////////////////

// #include "driver/uart.h"
#include "osapi.h"
#include "driver/uart_register.h"
#include "mem.h"
#include "os_type.h"

#define min(a, b) \
	({ __typeof__(a)_a = (a); \
	   __typeof__(b)_b = (b); \
	   _a < _b ? _a : _b; })

static inline uint16_t TX_FIFO_LEN(uint32_t uart_no)
{
	return (READ_PERI_REG(UART_STATUS(uart_no)) >> UART_TXFIFO_CNT_S)
	       & UART_TXFIFO_CNT;
}

static inline uint16_t RX_FIFO_LEN(uint32_t uart_no)
{
	return (READ_PERI_REG(UART_STATUS(uart_no)) >> UART_RXFIFO_CNT_S)
	       & UART_RXFIFO_CNT;
}


static void ICACHE_FLASH_ATTR
uart_config(uint8 uart_no, uint32 baudrate)
{
	uart_div_modify(uart_no, UART_CLK_FREQ / baudrate);//SET BAUDRATE

	WRITE_PERI_REG(UART_CONF0(uart_no), 0
		       | ((STICK_PARITY_DIS & UART_PARITY_EN_M) <<  UART_PARITY_EN_S)//SET BIT AND PARITY MODE
		       | ((NONE_BITS & UART_PARITY_M) << UART_PARITY_S )
		       | ((ONE_STOP_BIT & UART_STOP_BIT_NUM) << UART_STOP_BIT_NUM_S)
		       | ((EIGHT_BITS & UART_BIT_NUM) << UART_BIT_NUM_S)
	               //  | UART_TXD_BRK                   //line stop signal, set line to low for reset;
		       );

	WRITE_PERI_REG(UART_CONF1(uart_no), 0
		       | ((00 << UART_TXFIFO_EMPTY_THRHD_S) << UART_TXFIFO_EMPTY_THRHD_S)
		       | ((100 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S)
	                //  | (0x7f & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S
	                //  | UART_RX_TOUT_EN
		       );

	//clear rx and tx fifo,not ready
	SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);    //RESET FIFO
	CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);


	//clear all interrupt
	WRITE_PERI_REG(UART_INT_CLR(uart_no), 0xffff);
	//enable interrupt

	WRITE_PERI_REG(UART_INT_ENA(uart_no), 0x0000);
	SET_PERI_REG_MASK(UART_INT_ENA(uart_no), 0
			  | UART_RXFIFO_FULL_INT_ENA
	                        // | UART_RXFIFO_OVF_INT_ENA
	                        // | UART_RXFIFO_TOUT_INT_ENA
			  );
}

uint32_t ICACHE_FLASH_ATTR
Serial_FillTxFifo(uint8_t uart_no, uint8_t* data, uint32_t len)
{
	if (len == 0) {
		CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_TXFIFO_EMPTY_INT_ENA);
		return 0;
	}

	uint32_t fifo_len = 128 - TX_FIFO_LEN(uart_no);

	len = min(len, fifo_len);
	fifo_len = len;

	for (; len > 0; len--) {
		WRITE_PERI_REG(UART_FIFO(uart_no), *data);
		data++;
	}
	return fifo_len;
}

uint32_t ICACHE_FLASH_ATTR
Serial_read(uint8_t len, uint8_t * dataPtr)
{
	uint32_t fifo_len =  RX_FIFO_LEN(UART0);

	len = min(len, fifo_len);
	fifo_len = len;

	for (; len > 0; len--) {
		*dataPtr = READ_PERI_REG(UART_FIFO(UART0));
		dataPtr++;
	}
	return fifo_len;
}

/////////////////////////////
enum eStatesTransmitter {
	eStates_idle,
	eStates_transmitting,
};

typedef struct {
	enum eStatesTransmitter state;
	TransmittCallback transmittCallback;
	ReceiveCallback receiveCallback;
	void * userptr;
	uint8_t * dataToSend;
	uint32_t dataToSendLen;
} Serial_t;

static Serial_t serial0 =
{
	.state			= eStates_idle,
	.transmittCallback	= NULL,
	.receiveCallback	= NULL,
	.userptr		= NULL,
	.dataToSend		= NULL,
	.dataToSendLen		= 0,
};

static Serial_t serial1 =
{
	.state			= eStates_idle,
	.transmittCallback	= NULL,
	.receiveCallback	= NULL,
	.userptr		= NULL,
	.dataToSend		= NULL,
	.dataToSendLen		= 0,
};

void uart_handler(uint8_t uart_no, Serial_t * serial){
	uint32_t uart_interrupt_status = READ_PERI_REG(UART_INT_ST(uart_no));

	if ( 0 != uart_interrupt_status) {
		//Tx Fifo
		if (uart_interrupt_status & UART_TXFIFO_EMPTY_INT_ST) {
			if (serial->dataToSendLen == 0) {
				serial->dataToSendLen = serial->transmittCallback(
					&(serial->dataToSend),
					serial->userptr);
			}
			if (serial->dataToSendLen == 0)
				CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_TXFIFO_EMPTY_INT_ENA);
			else {
				uint32_t len = Serial_FillTxFifo(uart_no,
								 serial->dataToSend,
								 serial->dataToSendLen);
				serial->dataToSend += len;
				serial->dataToSendLen -= len;
			}

			WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_TXFIFO_EMPTY_INT_CLR);
		}
		//Rx Fifo
		if (uart_interrupt_status & UART_RXFIFO_FULL_INT_ST) {
			if (serial->receiveCallback != NULL)
				serial->receiveCallback(serial->userptr);
			WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_RXFIFO_FULL_INT_CLR);
		}
		//Rx Timeout
		if (uart_interrupt_status & UART_RXFIFO_TOUT_INT_ST)
			WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_RXFIFO_TOUT_INT_CLR);
		//Rx Frame Error
		if (uart_interrupt_status & UART_FRM_ERR_INT_ST)
			WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_FRM_ERR_INT_CLR);
	}
	WRITE_PERI_REG(UART_INT_CLR(uart_no), 0xffff);  //catch all
}

static void
uart_interrupt_handler(void *para)
{
	uart_handler(UART0, &serial0);
	uart_handler(UART1, &serial1);
}
///////////////////////////////////////////////////////////

void ICACHE_FLASH_ATTR
Serial_transmittRequest(void)
{
	uint8_t uart_no = UART0;

	SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_TXFIFO_EMPTY_INT_ENA);
}

void ICACHE_FLASH_ATTR
Serial_init(TransmittCallback transmitCallback, ReceiveCallback receiveCallback, void * userptr)
{
	ETS_UART_INTR_DISABLE();

	////////////////////////
	serial0.transmittCallback = transmitCallback;
	serial0.receiveCallback = receiveCallback;
	serial0.userptr = userptr;
	serial0.dataToSendLen = 0;

/////////////////////////
	uart_config(UART0, BIT_RATE_230400);

	//interrupt anmelden

	ETS_UART_INTR_ATTACH(uart_interrupt_handler,  NULL);
	ETS_UART_INTR_ENABLE();
}

void ICACHE_FLASH_ATTR
Serial1_init(TransmittCallback transmitCallback, uint32_t baudrate, void * userptr)
{
	ETS_UART_INTR_DISABLE();

	////////////////////////
	serial1.transmittCallback = transmitCallback;
	serial1.receiveCallback = NULL;
	serial1.userptr = userptr;
	serial1.dataToSendLen = 0;

	/////////////////////////
	uart_config(UART1, baudrate);

	//interrupt anmelden
	ETS_UART_INTR_ATTACH(uart_interrupt_handler,  NULL);
	ETS_UART_INTR_ENABLE();
}

///////////////////////////////////////////////////////////

static void uart_tx_one_char(uint8 uart, uint8 TxChar)
{
	while (true) {
		uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(uart)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);
		if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126)
			break;
	}
	WRITE_PERI_REG(UART_FIFO(uart), TxChar);
}


void ICACHE_FLASH_ATTR
uart1_write_char(char c)
{
	if (c == '\n') {
		uart_tx_one_char(UART1, '\r');
		uart_tx_one_char(UART1, '\n');
	}else if (c == '\r') {

	}else
		uart_tx_one_char(UART1, c);
}
