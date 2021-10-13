/*
 * Sprint9.c
 *
 * Author : gabriel
 */ 

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "PCD8544/nokia5110.h"

uint8_t selec = 0;
uint8_t greenTime = 1;
uint8_t redTime = 1;
uint8_t yellowTime = 1;
uint32_t tempo_ms = 0;
uint8_t mode = 1;//define se o modo é automatico ou manual
uint16_t qtCars = 0;
uint16_t carsPerMin = 0;
unsigned char carsLCD[10];
uint16_t LUX;//para armazenar a lux
uint8_t flag_500ms = 0;//tentar usar flag pra verificar o tempo de conversao
uint8_t flag_boat = 0;//flag para informar que um navio vem vindo
uint8_t flag_time_servo = 0;//flag para indicar que o tempo de fechar o cruzamento já passou
uint8_t flag_walkers = 0;//flag para indicar que alguém pressionou o botão para o semáforo de pedestres
uint8_t people = 0, flag_luz = 0;

unsigned char TimeGreen[3], TimeRed[3], TimeYellow[3];//variável usada para armazenar (em char) o valor (int) do tempo de cada sinal
unsigned char luxStr[50];//variavel que vai armazenar o valor de lux em string para ser impresso no display

ISR(INT0_vect){//função responsável por simplesmente mover o cursor no LCD de acordo com o número de vezes que a chave S foi pressionada
	
	selec++;
	if(selec==4){//após ser pressionada 3 vezes, pra voltar pra cor inicial, a variavel de contagem é zerada
		selec =0;
	}
	switch(selec){
		case 0:
			nokia_lcd_set_cursor(40,5);
			nokia_lcd_write_string("<",1);
			nokia_lcd_set_cursor(40,15);
			nokia_lcd_write_string(" ",1);//cursor na linha do tempo do modo
			nokia_lcd_set_cursor(40,25);
			nokia_lcd_write_string(" ",1);
			nokia_lcd_set_cursor(40,35);
			nokia_lcd_write_string(" ",1);
			nokia_lcd_render();
			break;
		case 1:
			nokia_lcd_set_cursor(40,5);
			nokia_lcd_write_string(" ",1);
			nokia_lcd_set_cursor(40,15);
			nokia_lcd_write_string("<",1);//cursor na linha do tempo do verde
			nokia_lcd_set_cursor(40,25);
			nokia_lcd_write_string(" ",1);
			nokia_lcd_set_cursor(40,35);
			nokia_lcd_write_string(" ",1);
			nokia_lcd_render();
			break;
		case 2:
			nokia_lcd_set_cursor(40,5);
			nokia_lcd_write_string(" ",1);
			nokia_lcd_set_cursor(40,15);
			nokia_lcd_write_string(" ",1);
			nokia_lcd_set_cursor(40,25);
			nokia_lcd_write_string("<",1);//cursor na linha do tempo do vermelho
			nokia_lcd_set_cursor(40,35);
			nokia_lcd_write_string(" ",1);
			nokia_lcd_render();
			break;
		case 3:
			nokia_lcd_set_cursor(40,5);
			nokia_lcd_write_string(" ",1);
			nokia_lcd_set_cursor(40,15);
			nokia_lcd_write_string(" ",1);
			nokia_lcd_set_cursor(40,25);
			nokia_lcd_write_string(" ",1);
			nokia_lcd_set_cursor(40,35);
			nokia_lcd_write_string("<",1);//cursor na linha do tempo do amarelo
			nokia_lcd_render();
			break;
		default:
			break;
	}
}


