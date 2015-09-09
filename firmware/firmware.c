/*
 * Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/usb/usbd.h>

#include <firmware.h>
#include <jbig85.h>
#include <0013.h>

#define FLASH_ACR_PREFETCH 0x10

static const clock_scale_t clock_72MHZ = {
	.pll = RCC_CFGR_PLLMUL_PLL_IN_CLK_X9,
	.pllsrc = RCC_CFGR_PLLSRC_HSE_PREDIV,
	.hpre = RCC_CFGR_HPRE_DIV_NONE,
	.ppre1 = RCC_CFGR_PPRE1_DIV_2,
	.ppre2 = RCC_CFGR_PPRE2_DIV_NONE,
	.power_save = 0,
	.flash_config = 0,
	.apb1_frequency = 36000000,
	.apb2_frequency = 72000000,
};

#define SYSTICK_PRIORITY (0U << 4)
#define SYSTICK_FREQUENCY (1000) // 1 kHz
#define SYSTICK_ADD 1
static volatile uint32_t tick_sum = 0;

static const uint32_t page_width = 800;
static const uint32_t page_height = 600;
static struct jbg85_dec_state state;

static void
pwr_setup(void)
{
	rcc_periph_clock_enable(RCC_PWR);
	//rcc_periph_clock_enable(RCC_SYSCFG);
}

static void
flash_setup(void)
{
	FLASH_ACR |= FLASH_ACR_PREFETCH;

	flash_set_ws(FLASH_ACR_LATENCY_2WS);
}

static void
rcc_clock_setup_hse(const clock_scale_t *clock)
{
	rcc_osc_on(HSI);
	rcc_wait_for_osc_ready(HSI);
	RCC_CFGR = 0x0;
	rcc_set_sysclk_source(RCC_CFGR_SW_HSI);
	rcc_wait_for_sysclk_status(HSI);

	rcc_css_disable();

	rcc_osc_off(PLL);
	rcc_wait_for_osc_not_ready(PLL);

	rcc_osc_off(HSE);
	rcc_wait_for_osc_not_ready(HSE);

	RCC_CIR = 0x0;

	rcc_osc_on(HSE);
	rcc_wait_for_osc_ready(HSE);

	rcc_set_hpre(clock->hpre);
	rcc_set_ppre2(clock->ppre2);
	rcc_set_ppre1(clock->ppre1);

	rcc_set_pll_source(clock->pllsrc);
	rcc_set_main_pll_hsi(clock->pll);

	rcc_osc_on(PLL);
	rcc_wait_for_osc_ready(PLL);

	rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
	rcc_wait_for_sysclk_status(PLL);

	rcc_apb1_frequency = clock->apb1_frequency;
	rcc_apb2_frequency = clock->apb2_frequency;
}

static void
clock_setup(void)
{
	rcc_clock_setup_hse(&clock_72MHZ);
}

__attribute__((section(".ccm_text"))) void
sys_tick_handler(void)
{
	tick_sum += SYSTICK_ADD;
}

static void
systick_setup(void)
{
	systick_clear();
	systick_set_frequency(SYSTICK_FREQUENCY, clock_72MHZ.apb2_frequency);
	systick_interrupt_enable();
	systick_counter_enable();
	nvic_set_priority(NVIC_SYSTICK_IRQ, SYSTICK_PRIORITY);
}

static void
gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);

	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO2);
}

static void
_usb_reenumerate(void)
{
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, GPIO12);
	//gpio_clear(GPIOA, GPIO12); // lower USB_DP
	for(int i = 0; i<720000; i++) //wait 10ms
		__asm__("nop");

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF14, GPIO11| GPIO12);
}

static void
usb_setup(void)
{
	rcc_usb_prescale_1_5();
	rcc_periph_clock_enable(RCC_USB);
	rcc_periph_reset_pulse(RST_USB);

	_usb_reenumerate();
}

volatile bool connected = false;

static void
_usb_reset(void)
{
	connected = false;
}

static void
_usb_suspend(void)
{
	connected = false;
}

static void
_usb_resume(void)
{
	connected = true;
}

void
cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
	connected = true;

	uint8_t buf [64];

	uint16_t len = usbd_ep_read_packet(usbd_dev, ep, buf, 64);
	if(len == 0)
		return;

	//TODO
}

static int line = 0;
		    
static int
_line_out(const struct jbg85_dec_state *s, uint8_t *start, size_t len,
	unsigned long y, void *data)
{
	(void)s;
	(void)start;
	(void)len;
	(void)y;
	(void)data;

	//TODO
	
	line++;

	return y == page_height - 1 // end of page?
		? 1
		: 0;
}

int
main(void)
{
	usbd_device *usbd_dev;

	pwr_setup();
	flash_setup();
	clock_setup();
	systick_setup();
	gpio_setup();
	usb_setup();

	usbd_dev = mod_cdcacm_new(); // initialize USB serial

	//for(int i=0; i<0x800000; i++) //TODO how long?
	//	__asm__("nop");

	usbd_register_reset_callback(usbd_dev, _usb_reset);
	usbd_register_suspend_callback(usbd_dev, _usb_suspend);
	usbd_register_resume_callback(usbd_dev, _usb_resume);

	while(!connected)
		usbd_poll(usbd_dev);
	
	gpio_set(GPIOB, GPIO2); // turn on LED

  const size_t bufoutlen = ((page_width >> 3) + !!(page_width & 7)) * 3;
	uint8_t bufout [bufoutlen];	

	const size_t len = sizeof(compressed);

	while(1)
	{
		int result;
		size_t cnt = 0;
		size_t cnt2;

		usbd_poll(usbd_dev);

		const uint32_t t0 = tick_sum;

		jbg85_dec_init(&state, bufout, bufoutlen, _line_out, NULL);
	
		while(cnt != len)
		{
			result = jbg85_dec_in(&state, (uint8_t *)compressed + cnt, len - cnt, &cnt2);
			cnt += cnt2;

			if(result == JBG_EOK_INTR)
				continue;

			if(result != JBG_EAGAIN)
				break;
		}

		while( (result == JBG_EAGAIN) || (result == JBG_EOK_INTR) )
			jbg85_dec_end(&state);
			
		const uint32_t t1 = tick_sum;
		const uint32_t dt = t1 - t0;

		char buf [128];
		sprintf(buf, "%li %i %i %lu ms\n\r", state.y, line, result, dt);
		while(!usbd_ep_write_packet(usbd_dev, 0x82, buf, strlen(buf)))
			;

		line = 0;
	}

	return 0;
}
