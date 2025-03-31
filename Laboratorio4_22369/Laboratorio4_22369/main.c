//***************************************************************
//Universidad del Valle de Guatemala
//IE2023: Programación de Microcontroladores
//Autor: Héctor Alejandro Martínez Guerra
//Hardware: ATMEGA328P
//LAB4
//***************************************************************


/****************************************/
// Encabezado (Libraries)
#include <avr/io.h>
#include <avr/interrupt.h>	//libreria de interrupciones

volatile uint8_t counter_10ms;	//ISR del timer0 a 10ms
uint8_t counter_value = 0;		//Para contador de 8 bits

//Variables para antirrebote de botones:
uint8_t debounce_inc = 0;
uint8_t debounce_dec = 0;
uint8_t inc_pressed = 0;
uint8_t dec_pressed = 0;


/****************************************/
// Function prototypes
void	setup();
void	checkButtons();
/****************************************/
// Main Function
int main(void)
{
	setup();
	while (1)
	{
		//Si han pasado 10ms, se procesa la lectura de botones
		if (counter_10ms)
		{
			checkButtons();
			counter_10ms = 0;		//Se reinicia el contador de intervalos
		}
	}
}
/****************************************/
// NON-Interrupt subroutines

void setup()
{
	//Configurar PB0 a PB5 como salidas para el contador (6 bits)
	DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB4) | (1 << PB5);

	//Configurar PC0 y PC1 como salidas para completar los 8 bits del contador
	DDRC |= (1 << PC0) | (1 << PC1);

	//Configurar PC2 y PC3 como entradas para los botones
	DDRC &= ~((1 << PC2) | (1 << PC3));
	
	//Activar resistencias pull-up internas en PC2 y PC3
	PORTC |= (1 << PC2) | (1 << PC3);
	
	//Configurar Timer0 en modo CTC para generar interrupciones cada ~10ms
	//Para 10ms, OCR0A = 155
	TCCR0A = (1 << WGM01);							//Modo CTC
	TCCR0B = (1 << CS02) | (1 << CS00);				//Prescaler 1024 (0b101)
	OCR0A = 155;									//Valor para generar interrupciones de 10ms
	
	//Habilitar la interrupción del Timer0 Compare Match A
	TIMSK0 |= (1 << OCIE0A);
	
	//Habilitar interrupciones globales
	sei();
}


/****************************************/
// Función para leer los botones y actualizar el contador
void checkButtons()
{
	//Botón de incremento
	if (!(PINC & (1<<PC3)))
	{
		if (!inc_pressed)		//Si aún no se detectó una pulsación
		{
			debounce_inc++;		//Incrementa el contador de antirrebote
			if (debounce_inc >= 2)	//Si se mantiene 20ms presionado
			{
				counter_value++;	//Se incrementa el contador
				inc_pressed = 1;		//Botón presionado
			}
		}
	}
	else
	{
		debounce_inc = 0;
		inc_pressed = 0;
	}
	//Botón decremento
	if (!(PINC & (1<<PC2)))
	{
		if (!dec_pressed)
		{
			debounce_dec++;		//Incrementar el contador de antirrebote
			if (debounce_dec >= 2)		//Si se mantiene 20ms presionado
			{
				counter_value--;		//Decrementar el contador
				dec_pressed = 1;		//Botón presionado
			}
		}
	}
	else
	{
		debounce_dec = 0;
		dec_pressed = 0;
	}
	
	//Actualización del contador
	//Los 6 bits de PORTB (PB0-PB5)
	PORTB = (PORTB & ~0x3F) | (counter_value & 0x3F);
	//Y los 2 bits menos significativos de PORTC (PC0 y PC1)
	PORTC = (PORTC & ~0x03)	| ((counter_value>>6) & 0x03);
}

/****************************************/
// Interrupt routines
ISR(TIMER0_COMPA_vect)
{
	counter_10ms++;
}