ISR(PCINT2_vect){
	
	if(!(PIND&(1<<7))){
		switch(selec){
			case 0://caso esteja selecionado o modo
				if(mode==1)
					mode = 0;//alterna entre os modos assim como no botão mais
				else
					mode = 1;

			break;
			
			case 1://caso o botao de decrementar seja pressionado enquanto o cursor estiver na linha do tempo verde
				while(greenTime<9){
					greenTime++;//a variavel que controla o tempo do verde é incrementada
					break;
				}
			break;
			
			case 2://caso o botao de decrementar seja pressionado enquanto o cursor estiver na linha do tempo vemelho
				while(redTime<9){
					redTime++;//a variavel que controla o tempo do vermelho é incrementada
					break;
				}
			break;
			
			case 3://caso o botao de decrementar seja pressionado enquanto o cursor estiver na linha do tempo amarelo
				while(yellowTime<9){
					yellowTime++;//a variavel que controla o tempo do amarelo é incrementada
					break;
				}
			break;
			
			default:
			break;
		}
	}
		
	else if(!(PIND&(1<<4))){
		
		switch(selec){
			case 0://caso a seleção esteja no botao do modo
			if(mode==1)
			mode = 0;//se estiver em 1 passa para 0,  se estiver em 0 passa para 1
			//sendo 0 o automático, e 1 o manual
			else
			mode = 1;
			
			break;
			
			case 1://caso o botao de decrementar seja pressionado enquanto o cursor estiver na linha do tempo verde
			while(greenTime>1){
				greenTime--;//a variavel que controla o tempo do verde é decrementada
				break;
			}
			break;
			
			case 2://caso o botao de decrementar seja pressionado enquanto o cursor estiver na linha do tempo vemelho
			while(redTime>1){
				redTime--;//a variavel que controla o tempo do vermelho é decrementada
				break;
			}
			break;
			
			case 3://caso o botao de decrementar seja pressionado enquanto o cursor estiver na linha do tempo amarelo
			while(yellowTime>1){
				yellowTime--;//a variavel que controla o tempo do amarelo é decrementada
				break;
			}
			break;
			
			default:
			break;
		}

	}
	Times();
	
}

ISR(PCINT0_vect){//interrupção que é responsável por fazer a contagem de carros na pista (bordas de descida do gerador de ondas)
	
	if(!(PINB&(1<<6))){
		qtCars++;
	}
	
	else if (!(PINB&(1<<7)))
	{
		flag_boat = 1;//o pino B7 indica que um navio vai passar
	}

}


ISR(TIMER0_COMPA_vect){//a cada 1ms a função de interrupção do Timer é chamada
	static uint32_t previous_TEMPO_MS_cars = 0;
	static uint32_t previous_TEMPO_MS_LDR = 0;//duas variaveis de tempo anterior diferente para lidar com contagens diferentes
	static uint32_t previous_TEMPO_MS_boat = 0; //variavel para contagem do tempo do fechamento do cruzamento

	tempo_ms ++;//e simplesmente é adicionado +1 na variavel que controla quantos ms passaram desde a execução do código

		if((tempo_ms - previous_TEMPO_MS_cars)>=5000){

			carsPerMin = qtCars*12;//conversão de qtd de carros no segundo para qtd de carros por minuto
			previous_TEMPO_MS_cars = tempo_ms;//atualização do tempo anterior com o tempo atual
			qtCars = 0;//a qtd de carros é zerada para que após 5s cada contagem parta do zero, evitando acumular carros anteriores na proxima contagem
			Times();
		}
		if(mode == 0){
			greenTime = carsPerMin/60 + 1;//equação que define o tempo do verde automaticamente
			redTime = 10 - greenTime;//equação que define o tempo do vermelho automaticamente
			//Times();
		}
		
		if((tempo_ms - previous_TEMPO_MS_LDR)>=500){
			flag_500ms = 1;//flag que indica que se passaram 500ms
	
				if ((PIND & 0b01000000) == 0b00000000)//verifica se a chave do pino D1 foi pressionada pro GND
				{
					people = 1;//caso sim, existe alguem e isso é registrado nessa variavel
				}
				if ((1023000/ADC - 1000) < 300){// Caso possua intensidade menor que 300 Lux
					flag_luz = 1;//flag q indica a baixa luminosidade
				} else{
					OCR0B = 0;//caso o LUX seja maior que 300 (está de dia), luminaria fica apagada
				}
			previous_TEMPO_MS_LDR = tempo_ms;//retoma a contagem do tempo do ultimo valor contado
			
		}

}

void Times();//atualiza valores dos tempos das cores no LCD

void stopLight(void);//controla todo apagar de LEDs no semáforo

void readLDR2lux(uint8_t *flag_500ms);//a função que vai ser responsável por converter o valor lido do LDR em lux

void luminary();

void USART_Transmit(uint8_t);

void boatSignal();//função do modo como o sinal fica quando o navio está passando

