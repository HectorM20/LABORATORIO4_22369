//***************************************************************
//Universidad del Valle de Guatemala
//IE2023: Programación de Microcontroladores
//Autor: Héctor Alejandro Martínez Guerra
//Hardware: ATMEGA328P
//LAB4
//***************************************************************


/****************************************/
// Encabezado (Libraries)
#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>			//libreria de interrupciones

volatile uint8_t counter_10ms;		//ISR del timer0 a 10ms
uint8_t counter_value = 0;			//Para contador de 8 bits
volatile uint8_t adc_value = 0x00;	//Valor del ADC (8 bits, resultado en ADCH)

//Variables para antirrebote de botones:
uint8_t debounce_inc = 0;
uint8_t debounce_dec = 0;
uint8_t inc_pressed = 0;
uint8_t dec_pressed = 0;
const uint8_t SEG_TABLE[16] = {
	0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71
}; //0-F

/****************************************/
//Function prototypes
void	setup();
void	checkButtons();
void	initADC();
void	updateDisplays();
/****************************************/
//Main Function
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
		//Actualización de los displays de forma constante para multiplexación
		updateDisplays();
		//Comparación de los contadores para encender la alarma en PD7
		//Si el valor del ADC es mayor que el del contador binarío enciende LED en PD7
		if (adc_value > counter_value)
		{
			//Preserva los bits 0-6 y enciende PD7
			PORTD = (PORTD & 0x7F) | 0x80;
		}
		else
		{
			PORTD &= ~(1<<PD7);
		}
	}
}
/****************************************/
//NON-Interrupt subroutines

void setup()
{
	cli();
	CLKPR = (1<<CLKPCE);
	CLKPR = (1<<CLKPS2);		//Configurar prescaler principal = 16; F = 1MHz
	
	UCSR0A = 0x00;
	UCSR0B = 0x00;
	UCSR0C = 0x00;
	
	//Configurar PB0 a PB5 como salidas para el contador (6 bits)
	DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB4) | (1 << PB5);

	//Configurar PC0 y PC1 como salidas para completar los 8 bits del contador
	DDRC |= (1 << PC0) | (1 << PC1);

	//Configurar PC2 y PC3 como entradas para los botones
	DDRC &= ~((1 << PC2) | (1 << PC3));
	
	//Activar resistencias pull-up internas en PC2 y PC3
	PORTC |= (1 << PC2) | (1 << PC3);
	
    //Configurar salidas para los displays del ADC:
    //PC4 y PC5: control de habilitación para displays (multiplexado)
    DDRC |= (1 << PC4) | (1 << PC5);
    //PORTD (PD0 a PD6): salidas para los segmentos del display
    DDRD |= 0x7F;									//Bits 0 a 6
	
	//Alarma
	DDRD |= (1<<PD7);
	
	//Configurar Timer0 en modo CTC para generar interrupciones cada 10ms
	//Para 10ms, OCR0A = 155
	TCCR0A = (1 << WGM01);							//Modo CTC
	TCCR0B = (1 << CS01) | (1 << CS00);				//Prescaler 64
	OCR0A = 155;									//Valor para generar interrupciones de 10ms
	
	//Habilitar la interrupción del Timer0 Compare Match A
	TIMSK0 |= (1 << OCIE0A);
	
	//Iniciar el ADC
	initADC();
	
	//Habilitar interrupciones globales
	sei();
}


/****************************************/
//Función para leer los botones y actualizar el contador
void checkButtons()
{
	//Botón de incremento
	//Al presionar el botón, el pin se pone en bajo, por lo que !(PINC & (1<<PC3)) será verdadero.
	if (!(PINC & (1<<PC3)))
	{
		if (!inc_pressed)						//Si aún no se detectó una pulsación
		{
			debounce_inc++;						//Incrementa el contador de antirrebote
			if (debounce_inc >= 3)				//Si se mantiene 30ms presionado
			{
				counter_value++;				//Se incrementa el contador
				inc_pressed = 1;				//Botón presionado
			}
		}
	}
	else
	{
		//Si el botón NO está presionado (alto), se reinicia el contador de antirrebote y la bandera de pulsación
		debounce_inc = 0;
		inc_pressed = 0;
	}
	//Botón decremento
	//Al presionar el botón, el pin se pone en bajo, por lo que !(PINC & (1<<PC2)) será verdadero.
	if (!(PINC & (1<<PC2)))
	{
		if (!dec_pressed)						//Si aún no se detectó una pulsación
		{
			debounce_dec++;						//Incrementar el contador de antirrebote
			if (debounce_dec >= 3)				//Si se mantiene 30ms presionado
			{
				counter_value--;				//Decrementar el contador
				dec_pressed = 1;				//Botón presionado
			}
		}
	}
	else
	{
		//Si el botón NO está presionado (alto), se reinicia el contador de antirrebote y la bandera de pulsación
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
//ADC
void initADC()
{
	ADMUX = 0;
	ADMUX |= (1<<REFS0);			//Voltaje de referencia AVcc
	ADMUX |= (1<<ADLAR);			//Justificación a la izquierda
	ADMUX |= (1<<MUX2) | (1<<MUX1);	//Configurar ADC6 del Arduino nano
	
	ADCSRA = 0;
	//Configurar Prescaler a 8 (1MHz/8 = 125KHz)
	ADCSRA |= (1 << ADPS1) | (1 << ADPS0);
	//Habilitar ADC y sus interrupciones
	ADCSRA |= (1<<ADEN) | (1<<ADIE);
	
	//Empezar ADC 
	ADCSRA |= (1<<ADSC);
}

void updateDisplays()
{
	static uint8_t currentDisplay = 0;
	static uint16_t refreshCount = 0;
	
	//Incrementa el contador de refresco
	refreshCount++;
    //Solo actualiza el dígito cada cierto número de ciclos
    if(refreshCount < 50) return;
    refreshCount = 0;					//Reinicia el contador
    //Desactivar ambos displays para evitar solapamientos
    PORTC &= ~((1 << PC4) | (1 << PC5));
	
	if (currentDisplay == 0)
	{
		//Mostrar el nibble bajo (unidades)
		uint8_t lowNibble = adc_value & 0x0F;
		//Actualiza PD0–PD6, preservando PD7
		PORTD = (PORTD & 0x80) | SEG_TABLE[lowNibble];
		//Habilitar display de unidades (PC4)
		PORTC |= (1 << PC4);
	}
	else
	{
		//Mostrar el nibble alto (decenas)
		uint8_t highNibble = (adc_value >> 4) & 0x0F;
		//Actualiza PD0–PD6, preservando PD7
		PORTD = (PORTD & 0x80) | SEG_TABLE[highNibble];
		//Habilitar display de decenas (PC5)
		PORTC |= (1 << PC5);
	}
	//Alterna entre 0 y 1 para la multiplexación de los displays
	currentDisplay = !currentDisplay;
}

/****************************************/
//Interrupt routines
ISR(TIMER0_COMPA_vect)
{
	counter_10ms++;
}

ISR(ADC_vect)
{
	//Debido a ADLAR, el resultado de 8 bits se encuentra en ADCH
    adc_value = ADCH;
    //Iniciar la siguiente conversión
    ADCSRA |= (1 << ADSC);
}

