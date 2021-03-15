#include <Keypad.h>
#include <RTClib.h>
#include <SDHT.h>
#include <lcdgfx.h>
//Objeto pantalla OLED
int8_t ResetPin =  -1;         //Cambiar por -1 si no hay pin de reset
int8_t Dir_Pantalla = 0x3C;   //Cambiar por 0x3C si esta direccion no funciona
DisplaySSD1306_128x64_I2C display(ResetPin, { - 1, Dir_Pantalla, -1, -1, 0});

//Objeto KEYPAD
const byte filas = 4;
const byte columnas = 4;
char teclas [filas][columnas] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte pinesFilas [filas] = {11, 10, 9, 8};//Definimos los pines correspondientes a las filas del teclado matricial
byte pinesColumnas [columnas] = {7, 6, 5, 4};//Definimos los pines correspondientes a las filas del teclado matricial
Keypad teclado = Keypad(makeKeymap(teclas), pinesFilas, pinesColumnas, filas, columnas); ///Crea el mapa del teclado

//Objeto RTC
RTC_DS1307 rtc;
DateTime hoy;

//Objeto DHT
#define pinsensorT 2////Definimos el puerto donde conectaremos el sensor de temperatura
#define TipoSensor DHT11/////Definimos el tipo de sensor que es
SDHT dht;

//Variables y pines
#define ldr_pin A2  //PIN 11
unsigned int ldr = 0;
const long A = 1000;     //Resistencia en oscuridad en KΩ
const int B = 15;        //Resistencia a la luz (10 Lux) en KΩ
const int Rc = 10;       //Resistencia calibracion en KΩ
int V;
int ilum;

char tecla_pulsada;
int horariego1_00[3];
int horariego2_00[3];
int horariego3_00[3];


char horariego1[6];
char horariego2[6];
char horariego3[6];

byte ano, mes, dia, hora, minuto, segundo;
char fecha[] = ("  :  :  ");
char calendario[] = ("  /  /20  ");

char opcion_seleccionada;
bool menu = true;
bool error = false;
bool bandera_riego = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  display.begin();
  rtc.begin();
  //rtc.adjust(DateTime(__DATE__, __TIME__));///ESTA LINEA SE TIENE QUE COMENTAR NADA MAS SE CARGUE UNA VEZ, Y VOLVER A CARGAR EL SKECTH PARA QUE DE ESTA MANERA SOLO SE PONGA EN HORA UNA VEZ A TRAVÉS DEL ORDENADOR
  //rtc.adjust(DateTime(2021, 02, 17, 19, 44, 00));
}

void loop() {
  if (menu) {
    menu_principal();
  }
  hora_de_regar();
  recuperar_fecha();
  dht.read(TipoSensor, pinsensorT);
  V = analogRead(ldr_pin);
  //t = dht.readTemperature();////En grados celsius por defecto
  hoy = rtc.now();///Recuperamos la fecha actual
  tecla_pulsada = teclado.getKey();//Comprobamos que tecla se ha pulsado
  opcion_seleccionada = tecla_pulsada; //La guardamos en una nueva variable
  //Tenemos varias opciones
  if (opcion_seleccionada == 'A') {
    programar_Horarios(horariego1_00, "A");
    opcion_seleccionada = '\0';
    error = false;
  }
  if (opcion_seleccionada == 'B') {
    programar_Horarios(horariego2_00, "B");
    opcion_seleccionada = '\0';
    error = false;
  }
  if (opcion_seleccionada == 'C') {
    programar_Horarios(horariego3_00, "C");
    opcion_seleccionada = '\0';
    error = false;
  }
  if (opcion_seleccionada == 'D') {
    opcion_seleccionada = '\0';
    ver_fecha();
  }
  if (opcion_seleccionada == '#') {
    opcion_seleccionada = '\0';
    opcion_sensado();
  }
}

//Aquí tenemos las distintas funciones en las que podemos programar la hora de riego

