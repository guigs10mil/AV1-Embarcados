/*
 * main.c
 *
 * Created: 05/03/2019 18:00:58
 *  Author: eduardo
 */ 

#include <asf.h>
#include "tfont.h"
#include "sourcecodepro_28.h"
#include "calibri_36.h"
#include "arial_72.h"
#include "math.h"

#define YEAR        2018
#define MOUNTH      3
#define DAY         19
#define WEEK        12
#define HOUR        0
#define MINUTE      0
#define SECOND      0

#define BUT2_PIO      PIOC
#define BUT2_PIO_ID   12
#define BUT2_IDX  31
#define BUT2_IDX_MASK (1 << BUT2_IDX)

#define BUT3_PIO        PIOA
#define BUT3_PIO_ID     10
#define BUT3_IDX        19
#define BUT3_IDX_MASK   1 << BUT3_IDX

#define radius          1

#define Y_info_0        50
#define Y_info_1        170
#define Y_info_2        290
#define Y_info_3        420


struct ili9488_opt_t g_ili9488_display_opt;
volatile Bool f_rtt_alarme = false;
volatile int flag_reset = 1;

volatile int rotations = 0;
volatile int total_rotations = 0;
volatile int velocity = 0;
volatile int distance = 0;
volatile int seconds = 0;
volatile int minutes = 0;
volatile int hours = 0;

volatile int idle_counter = 0;

void but2_callback(void)
{
	if (flag_reset) {
		flag_reset = 0;
	} else {
		distance = 0;
		total_rotations = 0;
		seconds = 0;
		minutes = 0;
		hours = 0;
		flag_reset = 1;
	}
}

void but3_callback(void)
{
	rotations += 1;
	total_rotations += 1;
}

void RTT_Handler(void)
{
	uint32_t ul_status;

	/* Get RTT status */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) { 
		
	}

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		
		velocity = calc_velocity(rotations);
		
		distance = calc_distance(total_rotations);
		
		f_rtt_alarme = true;
	}
}

void RTC_Handler(void)
{
	uint32_t ul_status = rtc_get_status(RTC);

	/*
	*  Verifica por qual motivo entrou
	*  na interrupcao, se foi por segundo
	*  ou Alarm
	*/
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
			rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
			rtc_set_date_alarm(RTC, 1, MOUNTH, 1, DAY);
			int hora, min, sec;
			rtc_get_time(RTC, &hora, &min, &sec);
			if (sec >= 59) {
				if (min >= 59) {
					rtc_set_time_alarm(RTC, 1, 0, hora+1, 0, 1, 0);
					hours += 1;
					minutes = 0;
				} else {
					rtc_set_time_alarm(RTC, 1, hora, 1, min+1, 1, 0);
					minutes += 1;
				}
				seconds = 0;
			} else {
				rtc_set_time_alarm(RTC, 1, hora, 1, min, 1, sec+1);
				seconds += 1;
			}
			
	}
	
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
	
}


void BUT_init(void){
	/* config. pino botao em modo de entrada */
	pmc_enable_periph_clk(BUT2_PIO_ID);
	pio_set_input(BUT2_PIO, BUT2_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	/* config. pino botao em modo de entrada */
	pmc_enable_periph_clk(BUT3_PIO_ID);
	pio_set_input(BUT3_PIO, BUT3_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);

	/* config. interrupcao em borda de descida no botao do kit */
	/* indica funcao (but_Handler) a ser chamada quando houver uma interrup??o */
	pio_enable_interrupt(BUT2_PIO, BUT2_IDX_MASK);
	pio_handler_set(BUT2_PIO, BUT2_PIO_ID, BUT2_IDX_MASK, PIO_IT_FALL_EDGE, but2_callback);
	/* config. interrupcao em borda de descida no botao do kit */
	/* indica funcao (but_Handler) a ser chamada quando houver uma interrup??o */
	pio_enable_interrupt(BUT3_PIO, BUT3_IDX_MASK);
	pio_handler_set(BUT3_PIO, BUT3_PIO_ID, BUT3_IDX_MASK, PIO_IT_FALL_EDGE, but3_callback);

	/* habilita interrup?c?o do PIO que controla o botao */
	/* e configura sua prioridade                        */
	NVIC_EnableIRQ(BUT2_PIO_ID);
	NVIC_SetPriority(BUT2_PIO_ID, 1);
	/* habilita interrup?c?o do PIO que controla o botao */
	/* e configura sua prioridade                        */
	NVIC_EnableIRQ(BUT3_PIO_ID);
	NVIC_SetPriority(BUT3_PIO_ID, 1);
};

void configure_lcd(void){
	/* Initialize display parameter */
	g_ili9488_display_opt.ul_width = ILI9488_LCD_WIDTH;
	g_ili9488_display_opt.ul_height = ILI9488_LCD_HEIGHT;
	g_ili9488_display_opt.foreground_color = COLOR_CONVERT(COLOR_WHITE);
	g_ili9488_display_opt.background_color = COLOR_CONVERT(COLOR_WHITE);

	/* Initialize LCD */
	ili9488_init(&g_ili9488_display_opt);
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
	
}

int calc_distance(int rot) {
	return (int) ((float) 0.65*2*M_PI*rot);
}

int calc_velocity(int rot) {
	return (int) ((float) 3.6*0.65*2*M_PI*rot/2);
}

void font_draw_text(tFont *font, const char *text, int x, int y, int spacing) {
	char *p = text;
	while(*p != NULL) {
		char letter = *p;
		int letter_offset = letter - font->start_char;
		if(letter <= font->end_char) {
			tChar *current_char = font->chars + letter_offset;
			ili9488_draw_pixmap(x, y, current_char->image->width, current_char->image->height, current_char->image->data);
			x += current_char->image->width + spacing;
		}
		p++;
	}	
}

static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses)
{
	uint32_t ul_previous_time;

	/* Configure RTT for a 1 second tick interrupt */
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	ul_previous_time = rtt_read_timer_value(RTT);
	while (ul_previous_time == rtt_read_timer_value(RTT));
	
	rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);

	/* Enable RTT interrupt */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 0);
	NVIC_EnableIRQ(RTT_IRQn);
	rtt_enable_interrupt(RTT, RTT_MR_ALMIEN);
}

