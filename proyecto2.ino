#include <stdint.h>
#include <stdbool.h>
#include <TM4C123GH6PM.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

#include "bitmaps.h"
#include "font.h"
#include "lcd_registers.h"


//Pines LCD

#define RST PD_0
#define CS PD_1
#define RS PD_2
#define WR PD_3
#define RD PE_1

//Notas musicales

#define snd1 291
#define snd2 340
#define snd3 315

// Declaración de variables

int DPINS[] = {PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7};
int state = 0;
int start = 0;
int btn = 1;
int y = 0;
int coordx, coordy;
int fnty = 240;
int fntx = 0;
String game_over = "GAME OVER";
String title = "PAC-MAN RUN";
String st_txt = "START";
String pt = ".";
int soundPin = PF_1;
int tmp1 = 0;
int tmp2 = 0;
int t1 = 0;
int t2 = 0;
int jugador = 0;

//***************************************************************************************************************************************
// Funciones Prototipo
//***************************************************************************************************************************************
void LCD_Init(void);
void menu_principal(void);
void LCD_CMD(uint8_t cmd);
void LCD_DATA(uint8_t data);
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void LCD_Clear(unsigned int c);
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void LCD_Print(String text, int x, int y, int fontSize, int color, int background);
void mover_fantasma(void);
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]);
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[], int columns, int index, char flip, char offset);

//Declaración de variables para uso en la memoria Flash
extern uint8_t fantasma[];
//***************************************************************************************************************************************
// Inicializacion
//***************************************************************************************************************************************
void setup() {
  SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);//Configuración del reloj
  Serial.begin(9600);

  GPIOPadConfigSet(GPIO_PORTB_BASE, 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);
  randomSeed(analogRead(0));
  //Configuración de las entradas de los botones
  pinMode(PF_4, INPUT_PULLUP);
  pinMode(PF_0, INPUT_PULLUP);
  pinMode(soundPin, OUTPUT);
  randomSeed(183);
  LCD_Init();//Inicialización de la LCD
  LCD_Clear(0x00);
  delay(2000);
  menu_principal ();//Mostrar la pantalla principal
  beep(snd1, 500);
}