void programar_Horarios(int horario[3], char *opcion) {
  int horario_Ant[3] = {horario[0], horario[1], horario[2]};
  char msj[22] = "Opcion ";
  strcat(msj, opcion);
  strcat(msj, " seleccionada");
  display.fill(0x00);
  escribir_texto(0, 0, msj, 1);
  programar_H_M_S(horario);
  if (error) {
    horario[0] = horario_Ant[0];///En caso de que se produzca un error al introducir la hora, nos quedamos con la hora anteriormente programada
    horario[1] = horario_Ant[1];
    horario[2] = horario_Ant[2];
    return 0;
  }
  char HORA_FINAL[9];///Esta cadena de caracteres contendrá la hora final programada
  HORA_FINAL[0] = 0;//Iniciamos en 0
  char HORA_F[3];//Esta cadena la usaremos para ir guardando los números enteros correspondientes a las horas, minutos y segundos convertidos en caracteres
  HORA_F[0] = 0;///Iniciamos en 0
  for (int j = 0; j < 3; j++) {
    if (horario[j] < 10) {
      strcat(HORA_FINAL, "0");////En caso de que el número sea menor de 10, añadimos un 0 para que quede en formato: "09"
    }
    sprintf(HORA_F, "%i", horario[j]);////Función para convertir el entero horario[j] en una cadena de caracteres que se guarda en HORA_F. %i indica que el número a convertir es un entero
    strcat(HORA_FINAL, HORA_F);//Unimos las horas, minutos y segundos..
    if ( j < 2) {
      strcat(HORA_FINAL, ":");///..separando cada dos digitos por : para que el formato quede así 00:00:00 
    }
  }
  display.fill(0x00);///Limpiamos la pantalla
  escribir_texto(15, 0, "Se ha programado", 1);
  escribir_texto(9, 9, "la hora de riego :", 1);
  escribir_texto(40, 20, HORA_FINAL, 1);
  delay(2000);
  display.fill(0x00);
  menu = true;
}

void programar_H_M_S(int horario_temp[3]) {
  char Hora[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};///Iniciamos la variable en 0
  recolecta_datos(24, 0, 9, ":", Hora);
  if (!strcmp(Hora, "invalido")) {///Esta funcion compara caracter por caracter, y en caso de que sean iguales devuelve un 0
    Invalido();
    error = true;
    menu = true;
    return 0;
  }
  char Minutos[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};//Iniciamos la variable en 0
  recolecta_datos(60, 21, 9, ":", Minutos);
  if (!strcmp(Minutos, "invalido")) {
    Invalido();
    error = true;
    menu = true;
    return 0;
  }
  char Segundos[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};//Iniciamos la variable en 0
  recolecta_datos(60, 40, 9, " ", Segundos);
  if (!strcmp(Segundos, "invalido")) {
    Invalido();
    error = true;
    menu = true;
    return 0;
  }
  horario_temp[0] = atoi(Hora);///La función atoi(matriz) convierte una cadena de caracteres en un entero
  horario_temp[1] = atoi(Minutos);
  horario_temp[2] = atoi(Segundos);
}

void recolecta_datos(int limite, int C_X, int C_Y, char *separador, char buf[9]) {////La variable char * indica que no se puede modificar, ya que esto podría causar un comportamiento indefinido
  char T_pulsada[2];
  T_pulsada[1] = 0;//Iniciamos la variable en 0
  for (int j = 0; j < 2; j++) {
    T_pulsada[0] = teclado.waitForKey(); ////Esta función es importante, ya que hasta que no pulsemos una tecla, el micro se queda parado en está instrucción. La unica manera de avanzar es pulsar una tecla o salir de bucle for
    if (isDigit(T_pulsada[0]) && T_pulsada[0] != '*' && T_pulsada[0] != '#') {
      escribir_texto(C_X, C_Y, T_pulsada, 1);
      strcat(buf, T_pulsada);///Vamos concatenando uno a uno los dígitos pulsados
      C_X += 7;///Hacemos esto para que no se superpongan los caracteres al imprimirlo por pantalla
    } else {
      j = 2;
      strcpy(buf, "invalido");
      return 0;
    }
  }
  int numero = atoi(buf);///Convertimos la cadena de digitos en un entero con la función atoi
  if (numero >= limite) {///Comprobamos que no el número introducido no supere el límite
    strcpy(buf, "invalido");///Si lo ha superado, metemos en buf la palabra inválido
    return 0;
  } else {
    escribir_texto(C_X, C_Y, separador, 1);
  }
}

void Invalido() {
  display.fill(0x00);
  escribir_texto(15, 24, "Invalido", 2);
  delay(1000);
  display.fill(0x00);
}
void menu_principal() {
  display.fill( 0x00 );
  escribir_texto(40, 16, "Menu", 2);
  escribir_texto(10, 32, "Principal", 2);
  delay(1000);
  // Limpiamos la pantalla, cambiamos el tamaño del texto y resituamos el cursor para escribir el nuevo mensaje. Luego lo mandamos
  display.fill( 0x00 );
  escribir_texto(0, 0, "Pulse A, B y C para  cambiar las horas de riego 1, 2 y 3 respectivamente", 1);
  menu = false;
}

