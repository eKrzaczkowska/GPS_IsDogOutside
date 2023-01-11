#include "stm32f401xe.h"
#include "core_cm4.h"
#include "cmsis_gcc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"


//-----------------------------------FUNKCJE--------------------------
void uart_sendstr(volatile char *dane);
void system_clock_config(void);
void usart2_init();
void GPIO_init();
void DMA_init();

//----------------------------RTOS------------------------------------
extern void vReadNMEA(void * pvParameters);
extern void vTransformRMC(void * pvParameters);
extern void vCheckIfOut(void * pvParameters);

//------------------------------------ZMIENNE---------------------------
#define ROZMIAR 2048
#define PRZEDZIAL 10
typedef struct {
	unsigned char tab[ROZMIAR];
	uint16_t poczatek;
	uint16_t koniec;
}bufor_cykliczny;
volatile bufor_cykliczny bufor = {.poczatek=0, .koniec=0};
//-------------------------RTOS----------------------------------------
QueueHandle_t gps_gprmc = 0;

void uart_sendstr(volatile char *dane) {
	while (*dane) {
		while ( !(USART2->SR & 0x00000040));
		USART2->DR = (*dane & (uint16_t)0x01FF);
		*(dane++);
	}
}

void uart_sendNUM(volatile int dane){
		char buf[15]="";
		siprintf(buf, "%d", dane);
		uart_sendstr(buf);
}

