/* Copyright 2017, Eric Pernia.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * Date: 2017-04-14 
 */

/*==================[inlcusiones]============================================*/

#include "driverTecladoMatricial.h"   // <= su propio archivo de cabecera
#include "sapi.h"                     // <= Biblioteca sAPI

/*==================[definiciones y macros]==================================*/

typedef enum{
   ESCANEANDO_TECLADO,
   ANTIRREBOTE,
   TECLA_PRESIONADA
} mefTecladoMatricial_t;

/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

// Guarda la ultima tecla apretada
uint16_t key = 0;

/* Pines del teclado matricial */

// Pines conectados a las Filas --> Salidas (MODO = OUTPUT)
uint8_t keypadRowPins[4] = {
   RS232_TXD, // Row 0
   CAN_RD,    // R1
   CAN_TD,    // R2
   T_COL1     // R3
};

// Pines conectados a las Columnas --> Entradas con pull-up (MODO = INPUT_PULLUP)
uint8_t keypadColPins[4] = {
   T_FIL0,    // Column 0
   T_FIL3,    // C1
   T_FIL2,    // C2
   T_COL0     // C3
};


// Vector para mostrar tecla presionada por UART
uint16_t asciiKeypadKeys[16] = {
                                '1', '2', '3', 'A',
                                '4', '5', '6', 'B',
                                '7', '8', '9', 'C',
                                '-', '0', '#', 'D'
                               };

// Vector para mostrar tecla presionada en el display 7 segmentos
uint16_t keypadKeys[16] = {
                               1,    2,    3, 0x0a,
                               4,    5,    6, 0x0b,
                               7,    8,    9, 0x0c,
                            0x0e,    0, 0x0f, 0x0d
                          };


// Variable de estado de la MEF
mefTecladoMatricial_t estadoMefTecladoMatricial;
                        
                          
/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

void configurarTecladoMatricial( void );
void inicializarTecladoMatrical( void );
void actualizarTecladoMatrical( void );


/*==================[definiciones de funciones internas]=====================*/

/*==================[definiciones de funciones externas]=====================*/

void configurarTecladoMatricial( void ){
   
   uint8_t i = 0;
   
   // Configure Rows as Outputs
   for( i=0; i<4; i++ ){
      gpioConfig( keypadRowPins[i], GPIO_OUTPUT );
   }

   // Configure Columns as Inputs with pull-up resistors enable
   for( i=0; i<4; i++ ){
      gpioConfig( keypadColPins[i], GPIO_INPUT_PULLUP );
   }
}



void guardarEnBuffer( uint16_t tecla ){
key=asciiKeypadKeys[tecla];
}


void inicializarTecladoMatricial( void ){
   estadoMefTecladoMatricial = ESCANEANDO_TECLADO;
}