int main(void)
{
	DDRB = 0b00111111;// habilitou os pinos B0 à B5 como saída, em 1, e os demais em 0, como entrada, inclusive o B6 que será usado agora
	DDRD = 0b00101010;  //habilitou os pinos D como entrada com exceção do D5 e D3 que são as saídas PWM e o D1 que é o TX
	PORTD = 0b11111111;
	
	DDRC = 0b1111110;//habilitou os pinos C6 -> C1 como saída e o C0 e o C5 como entrada
	PORTC = 0b0111110;//desabilita o pull-up do C0
	ADMUX = 0b01000000;//Vcc como fundo de escala, ajuste à esquerda, sinal de entrada vem do C0
	ADCSRA = 0b11100111;//habilita o conversor e a interrupção, conversão contínua e o prescaler de 128 que é o unico possível
	ADCSRB = 0b00;//conversão contínua
	DIDR0 = 0b00; //PC0 é a entrada do ADC0
	
	TCCR0A = 0b10100011;//habilita o modo CTC do TC0
	TCCR0B = 0b00000011;//utiliza-se o prescaler = 64
	OCR0A = 249;		//o comparador do TC0 conta até 249 (250 contagens)
	OCR0B = 77;			//intensidade de 30% do brilho da luminária
	TIMSK0 = 0b00000010;//habilita interrupção quando há igualdade na comparação com o OCR0A  
	
	/*ICR1 = 39999;
	TCCR2A = 0b10100010;//habilita o modo CTC do TC2
	TCCR2B = 0b00011010;//utiliza-se o prescaler = 8
	OCR2B = 3000;*/
	
	EICRA = 0b00001010;//uma borda de descida em INT1 e INT0 pede a interrupção
	EIMSK = 0b00000011;//habilita-se as interrupçoes em INT0 e INT1
	
	PCICR = 0b0000101;//habilita as interrupções nas portas D (PCINT2) e nas portas B (PCINT0)
	PCMSK2 = 0b11010000;//habilita individualmente, o pino D6, D7 e D4
	PCMSK0 = 0b11000000;//habilita individualmente o pino B6 e o B7
	
	UBRR0H = (unsigned char)(MYUBRR>>8);//ajusta a taxa de transmissão, parte alta
	UBRR0L = (unsigned char)MYUBRR;//parte baixa
	UCSR0B = (1<<RXEN0)|(1<<RXEN0)|(1<<TXEN0);//habilita o transmissor e o recepctor
	UCSR0C = (3<<UCSZ00);//ajusta o formato do frame, sendo de 8 bits de dados com 2 de parada
	 
	
	sei();//habilita o uso de interrupts
	
	nokia_lcd_init();
	nokia_lcd_clear();
	nokia_lcd_set_cursor(0,5);
	nokia_lcd_write_string("Modo", 1);
	nokia_lcd_set_cursor(40,5);
	nokia_lcd_write_string("<",1);
	nokia_lcd_set_cursor(0,15);
	nokia_lcd_write_string("T. Vd", 1);//todas essas funções sao pra iniciar a tela do LCD
	nokia_lcd_set_cursor(0,25);
	nokia_lcd_write_string("T. Vm",1);
	nokia_lcd_set_cursor(0,35);
	nokia_lcd_write_string("T. Am", 1);
	nokia_lcd_set_cursor(45,0);
	nokia_lcd_write_string("|",2);
	nokia_lcd_set_cursor(45,5);
	nokia_lcd_write_string("|",2);
	nokia_lcd_set_cursor(45,10);
	nokia_lcd_write_string("|",2);
	nokia_lcd_set_cursor(45,15);
	nokia_lcd_write_string("|",2);
	nokia_lcd_set_cursor(45,20);
	nokia_lcd_write_string("|",2);
	nokia_lcd_set_cursor(45,25);
	nokia_lcd_write_string("|",2);
	nokia_lcd_set_cursor(45,30);
	nokia_lcd_write_string("|",2);
	nokia_lcd_set_cursor(45,35);
	nokia_lcd_write_string("|",2);
	nokia_lcd_set_cursor(55,15);
	nokia_lcd_write_string("lux",1);
	nokia_lcd_set_cursor(55,40);
	nokia_lcd_write_string("c/min",1);
	Times();
	nokia_lcd_render();

    while (1) 
    {
		stopLight();//chama sempre a função que controla o acendimento dos LEDs do semaforo
		readLDR2lux(&flag_500ms);//chama a função que converte o valor lido pelo LDR para lux
		Times();//atualiza os tempos e o numero de carros
		nokia_lcd_render();//mostra as informações atualizadas no Display
		boatSignal();//função que controla o sinal do navio
		luminary();
	}
}


