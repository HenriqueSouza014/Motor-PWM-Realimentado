#include "TimerOne.h"
int SensorValue, contador, rpm;
int rpm_desejado = 0;
float pwmnece = 0;
float erro = 0;
int erro_print = 0;
#define MAX_BUFFER_SIZE 30

/* Flags globais para controle de processos da interrupcao */
int flag_check_command = 0;

/* Rotina auxiliar para comparacao de strings */
int str_cmp(char *s1, char *s2, int len) {
  /* Compare two strings up to length len. Return 1 if they are
      equal, and 0 otherwise.
  */
  int i;
  for (i = 0; i < len; i++) {
    if (s1[i] != s2[i]) return 0;
    if (s1[i] == '\0') return 1;
  }
  return 1;
}

/* Processo de bufferizacao. Caracteres recebidos sao armazenados em um buffer. Quando um caractere
    de fim de linha ('\n') e recebido, todos os caracteres do buffer sao processados simultaneamente.
*/

/* Buffer de dados recebidos */

typedef struct {
  char data[MAX_BUFFER_SIZE];
  unsigned int tam_buffer;
} serial_buffer;

serial_buffer Buffer;

void buffer_clean() {
  Buffer.tam_buffer = 0;
}

/* Adiciona caractere ao buffer */
int buffer_add(char c_in) {
  if (Buffer.tam_buffer < MAX_BUFFER_SIZE) {
    Buffer.data[Buffer.tam_buffer++] = c_in;
    return 1;
  }
  return 0;
}

/* Ao receber evento da UART */
void serialEvent() {
  char c;
  while (Serial.available()) {
    c = Serial.read();
    if (c == '\n') {
      buffer_add('\0'); /* Se recebeu um fim de linha, coloca um terminador de string no buffer */
      flag_check_command = 1;
    } else {
      buffer_add(c);
    }
  }
}

/* Configuração da serial e dos pinos da arduino utilizados no infravermelho e no motor
  O pino utilizado para o infravermelho foi o 2 */
void setup() {
  Serial.begin(9600);
  pinMode(2, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(2), infravermelho, RISING); /*Criacao da funcao de interrupcao do infravermelho. Essa interrupcao acontecera na subida do sinal digital*/
  Timer1.initialize(500000 * 2); /*criacao da funcao timer com periodo de 1 segundo*/
  Timer1.attachInterrupt(timer);
}
/* Sempre que a helice do motor passar entre os sensores infravermelho ela faz com que o sinal digital passe de 0 para 1. Quando ha essa subida, ocorre a interrupcao e o contador eh incrementado*/
void infravermelho() {
  contador++;
}

/* Nessa funcao ocorre a transformacao do RPM e o feedback para o controle de velocidade que leva o giros do motor para o RPM desejado.`Por ter duas helices multiplicamos 
o contador pela metade. A cada um segundo o contador eh zerado pela função timer, o que garante a correta leitura de rpm. O pino da Arduino responsável pelo PWM no motor
DC eh o 3*/
void timer() {
  rpm = contador * 30;   
  contador = 0;  
  erro = rpm_desejado - rpm;
  pwmnece = pwmnece + erro * 0.0055; /* através do erro entre o rpm requerido e o atual, há um incremento ou decremento do pwm, responsavel pela velocidade do motor*/
  analogWrite(3, pwmnece);   /* Aqui o valor de pwm é passado para o motor DC, fazendo com que a velocidade do mesmo aumente ou diminua*/ 
}

void loop() {

  char out_buffer[150];
  if (flag_check_command == 1) {
    sscanf(Buffer.data, "%d", &rpm_desejado); /*Leitura no terminal serial do RPM que o usario quer impor ao motor*/
    buffer_clean();
  }
  erro_print = (int)erro; /* Transformacao do erro de um float para um inteiro, para que ele possa ser imprimido*/
  sprintf(out_buffer, "RPM Atual: %-8d Diferenca RPM: %-8d \n", rpm, erro_print);
  Serial.println(out_buffer);
  delay(300);
}