//------------------------------PROGRAM G£ÓWNY---------------------------
int main(void)
{
	//--------------------------INICJALIZACJA----------------------------
	SCB->CPACR |= (0xF << 20); //Enable FPU on STM32
	system_clock_config(); //APB2 84MHz, APB1 42MHz, max clock 84MHz
	usart2_init();
	GPIO_init();
	DMA_init();
	HAL_Init();
	uart_sendstr("START");
	//-----------------------RTOS---------------------------------
	//gps_gprmc = xQueueCreate(1, sizeof(char)*90);

	xTaskCreate(vReadNMEA, "vReadNMEA", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
	xTaskCreate(vTransformRMC, "vTransformRMC", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(vCheckIfOut, "vCheckIfOut", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

	vTaskStartScheduler();

	while(1)
	{
//		char GPRMC[80] = {'0'};
//		getGPRMC(&GPRMC);
//		if (GPRMC[0] != '0')
//		{
//			uart_sendstr(GPRMC);
//			uart_sendstr("\r\n");
//		}
	}

}


//--------------------------------------FUNKCJE------------------------------------

void vTask1(void * pvParameters)
{
}

void getGPRMC(char *GPRMC) {


	for (; ; bufor.poczatek++)
	{
			if (bufor.poczatek==ROZMIAR)
			{
				bufor.poczatek=0;
			}
			bufor.koniec = ROZMIAR - DMA2_Stream2->NDTR;
			if ((bufor.poczatek - bufor.koniec)<PRZEDZIAL+50 && (bufor.poczatek - bufor.koniec)>-PRZEDZIAL) //Processor is faster than GPS data, so wait some time
			{
				HAL_Delay(500);
			}
			if ((bufor.tab[bufor.poczatek] == 'C') && (bufor.tab[bufor.poczatek-1] == 'M') && (bufor.tab[bufor.poczatek-2] == 'R'))
			{
				//uart_sendstr("jestem\r\n");
				bufor.poczatek++;									//Increase cyclic buffer by 1 character
				if (bufor.poczatek==ROZMIAR) { bufor.poczatek=0; } 	//Check if the cyclic buffer size was exceeded
										int j;
										GPRMC[0]='$';
										GPRMC[1]='G';
										GPRMC[2]='P';
										GPRMC[3]='R';
										GPRMC[4]='M';
										GPRMC[5]='C';
										for (j=6; j<90; j++)
										{
											if (bufor.poczatek==ROZMIAR)
											{
												//uart_sendstr("jestem3\r\n");
													bufor.poczatek=0;
											}
											bufor.koniec = ROZMIAR - DMA2_Stream2->NDTR;
											if ((bufor.poczatek - bufor.koniec)<PRZEDZIAL+50 && (bufor.poczatek - bufor.koniec)>-PRZEDZIAL)
											{
												//uart_sendstr("jestem4\r\n");
													HAL_Delay(200);
											}
											if ( bufor.tab[bufor.poczatek] =='\n')
											{
													GPRMC[j] = '\n';
													//uart_sendstr("jestem2\r\n");
													return;
											}
											GPRMC[j] = bufor.tab[bufor.poczatek];
											bufor.poczatek++;
//											uart_sendNUM(bufor.poczatek);
//											uart_sendstr("\r\n");
//											uart_sendNUM(bufor.koniec);
//											uart_sendstr("\r\n");

										}
									}
			}


}





void system_clock_config(void) {


	RCC->CR |= RCC_CR_HSION;	//HSI on

	//M=16, N=336, P=div4, Q=7
	RCC->PLLCFGR = 16ul<<0 | 336ul<<6 | RCC_PLLCFGR_PLLP_0 | 7ul<<24 | RCC_PLLCFGR_PLLSRC_HSI  | 1ul<<29;

	RCC->SSCGR = 500ul<<0 | 44ul<<13 | RCC_SSCGR_SSCGEN; //spread spectrum

	while (!(RCC->CR & RCC_CR_HSIRDY));
	RCC->CR |= RCC_CR_PLLON;
	RCC->CFGR = RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1 | RCC_CFGR_HPRE_DIV1; //dla adc RCC_CFGR_PPRE2_DIV8 (APB2)

	FLASH->ACR = FLASH_ACR_DCRST | FLASH_ACR_ICRST;
	FLASH->ACR = FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_2WS;
	while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_2WS);

	while (!(RCC->CR & RCC_CR_PLLRDY));	//wait for ready PLL
	RCC->CFGR |= RCC_CFGR_SW_PLL;	//sysclk from HSI to PLL
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
	RCC->CR &= ~RCC_CR_HSEON;	//hso off

	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	__DSB();
	SYSCFG->CMPCR = SYSCFG_CMPCR_CMP_PD;
	while (!(SYSCFG->CMPCR & SYSCFG_CMPCR_READY));

}

void usart2_init() {
	/*
	 * PA2 - USART2_TX
	 * PA3 - USART2_RX
	 */

	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
	RCC->AHB1ENR |=	RCC_AHB1ENR_GPIOAEN; //wlczenie usart2 i gpioa
	__DSB();

	GPIOA->MODER |= GPIO_MODER_MODER2_1;
	GPIOA->OTYPER &= ~GPIO_OTYPER_OT_2;
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR2;
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR2;
	GPIOA->AFR[0] |= 0x700; //funkcja alternatywna AF7 dla PA2

	GPIOA->MODER |= GPIO_MODER_MODER3_1;
	GPIOA->OTYPER |= GPIO_OTYPER_OT_3;
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR3;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR3_1;
	GPIOA->AFR[0] |= 0x7000; //funkcja alternatywna AF7 dla PA3

	USART2->BRR = 42000000/115200; //freq/baude
	USART2->CR1 = USART_CR1_UE | USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE; //| USART_CR1_TXEIE;



}

 void GPIO_init(){
	 /*
	 	 * PB6 - USART2_TX
	 	 * PB7 - USART2_RX
	 	 */
	 	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; //rejestr clock enable, bit GPIOB wlaczenie
	 	__DSB();
	 	RCC->APB2ENR |= RCC_APB2ENR_USART1EN; //wlczenie usart1
	 	__DSB();

	 	GPIOB->MODER |= GPIO_MODER_MODER7_1;
	 	GPIOB->OTYPER |= GPIO_OTYPER_OT_7;
	 	GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR7;
	 	GPIOB->PUPDR |= GPIO_PUPDR_PUPDR7_1;
	 	GPIOB->AFR[0] |= 0x70000000; //funkcja alternatywna AF7 dla PA3

	 	USART1->BRR = 84000000/9600; //freq/baude
	 	USART1->CR1 = USART_CR1_TE | USART_CR1_RE; //| USART_CR1_TXEIE; USART_CR1_UE
	 	USART1->CR3 = USART_CR3_DMAR; //odbieranie za pomoca dma
	 	USART1->CR1 |= USART_CR1_UE;
 }

 void DMA_init(){
	 	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN; //wlaczenie DMA2

	 	DMA2_Stream2->CR = 0;
	 	//DMA2_Stream2->CR |= DMA_SxCR_CHSEL_2 | DMA_SxCR_PL_1 | DMA_SxCR_CIRC | DMA_SxCR_MSIZE_0 | DMA_SxCR_PSIZE_0; //tryb memory to memory kanal4
	 	DMA2_Stream2->CR |= DMA_SxCR_CHSEL_2 | DMA_SxCR_MINC | DMA_SxCR_CIRC;
	 	DMA2_Stream2->PAR = (uint32_t)&USART1->DR; //adres skad

	 	//DMA2->LIFCR = DMA_LIFCR_CTCIF2; // czyszczenie flagi
	 	//NVIC_EnableIRQ(DMA2_Stream2_IRQn);

	 	DMA2_Stream2->M0AR = (uint32_t)&bufor.tab; //adres dokad
	 	DMA2_Stream2->NDTR = ROZMIAR; //rozmiar, liczba transakcji

	 	DMA2_Stream2->CR |= DMA_SxCR_EN; //wlaczenie

 }
