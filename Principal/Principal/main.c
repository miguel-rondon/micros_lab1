/*
 * Principal.c
 *
 * Created: 28/09/2021 10:22:22 p. m.
 * Author : Ivan Holguin 2171120  -  Miguel Rondon 2166391
 */

#define F_CPU 16000000UL
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>

int btstop, btsensor1, btsensor2, btpower, motor, servo, buzzerStop, Contbuzzer;
int A = 0;
int B = 0;
int cnt = 0;
int IngredienteA = 0;
int IngredienteB = 0;
int PausaMotor = 0;
int ledtemp45 = 0;
int inicioservo = 0;
int ledtempmin = 0;
int Posicion_servo = 0;

void motoragitador(void) // CONFIGURACION Y REGISTROS MOTOR AGITADOR
{

	TCCR2A |= (1 << COM2A1) | (0 << COM2A0) | (1 << WGM20) | (1 << WGM21); // MODO FAST PWM NO INVERSOR
	TCCR2B |= (0 << CS22) | (1 << CS21) | (0 << CS20);					   // clkT2S/  PRESCALER 64
}

void temporizador(void) // CONTADOR TIMER0 POR DESBORDAMIENTO PARA TEMPORIZADOR 45 SEG Y 1 MIN
{
	TCCR0A = 0b00000000; // TIMMER EN MODO NORMAL
	TCCR0B = 0b00000101; // PREESCALER DE 1024 PARA DESBORDAMIENTO "OVER FLOW" CADA 10 MS
	TIMSK0 = 0b00000001; // HABILITO LA INTERRUPCION POR DESBORDAMIENTO
	TIFR0 = 0;
	TCNT0 = 100; // VALOR DEL REGISTRO PARA UN PERIODO DE SALIDA DE 10 MS
}

void Buzzerstop(void) // CONFIGURACION DEL BUZER
{
	TCCR1A |= (1 << COM1B0); //Toggle OC1A/OC1B on compare match
	TCCR1B |= (1 << WGM12);	 //MODO CTC con comparador OCR1   (WGM13=0, WGM12=1, WGM11=0, WGM10=0)
	TCCR1B |= (1 << CS12);	 //lkI/O/256 (from prescaler)   (CS12=1, CS11=0, CS10=0)
}

void servomotor(void) // CONFIGURACION SERVOMOTOR (EN EL MAIN)
{
}

// Motor Agitador ideal

// DDRB = 0b11111111; // ESTABLECEMOS PUERTOS B COMO SALIDA (1)
// TCCR1A = 0b10000010;
// TCCR1B = 0b00011001;
// ICR1 = 1600;
// OCR1A = 480; // Dutty Cicle  OCR1A=480=(30%)   0CR1A=1280=(80%)

int main(void)
{
	//Entradas (0) y Salidas (1)

	DDRB = 0b11111111; //A = PC0, B = PC1 , SERVO = OC1A(PB1), BUZZER =(PC4) , MOTOR = OC2A(PB3),
	DDRC = 0b11111111; //btstop= PD2, btsensor1= PD3, btsenosr2= PD4, btpower=PD5

	//Interrupciones de los puertos PCINT2  21

	DDRD &= ~(1 << 5);	 //Establecer como entrada
	PCICR = 0b000000110; //grupo 1 Y GRUPO 2 (8- 14) (16- 23)
	PCMSK2 = 0b00100000; // Habilitar  21

	//Interrupciones de los puertos PCINT1  DEL PUERTO PCIN 10

	DDRC &= ~(1 << 2);	 //Establecer como entrada
	PCMSK1 = 0b00000100; // Habilitar  puerto 10

	//Interrupciones INT1 e INT0

	EIMSK = 0b00000011; //HABILITAR INT1 E INT0
	EICRA = 0b00000001; // ACTIVACION POR FLANCO SUBIDA
	EIFR = 0;			// Banderas de INT0 limpias

	// CONFIGURACION TIMMER 2
	TCCR2B = 0b00000100; //PRESCALER (64)
	TIMSK2 = 0b00000001; // REGISTRO DESBORDAMIENTO
	TCNT2 = 6;			 // 256 - (F_cpu* Tpout)/PRESCALER    (256 - (16Mhz*1ms)/64)
	//

	// CONFIGURACION SERVOMOTOR
	TCNT1 = 0;										   // REINICIAR
	ICR1 = 20000;									   // PERIODO DE SE�AL 20 ms
	TCCR1A = (1 << COM1A1) | (0 << COM1A0);			   // COMPARACION EN BAJO
	TCCR1A |= (1 << WGM11) | (0 << WGM10);			   // Fast PWM: TOP: ICR1
	TCCR1B = (1 << WGM13) | (1 << WGM12);			   // // Fast PWM: TOP: ICR1
	TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10); // PRESCALER 8

	sei(); // HABILITAR INTERRUPCIONES GLOBALES .-.

	while (1)
	{
		// CONDICIONAL INICIO DE PROCESO

		if (btpower == 1)
		{
			// CONDICION CUMPLIMIENTO DE INGEDIENTE A
			if (A == 5)
			{
				PORTC &= ~(1 << 0); // APAGAR LED A
				IngredienteA = 1;
			}

			// CONDICION CUMPLIMIENTO DE INGEDIENTE B
			if (B == 29)
			{
				PORTC &= ~(1 << 1); // APAGAR LED B
				IngredienteB = 1;
			}

			// CONDICIONAL INGREDIENTES COMPLETOS EN LA MEZCLA
			if (IngredienteA == 1 && IngredienteB == 1)
			{
				//MODIFICAR MOTOR AGITADOR AL 80%
				motoragitador();
				OCR2A = 204;
			}
			else
			{
				// MODIFICAR MOTOR AL 30%
				motoragitador();
				OCR2A = 77;
			}

			// CONDICION AL PRESIONAR BOTON STOP

			if (btstop == 1)
			{
				buzzerStop = 1;
				btstop = 1;
				OCR2A = 0;
				PORTC &= ~(1 << 0);
				PORTC &= ~(1 << 1);
				PORTB &= ~(1 << 3);
				PORTB &= ~(1 << 2);
				PORTB &= ~(1 << 4);
				PORTB &= ~(1 << 5);
			}

			// CONDICION AL ESPERAR 1 MINUTO

			if (ledtempmin == 1)
			{
				//APAGAR MOTOR AL 0% DUTTY CICLE
				motoragitador();
				OCR2A = 0;
			}

			// CONDICONAL PARA MODOFICAR SERVOMOTOR AL 135�
			if (inicioservo)
			{
				//ENCENDER SERVOMOTOR A 135�
				servomotor();
				OCR1A = 1749;	   // SERVO A 135�;
				DDRB &= ~(1 << 1); // CONFIGURAR PB1 COMO SALIDA
				DDRB |= (1 << 1);
				Posicion_servo = 1749; // GUARDAR POSICION DEL SERVO
			}

		} //btpower

	} // while

} // main