//***************************************************************************************************************************************
// Ciclo de proceso principal
//***************************************************************************************************************************************
void loop() {
  switch (state) {  //estado actual del juego
    case 0:    //Pantalla de inicio
      for (int x = 0; x < 42; x++) {
        btn = digitalRead(PF_4);
        delay(100);
        if (start == 1 && btn == 1 && state == 0) {    //Verificar boton ha sido presionado
          x = 0;
          state = 1;    //Pasa a siguiente estado
          start = 0;
        }

        int indx_pac = x % 2;
        
        if (state == 1) { //anima al pacman al empezar el juego
          y = 4 * x;
          LCD_Sprite(145 + y, 53, 32, 32, pacman, 2, indx_pac, 0, 0); //mueve a pacman al centro de la pantalla
          if ( y > 135) {
            x = 42;
            LCD_Clear(0x00);
        }
        }
        else {
          LCD_Sprite(145, 53, 32, 32, pacman, 6, indx_pac, 0, 0);
        }
        
        if (btn == 0 && start == 0) { //Antirrebote del boton para iniciar
          start = 1;
        }
      }
      break;

    
    case 1: //inicializacion el juego
      for (int x = 0; x < 72; x++) {
        y = 2 * x;
        delay(50);
      }
      state++;
      coordx = 144;
      coordy = 53;
      fntx = random(0, 268);  //Salida aleatoria para primer fantasma
      tmp1 = millis();
      if(jugador == 0){
        LCD_Print("JUGADOR 1", 300, 0, 2, 0xffff, 0x0000);
        }
      else {
        LCD_Print("JUGADOR 2", 300, 0, 2, 0xffff, 0x0000);
        } 
      break;

    
    case 2:   //Ya jugando en la pantalla
      for (int x = 0; x < 680; x++) {
        mover_fantasma();
        int  indx_pac = (x / 20) % 2;
        LCD_Sprite(coordx, coordy, 32, 32, pacman, 2, indx_pac, 0, 0);
        V_line(coordx - 1, coordy, 32, 0x00);
        btn = digitalRead(PF_4);

        if (state == 3) {
          if(jugador == 0){
            t1 = millis() - tmp1;
            }
          else{
            t2 = millis() - tmp1;
            }
          goto gameover;
        }
        //Sentencia para que se haga el movimiento mientras el botón este presionado
        while (btn == 0) {
          btn = digitalRead(PF_4);
          //Programación defensiva para que el satélite no salga de la pantalla
          if (coordx > 255) {
            coordx = 255;
          } else if (coordx < 1) {
            coordx = 1;
          }
          if (coordy > 208) {
            coordy = 208;
          } else if (coordy < 1) {
            coordy = 1;
          }
          mover_fantasma();
          //Casos para los movimientos del satélite
          switch ( indx_pac) {
            //Caso para que el satélite suba
            case 0:
              LCD_Sprite(coordx, coordy--, 32, 32, pacman, 2, indx_pac, 0, 0);
              V_line(coordx + 32, coordy, 32, 0x00);
              break;
            //Caso para que el satélite vaya a la esquina superior izquierda
            case 1:
              LCD_Sprite(coordx--, coordy--, 32, 32, pacman, 2, indx_pac, 0, 0);
              break;
            //Caso para que el satélite vaya a la izquierda
            case 2:
              LCD_Sprite(coordx--, coordy, 32, 32, pacman, 2, indx_pac, 0, 0);
              V_line(coordx + 32, coordy, 32, 0x00);
              break;
            //Caso para que el satélite a la esquina inferior izquierda
            case 3:
              LCD_Sprite(coordx--, coordy++, 32, 32, pacman, 2, indx_pac, 0, 0);
              H_line(coordx, coordy - 1, 32, 0x00);
              break;
            //Caso para que el satélite baje
            case 4:
              LCD_Sprite(coordx, coordy++, 32, 32, pacman, 2, indx_pac, 0, 0);
              break;
            //Caso para que el satélite vaya a la esquina inferior derecha
            case 5:
              LCD_Sprite(coordx++, coordy++, 32, 32, pacman, 2, indx_pac, 0, 0);
              V_line(coordx - 1, coordy, 32, 0x00);
              H_line(coordx, coordy - 1, 32, 0x00);
              break;
            //Caso para que el satélite vaya a la derecha
            case 6:
              LCD_Sprite(coordx++, coordy, 32, 32, pacman, 2, indx_pac, 0, 0);
              break;
            //Caso para que el satélite vaya a la esquina superior derecha
            case 7:
              LCD_Sprite(coordx++, coordy--, 32, 32, pacman, 2, indx_pac, 0, 0);
              V_line(coordx - 1, coordy, 32, 0x00);
              H_line(coordx, coordy + 32, 32, 0x00);
              break;
            //Caso por defecto
            default:
              break;
          }
        }
      }
gameover:
      break;
    //Caso de cuando se ha finalizado el juego
    case 3:
      LCD_Print(game_over, 80, 60, 2, 0xffff, 0x0000);
      if(jugador != 0){
        LCD_Print("JUGADOR 1", 80, 120, 2, 0xffff, 0x0000);
      }
      else{
        LCD_Print("JUGADOR 2", 80, 120, 2, 0xffff, 0x0000);
      }
      LCD_Print(st_txt, 60, 200, 2, 0xffff, 0x0000);
      
      fnty = 208;
      btn = digitalRead(PF_4);
      if (btn == 0) {
        if(jugador != 0){
            jugador == 1;
            LCD_Clear(0x00);
            LCD_Print("JUGADOR 2", 80, 60, 2, 0xffff, 0x0000);
            LCD_Print("LISTO!", 60, 200, 2, 0xffff, 0x0000);
            state = 1;

          }
        else{
            if (t1 > t2){
                LCD_Clear(0x00);
                LCD_Print("JUGADOR 1", 80, 60, 2, 0xffff, 0x0000);
                LCD_Print("GANA!", 60, 200, 2, 0xffff, 0x0000);
              }
            if (t1 < t2){
                LCD_Clear(0x00);
                LCD_Print("JUGADOR 2", 80, 60, 2, 0xffff, 0x0000);
                LCD_Print("GANA!", 60, 200, 2, 0xffff, 0x0000);
              }
            else{
                LCD_Clear(0x00);
                LCD_Print("EMPATE", 80, 60, 2, 0xffff, 0x0000);   
            }
            state = 0;
            menu_principal();
            delay(500);
      }
      break;
      }
  }
}