void stopLight(){
	
	const uint16_t stopLightState[9] = {0b00000000, 0b00000001,0b00000010,0b00000011,0b00000100,0b00010101,0b00010110,0b00010111,0b00011000};
	//vetor que armazena os valores em binário que vao passando para apagar os LEDs do semáforo de acordo com a combinação logica das portas AND
	static uint32_t previous_TEMPO_MS = 0;//tempo anterior para ser usado na verificação do tempo que se passou
	static uint16_t i = 0;//variavel que vai ser usada como indice do vetor dos numeros binários, vai sendo incrementada no passo que o LED apaga
	
	PORTB = stopLightState[i];
		
	if(i<=3){
		if((tempo_ms - previous_TEMPO_MS)>=greenTime*250){
			i++;
			previous_TEMPO_MS += greenTime*250;
			
		}
	}
	else if(i==4){
		if((tempo_ms - previous_TEMPO_MS) >= yellowTime*1000){
			i++;
			previous_TEMPO_MS += yellowTime*1000;

		}
	}
	else if(i<=8){
		if((tempo_ms - previous_TEMPO_MS) >= redTime*250){
			i++;
			previous_TEMPO_MS += redTime*250;

		}
	}
	else if(i>8){
		i = 0;
		previous_TEMPO_MS = tempo_ms;//após um ciclo de semáforo (verde, amarelo, vermelho) a variavel vlta para 0, iniciando o ciclo de novo
	}
	luminary();//após atualizar os LEDs no semáforo, chama a função que acende ou apaga a luminaria
}

void Times(){
	
	sprintf(TimeGreen, "%u", greenTime);
	sprintf(TimeRed, "%u", redTime);		//esses 3 sprintf são responsáveis por converter em char, o valor numerico do tempo de cada sinal
	sprintf(TimeYellow, "%u", yellowTime);
	sprintf(carsLCD, "%u", carsPerMin);
	sprintf(luxStr, "%u", 1023000/ADC - 1000);//é feita a conversao para char do valor de lux lido
	
	nokia_lcd_set_cursor(33,5);
	if(mode)
		nokia_lcd_write_string("M",1);
	else
		nokia_lcd_write_string("A",1);
	nokia_lcd_set_cursor(33, 15);
	nokia_lcd_write_string(TimeGreen, 1);
	nokia_lcd_set_cursor(33, 25);
	nokia_lcd_write_string(TimeRed, 1);//aqui, imprime no LCD os valores atualizados do tempo
	nokia_lcd_set_cursor(33, 35);
	nokia_lcd_write_string(TimeYellow, 1);
	nokia_lcd_set_cursor(55,32);
	nokia_lcd_write_string("   ",1);
	nokia_lcd_set_cursor(55,32);
	nokia_lcd_write_string(carsLCD,1);
	nokia_lcd_set_cursor(55,5);
	nokia_lcd_write_string("    ",1);
	nokia_lcd_set_cursor(55,5);
	nokia_lcd_write_string(luxStr,1);//o valor de lux é impresso no LCD
	nokia_lcd_render();
	
	USART_Transmit((redTime - yellowTime));

	USART_Transmit(yellowTime);	

	USART_Transmit((yellowTime + greenTime));

	
}

void readLDR2lux(uint8_t *flag_500ms){
	
	if(*flag_500ms){//vai entrar quando a flag for 1, ou seja, se passaram 500ms
		LUX = 1023000/ADC - 1000;//converte o valor de LUX para o que é lido
		*flag_500ms = 0;//a flag é zerada, indicando que ja foi feito o procedimento necessário, e que ela só será 1 novamente após mais 500ms
	}
}

void luminary (){
	
	uint16_t thereAreCars = qtCars;
	
	if(flag_luz){
		if(people|| thereAreCars>0){//caso haja ou alguma pessoa ou algum carro
			
			OCR0B = 255;//luminária acende com brilho máximo
			people = 0;//as pessoas são zeradas para uma posterior verificação
			thereAreCars = 0;//a contagem de carros passando zera para nao ir acumulando

		}
		else{
			OCR0B = 77;//caso nao haja ninguem nem nenhum carro, a luminária brilha com 30% de intensidade
		}
	flag_luz = 0;
	}			
}

void USART_Transmit(uint8_t timeData){
	
		while(!(UCSR0A & (1<<UDRE0)));//garante a limpeza do registrador para transmitir o dado completo
		UDR0 = timeData;//envia o dado do tempo
	
}

void boatSignal(){
	 
	 if(flag_boat){//se o botao que alerta que vem navio for ativado ele seta a flag_boat em um
		 PORTB = 0b00000100;//semaforo fica em amarelo
		 _delay_ms(3000);//por 3 segundos 
		 PORTB = 0b00000101;//semáforo fica todo em vermelho
		 PORTB &= 0b11011111;
		 _delay_ms(3000);//sinal fica fechado por 3s enquanto a ponte sobe
		 PORTB |= 0b00100000;//a ponte desce
		 _delay_ms(3000);//durante 3s
		 flag_boat = 0;//flag que indica que tem um navio é zerada para que o trânsito volte a fluir normalmente
	
	 }
}
