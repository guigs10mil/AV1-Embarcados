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


#define BUT3_PIO        PIOA
#define BUT3_PIO_ID     10
#define BUT3_IDX        19
#define BUT3_IDX_MASK   1 << BUT3_IDX


#define radius          1


struct ili9488_opt_t g_ili9488_display_opt;
volatile Bool f_rtt_alarme = false;

volatile int rotations = 0;
volatile int velocity = 0;
volatile int distance = 0;


void but3_callback(void)
{
	rotations += 1;
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
		
		velocity = (int) ((float) 2*3.14*rotations/2);
		
		f_rtt_alarme = true;
	}
}


void BUT_init(void){
	/* config. pino botao em modo de entrada */
	pmc_enable_periph_clk(BUT3_PIO_ID);
	pio_set_input(BUT3_PIO, BUT3_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);

	/* config. interrupcao em borda de descida no botao do kit */
	/* indica funcao (but_Handler) a ser chamada quando houver uma interrup??o */
	pio_enable_interrupt(BUT3_PIO, BUT3_IDX_MASK);
	pio_handler_set(BUT3_PIO, BUT3_PIO_ID, BUT3_IDX_MASK, PIO_IT_FALL_EDGE, but3_callback);

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


int main(void) {
	board_init();
	sysclk_init();
	
	BUT_init();
	configure_lcd();
	
	f_rtt_alarme = true;
	
	while(1) {
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
		
		font_draw_text(&sourcecodepro_28, "GUILHERME", 50, 50, 1);
		font_draw_text(&calibri_36, "Rotacoes", 50, 100, 1);
		
		char buffer[32];
		sprintf(buffer, "%d", rotations);
		font_draw_text(&arial_72, buffer, 50, 150, 1);
		
		font_draw_text(&calibri_36, "Velocidade", 50, 250, 1);
		
		char buffer2[32];
		sprintf(buffer2, "%d", velocity);
		font_draw_text(&arial_72, buffer2, 50, 300, 1);
		
		if (f_rtt_alarme){
			uint16_t pllPreScale = (int) (((float) 32768) / 2.0);
			uint32_t irqRTTvalue  = 2;
      
			RTT_init(pllPreScale, irqRTTvalue);         

		  f_rtt_alarme = false;
		}
	}
}