// Funcion para sonidos
void beep(int note, int duration) 
{
  tone(soundPin, note, duration / 2);
  delay(duration / 2);
  noTone(soundPin);
  delay(duration / 2 + 20);
}

// Función para menu
void menu_principal(void) { 
  LCD_Clear(0x00);

  LCD_Print(title, 100, 90, 2, 0xffff, 0x0000);
  LCD_Print(st_txt, 80, 150, 2, 0xffff, 0x0000);
  
}

// Función para que se muevan los fantasmas mostrados en pantalla
void mover_fantasma(void) { 
  if (fnty <= -32) {
    H_line(0, 0, 320, 0x00);
    fntx = random(0, 268);
    fnty = 240;
  }
  LCD_Sprite(fntx, fnty, 32, 32, fantasma, 1, 0, 0, 0);
  H_line(fntx, fnty + 32, 32, 0x00);
  fnty = fnty - 1;
  if (((fntx + 32) > coordx + 5) && ((fntx + 32) < coordx + 59)) {
    if (((fnty + 32) > coordy + 5) && ((fnty + 32) < coordy + 59)) {
      state = 3;
      beep(snd3, 100);
      beep(snd2, 100);
      beep(snd1, 100);
    }
  }
}


// Función para inicializar la LCD
void LCD_Init(void) {   
  pinMode(RST, OUTPUT);
  pinMode(CS, OUTPUT);
  pinMode(RS, OUTPUT);
  pinMode(WR, OUTPUT);
  pinMode(RD, OUTPUT);
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(DPINS[i], OUTPUT);
  }

  // Secuencia de inicialización
  digitalWrite(CS, HIGH);
  digitalWrite(RS, HIGH);
  digitalWrite(WR, HIGH);
  digitalWrite(RD, HIGH);
  digitalWrite(RST, HIGH);
  delay(5);
  digitalWrite(RST, LOW);
  delay(20);
  digitalWrite(RST, HIGH);
  delay(150);
  digitalWrite(CS, LOW);
  //****************************************
  LCD_CMD(0xE9);  // SETPANELRELATED
  LCD_DATA(0x20);
  //****************************************
  LCD_CMD(0x11); // Exit Sleep SLEEP OUT (SLPOUT)
  delay(100);
  //****************************************
  LCD_CMD(0xD1);    // (SETVCOM)
  LCD_DATA(0x00);
  LCD_DATA(0x71);
  LCD_DATA(0x19);
  //****************************************
  LCD_CMD(0xD0);   // (SETPOWER)
  LCD_DATA(0x07);
  LCD_DATA(0x01);
  LCD_DATA(0x08);
  //****************************************
  LCD_CMD(0x36);  // (MEMORYACCESS)
  LCD_DATA(0x40 | 0x80 | 0x20 | 0x08); // LCD_DATA(0x19);
  //****************************************
  LCD_CMD(0x3A); // Set_pixel_format (PIXELFORMAT)
  LCD_DATA(0x05); // color setings, 05h - 16bit pixel, 11h - 3bit pixel
  //****************************************
  LCD_CMD(0xC1);    // (POWERCONTROL2)
  LCD_DATA(0x10);
  LCD_DATA(0x10);
  LCD_DATA(0x02);
  LCD_DATA(0x02);
  //****************************************
  LCD_CMD(0xC0); // Set Default Gamma (POWERCONTROL1)
  LCD_DATA(0x00);
  LCD_DATA(0x35);
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x02);
  //****************************************
  LCD_CMD(0xC5); // Set Frame Rate (VCOMCONTROL1)
  LCD_DATA(0x04); // 72Hz
  //****************************************
  LCD_CMD(0xD2); // Power Settings  (SETPWRNORMAL)
  LCD_DATA(0x01);
  LCD_DATA(0x44);
  //****************************************
  LCD_CMD(0xC8); //Set Gamma  (GAMMASET)
  LCD_DATA(0x04);
  LCD_DATA(0x67);
  LCD_DATA(0x35);
  LCD_DATA(0x04);
  LCD_DATA(0x08);
  LCD_DATA(0x06);
  LCD_DATA(0x24);
  LCD_DATA(0x01);
  LCD_DATA(0x37);
  LCD_DATA(0x40);
  LCD_DATA(0x03);
  LCD_DATA(0x10);
  LCD_DATA(0x08);
  LCD_DATA(0x80);
  LCD_DATA(0x00);
  //****************************************
  LCD_CMD(0x2A); // Set_column_address 320px (CASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x3F);
  //****************************************
  LCD_CMD(0x2B); // Set_page_address 480px (PASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0xE0);
  //  LCD_DATA(0x8F);
  LCD_CMD(0x29); //display on
  LCD_CMD(0x2C); //display on

  LCD_CMD(ILI9341_INVOFF); //Invert Off
  delay(120);
  LCD_CMD(ILI9341_SLPOUT);    //Exit Sleep
  delay(120);
  LCD_CMD(ILI9341_DISPON);    //Display on
  digitalWrite(CS, HIGH);
}