ISR(TIMER0_OVF_vect) //TEMPORIZADOR 1 MINUTO Y 45 SEGUNDOS
{
	cnt++;

	if (cnt >= 2799) // 2799 :56 segundos
	{
		inicioservo = 1;
		ledtempmin = 1;

		DDRC |= (1 << 4); //BUZZER
		PORTC = (1 << 4); // Puerto "PC4" esta en alto   (prender el pc4)

		// CONDICION PARA BUZZER (2 BEEPS)

		if (cnt >= 2805 && cnt <= 2810)
		{
			PORTC &= ~(1 << 4); // Puerto "PC4" en bajo
		}
		else if (cnt >= 2810 && cnt <= 2815)
		{
			PORTC = (1 << 4); // Puerto "PC4" esta en alto
		}
		else if (cnt >= 2815 && cnt <= 2820)

		{
			PORTC &= ~(1 << 4); // Puerto "PC4" en bajo
		}
	}
	// PROCESO FINAL ESPERAR 45 SEGUNDOS Y CONDICIONES INICIALES (1 min +45 seg) cnt=2799 =56 seg, cnt= 2199 =45 segundos

	if (cnt >= 4998)
	{
		IngredienteA = 0;
		IngredienteB = 0; // Variables iniciales (0)
		A = 0;
		B = 0;
		cnt = 0;
		motoragitador();
		OCR2A = 0;
		servomotor();
		OCR1A = 0; //servomotor en 0�
	}

	TCNT0 = 100;
}

ISR(INT0_vect) // ACCION DE PRESIONAR BOTON STOP
{
	btstop = 1;
	btpower = 0;
	buzzerStop = 1;
}

ISR(INT1_vect) // ACCIONAR SENSOR 1 DE INGREDIENTE A
{
	if (btpower == 1)
	{
		A++;
	}
}

ISR(PCINT1_vect) // ACCIONAR SENSOR 2 DE INGREDIENTE B
{
	if (btpower == 1)
	{
		B++;
	}
}

ISR(PCINT2_vect) // PRESIONAR BOTON POWER (INICIO)
{
	temporizador();
	motoragitador();
	PORTC |= (1 << 5);
	OCR2A = 77;
	btpower = 1;
	btstop = 0;

	if (btpower == 1)
	{
		PORTC = 0b00000011; // ENCENDER LED A y B
	}
	else
	{
		PORTC = 0b00000000; // APAGAR LED A y B
	}
}
ISR(TIMER2_OVF_vect) // TIMER BUZZER DEL BOTON STOP Y PARAR
{
	Contbuzzer++;

	if (Contbuzzer == 250) // 250 ms (500ms/2) = 50%
	{
		Contbuzzer = 0;

		//CONDICONAL SI EST� APAGADO Y BOTON STOP PRESIONADO

		if ((btpower == 0) || (buzzerStop == 1))
		{
			if (btstop == 1)
			{

				PORTC ^= (1 << 4);
			}
			else
			{
				PORTC &= ~(1 << 4);
			}
		}
		else
		{
			PORTC ^= (1 << 4);
		}
	}
}

ISR(TIMER1_OVF_vect) //SERVO MOTOR Y BUZZER
{

	Buzzerstop();
	OCR1A = 7813;
	DDRB |= (1 << PORTB2); //DEFINIR PUERTO PB2 COMO SALIDA
}