void actualizarTecladoMatrical( void ){
   
   static uint16_t r = 0; // Rows
   static uint16_t c = 0; // Columns
   
   static delay_t delayAntirebote;
   static bool_t flagEstadoTeclaPresionada = FALSE;
   static bool_t flagEstadoEscaneandoTeclado=FALSE;
   static bool_t flagEstadoAntirrebote=FALSE;
      
   bool_t fExit = FALSE;
   
 //  uartWriteByte(UART_USB,estadoMefTecladoMatricial+'0');
   switch(estadoMefTecladoMatricial){
      case ESCANEANDO_TECLADO:
         if(flagEstadoEscaneandoTeclado==FALSE){
           flagEstadoEscaneandoTeclado==TRUE;
           //gpioWrite(LEDR, ON);
           //Poner filas de scan en cero
           for( r=0; r<4; r++ ){
             gpioWrite( keypadRowPins[r], LOW );
           }           
         }
         
      
         
         // Chequear condiciones de transicion de estado
         // Chequear eTeclaPresionada todas las columnas buscando si hay alguna tecla presionada
         for( c=0; gpioRead( keypadColPins[c] ) && c<4 ; c++ );
         // Si leo un estado BAJO en una columna entonces puede haber una tecla presionada
         if( c<4 )
           estadoMefTecladoMatricial = ANTIRREBOTE;
         // Exit del estado
         if (estadoMefTecladoMatricial != ESCANEANDO_TECLADO){
           for( r=0; r<4; r++ ){
             gpioWrite( keypadRowPins[r], HIGH );
           }
           flagEstadoEscaneandoTeclado=FALSE;                      
           //gpioWrite(LEDR, OFF);
         }
      break;
      case ANTIRREBOTE:
         if(flagEstadoAntirrebote==FALSE){
           //Entry
           flagEstadoAntirrebote=TRUE;
           //gpioWrite(LEDG, ON);
           //Configurar delayAntirrebote
           delayConfig(&delayAntirebote, 50);
           // Escanear la fila y columna de la tecla presionada
           fExit=FALSE;
           for(r=0; !fExit && r<4;){
             gpioWrite( keypadRowPins[r],LOW);
             for(c=0;gpioRead(keypadColPins[c]) && c<4; c++);
             if(c<4)
               fExit=TRUE;
             else{
               gpioWrite( keypadRowPins[r],HIGH);
               r++;
             }   
           }
         }
         // El delay se incrementa solo!
         // Chequear condiciones de transicion de estado
         
         if( delayRead(&delayAntirebote) ){
            // Chequeo si sigue presionada la tecla
           if( r<4 && c<4 && !gpioRead( keypadColPins[c] ) ){
             estadoMefTecladoMatricial = TECLA_PRESIONADA;
           }else{
             estadoMefTecladoMatricial = ESCANEANDO_TECLADO;
           }             
         }      
         // Con un flag ejecuto una vez al salir del estado
         if(estadoMefTecladoMatricial != ANTIRREBOTE){
           //gpioWrite(LEDG, OFF);
           flagEstadoAntirrebote= FALSE;
         }
      break;
      case TECLA_PRESIONADA:
         if(flagEstadoTeclaPresionada==FALSE){
           flagEstadoTeclaPresionada=TRUE;
           //gpioWrite(LEDB, ON);
         }
         // Chequear condiciones de transicion de estado eTeclaLiberada        
         if( gpioRead( keypadColPins[c] ) == HIGH ){
           estadoMefTecladoMatricial = ESCANEANDO_TECLADO;  
         }        
         // Exit      
         if( estadoMefTecladoMatricial != TECLA_PRESIONADA ){
           //gpioWrite(LEDB, OFF);          
           flagEstadoTeclaPresionada = FALSE;
           // Con un flag ejecuto una vez al salir del estado
           guardarEnBuffer( r * 4 + c );
         }
      break;
         
      default:
         inicializarTecladoMatricial();
      break;
   }
}




/* Devuelve TRUE si hay alguna tecla presionada o FALSE (0) en caso contrario.
 * Si hay tecla presionada guarda el valor en la variable key.
 * El valor es un numero de indice entre 0 y 15 */
bool_t leerTecladoMatricial( void ){

   bool_t retVal = FALSE;

   uint16_t r = 0; // Rows
   uint16_t c = 0; // Columns

   // Poner todas las filas en estado BAJO
   for( r=0; r<4; r++ ){
	  gpioWrite( keypadRowPins[r], LOW );
   }

   // Chequear todas las columnas buscando si hay alguna tecla presionada
   for( c=0; c<4; c++ ){

      // Si leo un estado BAJO en una columna entonces puede haber una tecla presionada
      if( !gpioRead( keypadColPins[c] ) ){

         delay( 50 ); // Anti-rebotes de 50 ms

         // Poner todas las filas en estado ALTO excepto la primera
         for( r=1; r<4; r++ ){
            gpioWrite( keypadRowPins[r], HIGH );
         }

         // Buscar que tecla esta presionada
         for( r=0; r<4; r++ ){

            // Poner la Fila[r-1] en estado ALTO y la Fila[r] en estado BAJO
            if( r>0 ){ // Exceptua el indice negativo en el array
               gpioWrite( keypadRowPins[r-1], HIGH );
            }
            gpioWrite( keypadRowPins[r], LOW );

            // Chequear la Columna[c] en Fila[r] para buscar si la tecla esta presionada
            // Si dicha tecla esta presionada (en estado BAJO) entonces graba la tecla en
            // key y retorna TRUE
            if( !gpioRead( keypadColPins[c] ) ){
               retVal = TRUE;
               key = r * 4 + c;
               /*
                  Formula de las teclas de Teclado Matricial (Keypad)
                  de 4 filas (rows) * 5 columnas (columns)

                     c0 c1 c2 c3 c4
                  r0  0  1  2  3  4
                  r1  5  6  7  8  9   Si se presiona la tecla r[i] c[j]:
                  r2 10 11 12 13 14   valor = (i) * cantidadDeColumnas + (j)
                  r3 15 16 17 18 19
               */
               return retVal;
            }
         }

      }
   }
   return retVal;
}

/*==================[fin del archivo]========================================*/