// Función para enviar parámetros y comandos a la LCD
void LCD_CMD(uint8_t cmd) {
  digitalWrite(RS, LOW);
  digitalWrite(WR, LOW);
  GPIO_PORTB_DATA_R = cmd;
  digitalWrite(WR, HIGH);
}

// Función para enviar datos a la LCD
void LCD_DATA(uint8_t data) {
  digitalWrite(RS, HIGH);
  digitalWrite(WR, LOW);
  GPIO_PORTB_DATA_R = data;
  digitalWrite(WR, HIGH);
}

// Función para definir el rango en las que se trabajará
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) {
  LCD_CMD(0x2a); // Set_column_address 4 parameters
  LCD_DATA(x1 >> 8);
  LCD_DATA(x1);
  LCD_DATA(x2 >> 8);
  LCD_DATA(x2);
  LCD_CMD(0x2b); // Set_page_address 4 parameters
  LCD_DATA(y1 >> 8);
  LCD_DATA(y1);
  LCD_DATA(y2 >> 8);
  LCD_DATA(y2);
  LCD_CMD(0x2c); // Write_memory_start
}

// Función para borrar la pantalla
void LCD_Clear(unsigned int c) {
  unsigned int x, y;
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(RS, HIGH);
  digitalWrite(CS, LOW);
  SetWindows(0, 0, 319, 239); // 479, 319);
  for (x = 0; x < 320; x++)
    for (y = 0; y < 240; y++) {
      LCD_DATA(c >> 8);
      LCD_DATA(c);
    }
  digitalWrite(CS, HIGH);
}

// Función para dibujar una línea horizontal
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
  unsigned int i, j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(RS, HIGH);
  digitalWrite(CS, LOW);
  l = l + x;
  SetWindows(x, y, l, y);
  j = l;// * 2;
  for (i = 0; i < l; i++) {
    LCD_DATA(c >> 8);
    LCD_DATA(c);
  }
  digitalWrite(CS, HIGH);
}