void recuperar_fecha(){
  hoy = rtc.now();///Recuperamos la fecha actual
  ano = hoy.year() % 100; // Eliminamos el siglo y queda el año con dos dígitos
  mes = hoy.month();
  dia = hoy.day();
  hora = hoy.hour();
  minuto = hoy.minute();
  segundo = hoy.second();

  fecha[7] = segundo % 10 + 48;
  fecha[6] = segundo / 10 + 48;
  fecha[4] = minuto % 10 + 48;
  fecha[3] = minuto / 10 + 48;
  fecha[1] = hora % 10 + 48;
  fecha[0] = hora / 10 + 48;

  calendario[9] = ano % 10 + 48;
  calendario[8] = ano / 10 + 48;
  calendario[4] = mes % 10 + 48;
  calendario[3] = mes / 10 + 48;
  calendario[1] = dia % 10 + 48;
  calendario[0] = dia / 10 + 48;

}

void ver_fecha() {

  display.fill(0x00);
  escribir_texto(15, 14, fecha, 2);
  escribir_texto(4, 35, calendario, 2);
  delay(2000);
  menu = true;
}

void hora_de_regar() {
  //A pesar de que cuando introducimos manualmente la hora de riego se piden la hora, los minutos y los segundos, en esta parte del código nos fijaremos solo en la hora y los minutos
  ///De esta manera si por casualidad estamos tocando el keypad justo a la hora de riego y nos encontramos con un delay, no perderemos el riego, si no que regaremos una vez pase el delay
  ///Y levantaremos una bandera para que solo se riegue una sola vez.

  //int hora_anterior;

  if (horariego1_00[0]==hora&&horariego1_00[1]==minuto) {
    /////ENCENDEMOS LA BOMBA DURANTE 10 segundos////////
   
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1000);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(1000);

  }

  ///////////////--------------------AQUÍ FALTA HACER QUE SE BAJE LA BANDERA DE REGADO AL MINUTO (P.E) DE HABER REGADO LA ÚLTIMA VEZ-------////////////////////////
  //////////////----------------------EL PROBLEMA ES QUE AL INICIARSE EL PROGRAMA, ENTRA DIRECTAMENTE EN EL PRIMER IF (CREO QUE SE DEBE A QUE AL INIARSE TODO ESTA A 0 Y TODO SE CUMPLE-----------///////////
  /////////////----------------------TENGO QUE INVESTIGARLO AUN UN POCO MAS-------------///////
    
  if (horariego2_00[0]==hora&&horariego2_00[1]==minuto) {
    /////ENCENDEMOS LA BOMBA DURANTE 10 segundos////////
  }
  if (horariego3_00[0]==hora&&horariego3_00[1]==minuto) {
    /////ENCENDEMOS LA BOMBA DURANTE 10 segundos////////
  }
}

void opcion_sensado() {
  char dht_temp[20]; dht_temp[0] = '\0';//Iniciamos las variables como una cadena vacía
  char tmp[6]; tmp[5] = '\0';
  char dht_hum[17]; dht_temp[0] = '\0';
  char hmd[6]; hmd[0] = '\0';
  strcpy(dht_temp, "Temperatura = ");//Añadimos la frase Temperatura = a nuestra cadena
  dtostrf((double(dht.celsius) / 10), 3, 2, tmp);///Función que sirve para convertir un float en un string. El 6 indica el número de enteros y el 2 el número de decimales
  strcat(dht_temp, tmp);///Concatenamos "Temperatura =" con el valor de la temperatura

  strcpy(dht_hum, " Humedad = ");
  dtostrf((double(dht.humidity) / 10), 5, 2, hmd);
  strcat(dht_hum, hmd);
  display.fill(0x00);
  escribir_texto(1, 0, dht_temp, 1);
  escribir_texto(1, 9, dht_hum, 1);

  char luxometro[5];
  ilum = ((long)V * A * 10) / ((long)B * Rc * (1024 - V));
  sprintf(luxometro, "%i", ilum);
  escribir_texto(0, 19, "La cantidad de luz es: ", 1);
  escribir_texto(23, 27, luxometro, 1);
  delay(2000);
  menu = true;
}

void escribir_texto(byte x_pos, byte y_pos, char *text, byte text_size) {
  display.setFixedFont(ssd1306xled_font6x8);///Seleccionamos una fuente
  if (text_size == 1) {
    display.printFixed (x_pos,  y_pos, text, STYLE_NORMAL);
  }
  else if (text_size == 2) {
    display.printFixedN (x_pos, y_pos, text, STYLE_NORMAL, FONT_SIZE_2X);
  }
}