void RTC_init(){
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(RTC, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(RTC, YEAR, MOUNTH, DAY, WEEK);
	rtc_set_time(RTC, HOUR, MINUTE, SECOND);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(RTC_IRQn);
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_SetPriority(RTC_IRQn, 0);
	NVIC_EnableIRQ(RTC_IRQn);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(RTC, RTC_IER_ALREN);

}


int main(void) {
	board_init();
	sysclk_init();
	
	BUT_init();
	configure_lcd();
	
	RTC_init();
	
	rtc_set_date_alarm(RTC, 1, MOUNTH, 1, DAY);
	rtc_set_time_alarm(RTC, 1, HOUR, 1, MINUTE, 1, SECOND+1);
	
	rotations = 0;
	total_rotations = 0;
	distance = 0;
	flag_reset = 1;
	
	f_rtt_alarme = true;
	
	font_draw_text(&sourcecodepro_28, "GUILHERME", 30, 20, 1);
	font_draw_text(&calibri_36, "Rotacoes", 30, Y_info_0, 1);
	
	char buffer[32];
	sprintf(buffer, "%d", rotations);
	font_draw_text(&arial_72, buffer, 30, Y_info_0+30, 1);
	
	font_draw_text(&calibri_36, "Velocidade (km/h)", 30, Y_info_1, 1);
	
	char buffer2[32];
	sprintf(buffer2, "%d", velocity);
	font_draw_text(&arial_72, buffer2, 30, Y_info_1+30, 1);
	
	font_draw_text(&calibri_36, "Distancia (m)", 30, Y_info_2, 1);
	
	char buffer3[32];
	sprintf(buffer3, "%d", distance);
	font_draw_text(&arial_72, buffer3, 30, Y_info_2+30, 1);
	
	char buffer4[32];
	sprintf(buffer4, "%02d:%02d:%02d", hours, minutes, seconds);
	font_draw_text(&calibri_36, buffer4, 30, Y_info_3, 1);
	
	while(1) {
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	
		if (flag_reset) {
			if (f_rtt_alarme){
				uint16_t pllPreScale = (int) (((float) 32768) / 2.0);
				uint32_t irqRTTvalue  = 8;
				
				RTT_init(pllPreScale, irqRTTvalue);

				f_rtt_alarme = false;
				
				ili9488_draw_filled_rectangle(30, Y_info_0+30, ILI9488_LCD_WIDTH-51, Y_info_0+100);
				ili9488_draw_filled_rectangle(30, Y_info_1+30, ILI9488_LCD_WIDTH-51, Y_info_1+100);
				ili9488_draw_filled_rectangle(30, Y_info_2+30, ILI9488_LCD_WIDTH-51, Y_info_2+100);
				
				font_draw_text(&sourcecodepro_28, "GUILHERME", 30, 20, 1);
				font_draw_text(&calibri_36, "Rotacoes", 30, Y_info_0, 1);
				
				
				sprintf(buffer, "%d", rotations);
				font_draw_text(&arial_72, buffer, 30, Y_info_0+30, 1);
				
				font_draw_text(&calibri_36, "Velocidade (km/h)", 30, Y_info_1, 1);
				
				
				sprintf(buffer2, "%d", velocity);
				font_draw_text(&arial_72, buffer2, 30, Y_info_1+30, 1);
				
				font_draw_text(&calibri_36, "Distancia (m)", 30, Y_info_2, 1);
				
				
				sprintf(buffer3, "%d", distance);
				font_draw_text(&arial_72, buffer3, 30, Y_info_2+30, 1);
				
				rotations = 0;
			}
			
			ili9488_draw_filled_rectangle(30, Y_info_3, ILI9488_LCD_WIDTH-51, ILI9488_LCD_HEIGHT-51);
			sprintf(buffer4, "%02d:%02d:%02d", hours, minutes, seconds);
			font_draw_text(&calibri_36, buffer4, 30, Y_info_3, 1);
		}
	}
}