// Función para dibujar una línea vertical - parámetros ( coordenada x, cordenada y, longitud, color)
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
  unsigned int i, j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(RS, HIGH);
  digitalWrite(CS, LOW);
  l = l + y;
  SetWindows(x, y, x, l);
  j = l; //* 2;
  for (i = 1; i <= j; i++) {
    LCD_DATA(c >> 8);
    LCD_DATA(c);
  }
  digitalWrite(CS, HIGH);
}

// Función para dibujar un rectángulo - parámetros ( coordenada x, cordenada y, ancho, alto, color)
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  H_line(x  , y  , w, c);
  H_line(x  , y + h, w, c);
  V_line(x  , y  , h, c);
  V_line(x + w, y  , h, c);
}

// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  unsigned int i;
  for (i = 0; i < h; i++) {
    H_line(x  , y  , w, c);
    H_line(x  , y + i, w, c);
  }
}

// Función para dibujar texto - parámetros ( texto, coordenada x, cordenada y, color, background)
void LCD_Print(String text, int x, int y, int fontSize, int color, int background) {
  int fontXSize ;
  int fontYSize ;

  if (fontSize == 1) {
    fontXSize = fontXSizeSmal ;
    fontYSize = fontYSizeSmal ;
  }
  if (fontSize == 2) {
    fontXSize = fontXSizeBig ;
    fontYSize = fontYSizeBig ;
  }

  char charInput ;
  int cLength = text.length();
  Serial.println(cLength, DEC);
  int charDec ;
  int c ;
  int charHex ;
  char char_array[cLength + 1];
  text.toCharArray(char_array, cLength + 1) ;
  for (int i = 0; i < cLength ; i++) {
    charInput = char_array[i];
    Serial.println(char_array[i]);
    charDec = int(charInput);
    digitalWrite(CS, LOW);
    SetWindows(x + (i * fontXSize), y, x + (i * fontXSize) + fontXSize - 1, y + fontYSize );
    long charHex1 ;
    for ( int n = 0 ; n < fontYSize ; n++ ) {
      if (fontSize == 1) {
        charHex1 = pgm_read_word_near(smallFont + ((charDec - 32) * fontYSize) + n);
      }
      if (fontSize == 2) {
        charHex1 = pgm_read_word_near(bigFont + ((charDec - 32) * fontYSize) + n);
      }
      for (int t = 1; t < fontXSize + 1 ; t++) {
        if (( charHex1 & (1 << (fontXSize - t))) > 0 ) {
          c = color ;
        } else {
          c = background ;
        }
        LCD_DATA(c >> 8);
        LCD_DATA(c);
      }
    }
    digitalWrite(CS, HIGH);
  }
}

// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]) {
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(RS, HIGH);
  digitalWrite(CS, LOW);

  unsigned int x2, y2;
  x2 = x + width;
  y2 = y + height;
  SetWindows(x, y, x2 - 1, y2 - 1);
  unsigned int k = 0;
  unsigned int i, j;

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k + 1]);
      //LCD_DATA(bitmap[k]);
      k = k + 2;
    }
  }
  digitalWrite(CS, HIGH);
}

// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[], int columns, int index, char flip, char offset) {
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(RS, HIGH);
  digitalWrite(CS, LOW);

  unsigned int x2, y2;
  x2 =   x + width;
  y2 =    y + height;
  SetWindows(x, y, x2 - 1, y2 - 1);
  int k = 0;
  int ancho = ((width * columns));
  if (flip) {
    for (int j = 0; j < height; j++) {
      k = (j * (ancho) + index * width - 1 - offset) * 2;
      k = k + width * 2;
      for (int i = 0; i < width; i++) {
        LCD_DATA(bitmap[k]);
        LCD_DATA(bitmap[k + 1]);
        k = k - 2;
      }
    }
  } else {
    for (int j = 0; j < height; j++) {
      k = (j * (ancho) + index * width + 1 + offset) * 2;
      for (int i = 0; i < width; i++) {
        LCD_DATA(bitmap[k]);
        LCD_DATA(bitmap[k + 1]);
        k = k + 2;
      }
    }


  }
  digitalWrite(CS, HIGH);
}
