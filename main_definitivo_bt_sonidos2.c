#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"
#include <stdio.h>
#include <string.h>
#include <imagenes.h>
#include "HAL_I2C.h"
#include "sensorlib2.h"
#include <math.h>

//*****************************************************************************
//
// System clock rate in Hz.
//
//*****************************************************************************
uint32_t RELOJ;


//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// Macros necesarios para pintar los bitmaps y para los sonidos. Obligatorio que sean globales
//

#define RGB565               7
#define BITMAPS              1
#define BITMAP_SOURCE(addr) ((1<<24)|(((addr)&1048575)<<0))
#define BITMAP_LAYOUT(format,linestride,height) ((7<<24)|(((format)&31)<<19)|(((linestride)&1023)<<9)|(((height)&511)<<0))
#define BITMAP_SIZE(filter,wrapx,wrapy,width,height) ((8<<24)|(((filter)&1)<<20)|(((wrapx)&1)<<19)|(((wrapy)&1)<<18)|(((width)&511)<<9)|(((height)&511)<<0))

#define BEGIN(prim) ((31<<24)|(((prim)&15)<<0))
#define END() ((33<<24))

// Sonidos
#define S_BELL 0x49
#define N_SI_8        83
#define S_TUBA     0x42
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// Variables globales del battleship
//
unsigned int CMD_Offset_G = 0;      // Offset necesario para escribir en la RAM de la pantalla

// Direccciones de memoria donde se va a escribir cada bitmap
uint32_t dir_agua, dir_mapaagua;
uint32_t dir_barco2, dir_barco2_180, dir_barco2_giro, dir_barco2_giro_180;
uint32_t dir_barco3, dir_barco3_180, dir_barco3_giro, dir_barco3_giro_180;
uint32_t dir_barco4, dir_barco4_180, dir_barco4_giro, dir_barco4_giro_180;

// Variables para acceder f�cilmente a las direcciones guardadas y las dimensiones
uint32_t dir2[4][4];
uint32_t dimbarcos[4][2];

int dibujaBolaHueco();

// Variables que guardan la posici�n y el giro de los barcos puestos
int cuadroxPuesto[4];
int cuadroyPuesto[4];
int giroPuesto[4];

// Cadena obtenida por puerto serie y el �ndice de escritura en la cadena
char cadenaleida[9];
int ind=0;
int auxManda=0;

int t_nota; //Variable de tiempo que controla los sonidos

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************



#define FREC 50 //Frecuencia en hercios del tren de pulsos: 50ms
volatile int Flag_ints=0;
unsigned long REG_TT[6];
const int32_t REG_CAL[6]= {CAL_DEFAULTS};


//Enum de estados
typedef enum {
    pedirPin = 0,
    acceso=1,
    pedirHora=2,
    paginaPrincipal=3,
    bolaHueco=4,
    battleship=5,

} Estados;

typedef enum {
    colocar = 0,
    validar=1,
    marcar=2,
    esperar=3,
    resultado=4,
    final=5,

} Estadosbattleship;


int estado=pedirPin;
int estadobattleship=colocar;

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// Funci�n que se usa para incrementar el offset de la RAM de la pantalla
//
unsigned int FT800_IncCMDOffset_G(unsigned int Current_Offset, byte Command_Size)
{
    unsigned int New_Offset;

    New_Offset = Current_Offset + Command_Size;

    if(New_Offset > 256000)
    {
        New_Offset = (New_Offset - 256000);
    }

    return New_Offset;
}
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// Funci�n de escritura de la RAM de la pantalla de 16 en 16 bits
//
uint32_t EscribeRam16_G(uint16_t dato)
{
    HAL_SPI_CSLow();
    uint32_t adress=RAM_G + CMD_Offset_G;
    FT800_SPI_SendAddressWR(RAM_G + CMD_Offset_G);          // Mandar pr�xima direcci�n disponible
    FT800_SPI_Write16(dato);                                // Escribir en la pr�xima posici�n disponible
    HAL_SPI_CSHigh();

    CMD_Offset_G = FT800_IncCMDOffset_G(CMD_Offset_G, 2);   // Desplazar el offset de la RAM
    return adress;
}
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// Funci�n para mandar el Bitmap completo a la memoria de la pantalla
// devuelve la primera posici�n en la que comenz� a escribir el bitmap
//
uint32_t ComJPEG(const uint16_t *jpg, const uint32_t longitud, int vuelta)
{

    // jpg es el bitmap completo
    // longitud es su dimensi�n total
    // vuelta=0 si se quiere escribir el bitmap normal y vuelta=1 si se quiere escribir del rev�s
    uint32_t i, adress;

    ComEsperaFin();

    if (vuelta==0){
        for (i = 0; i < longitud; i++)
        {
            if (i==0)
                adress=EscribeRam16_G(jpg[i]);
            else
                EscribeRam16_G(jpg[i]);
        }
    }

    if (vuelta==1){
        for (i = longitud; i > 0; i--)
        {
            if (i==longitud)
                adress=EscribeRam16_G(jpg[i]);
            else
                EscribeRam16_G(jpg[i]);
        }
    }
    return adress;

}
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// Funci�n para mostrar el bitmap por pantalla
//
void ComBITMAP(uint32_t direccion, int ancho_im, int largo_im, int escala, int filtro, int x, int y){

    // direccion es la posici�n donde comienza el bitmap que se quiere
    // ancho_im es el ancho real del bitmap
    // largo_im es el largo real del bitmap
    // escala=1 bitmap a tama�o real, escala=2 bitmap doble de grande ...
    // filtro=0 sin filtro, filtro=1 con un filtro que homogeneiza con un filtro bilineal el bitmap
    // x posici�n horixontal donde se mostrar�
    // y posici�n vertical donde se mostrar�

    int anchoreal=ancho_im,largoreal=largo_im;

    if (filtro>1) filtro=1;
    else if (filtro<0) filtro=0;

    if (escala<1) escala=1;

    EscribeRam32(CMD_BEGIN_BMP);
    EscribeRam32(BITMAP_SOURCE(direccion));

        if(escala!=0){
            anchoreal=escala*ancho_im;
            largoreal=escala*largoreal;

            EscribeRam32(CMD_LOADIDENTITY);
            EscribeRam32(CMD_SCALE);
            EscribeRam32(escala* 65536);
            EscribeRam32(escala* 65536);
            EscribeRam32(CMD_SETMATRIX);

        }


    EscribeRam32(BITMAP_LAYOUT(RGB565, ancho_im * 2, largo_im));
    EscribeRam32(BITMAP_SIZE(filtro, 0, 0, anchoreal, largoreal));
    ComVertex2ff(x,y);
    EscribeRam32(CMD_END);
}
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// Funci�n para mandar mensajes por puerto serie acordes al protocolo establecido
//
void Mandarmsg(char mensaje[]){
    int ind=0;

    //if(UARTCharsAvail(UART2_BASE)){
        for(ind=0; ind<=8; ind++){
            UARTCharPutNonBlocking(UART2_BASE, mensaje[ind]);
            UARTCharPutNonBlocking(UART0_BASE, '\n');           //Eliminar esto
            UARTCharPutNonBlocking(UART0_BASE, mensaje[ind]);   //Eliminar esto
     //   }

    }

}
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// Funci�n para leer mensajes por puerto serie acordes al protocolo establecido
// devuelve la cadena leida ya modificada
//
char *Leermsg(){

        //while(UARTCharsAvail(UART2_BASE)){                        //Descomentar
        while(UARTCharsAvail(UART0_BASE)){                          //Eliminar esto
            cadenaleida[ind]=UARTCharGetNonBlocking(UART0_BASE);

            //cadenaleida[ind]=UARTCharGetNonBlocking(UART2_BASE);  //Descomentar
            UARTCharPutNonBlocking(UART0_BASE, cadenaleida[ind]);
            //UARTCharPutNonBlocking(UART2_BASE, cadenaleida[ind]); //Descomentar
            //No era un mensaje
            if(cadenaleida[8]!='a' && ind==8){
                strncpy(cadenaleida,"ffffffffa", 9);
                ind=0;
            }

            // Estan puestos estados aleatorios, cambiar por los necesarios, el battleship se ha tomado que es el 2, no tocar
            if (cadenaleida[0]=='0' && estado!= paginaPrincipal && ind==8){
                strncpy(cadenaleida,"ffffffffa", 9);
                ind=0;

            }
            else if (cadenaleida[0]=='1' && estado!= acceso && ind==8){
                strncpy(cadenaleida,"ffffffffa", 9);
                ind=0;

            }
            else if (cadenaleida[0]=='2' && estado!= battleship && ind==8){
                strncpy(cadenaleida,"ffffffffa", 9);
                ind=0;

            }
            else if (cadenaleida[0]=='3' && estado!= pedirPin && ind==8){
                strncpy(cadenaleida,"ffffffffa", 9);
                ind=0;

            }
            else if (ind==8)
                ind=0;

            else
                ind++;

        }

    return cadenaleida;

}
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// Funci�n para detectar colisiones con otros barcos a la hora de colocar
// devuelve 0 si colisi�n o 1 si espacio libre
//
int Colision_barcoPoner(int barcoPuesto, int barco, int giro){
    // barcoPuesto puede ser 0, 1, 2 o 3 ya que se colocan 4 barcos, elegir con cual se eval�a la colisi�n (si no est� puesto no detectar� nada)
    // barco, misma codificaci�n que barcoPuesto, se indica qu� barco que quiere poner
    // giro indica en qu� orientaci�n se encuentra el barco que se quiere poner

    int i=0;
    int anchoPuesto, largoPuesto, ancho, largo;

    int cx = (POSX - 37) / 22;
    int cy = (POSY - 37) / 22;

    //Barco a poner en vertical
    if(giro%2==0){

        if(giroPuesto[barcoPuesto]%2==0){
            if (barcoPuesto==0){        //barco2
                anchoPuesto=1;
                largoPuesto=2;
            }
            else if (barcoPuesto==1 || barcoPuesto==2){        //barco3 o 3_0
                 anchoPuesto=1;
                 largoPuesto=3;
             }
        }
        else{
            if (barcoPuesto==0){        //barco2
                anchoPuesto=2;
                largoPuesto=1;
            }
            else if (barcoPuesto==1 || barcoPuesto==2){        //barco3 o 3_0
                 anchoPuesto=3;
                 largoPuesto=1;
             }
        }

        if (barco==1 || barco==2){        //barco3 o 3_0
             ancho=1;
             largo=3;
         }
        if (barco==3){        //barco4
             ancho=1;
             largo=4;
         }

        //Colisi�n giro par del que se pone
        if (    (cx==cuadroxPuesto[barcoPuesto] &&
                cy >= cuadroyPuesto[barcoPuesto]-largo+1 && cy <= cuadroyPuesto[barcoPuesto]+largoPuesto-1) &&
                giroPuesto[barcoPuesto]%2==0)       //Posicion barco puesto giro par
            i=0;

        else if ( (cx >= cuadroxPuesto[barcoPuesto] && cx <= cuadroxPuesto[barcoPuesto] + anchoPuesto-1) &&
                  (cy >= cuadroyPuesto[barcoPuesto]-largo+1 && cy <= cuadroyPuesto[barcoPuesto]) &&
                  giroPuesto[barcoPuesto] % 2 != 0 )     //Posicion barco puesto giro impar
            i=0;

        else if (cy > 8-largo+1)
            i=0;

        else i=1;
    }

    //Barco a poner en horizontal
    else if (giro%2!=0){
        if(giroPuesto[barcoPuesto]%2==0){
            if (barcoPuesto==0){        //barco2
                anchoPuesto=1;
                largoPuesto=2;
            }
            else if (barcoPuesto==1 || barcoPuesto==2){        //barco3 o 3_0
                 anchoPuesto=1;
                 largoPuesto=3;
             }
        }
        else{
            if (barcoPuesto==0){        //barco2
                anchoPuesto=2;
                largoPuesto=1;
            }
            else if (barcoPuesto==1 || barcoPuesto==2){        //barco3 o 3_0
                 anchoPuesto=3;
                 largoPuesto=1;
             }
        }

        if (barco==1 || barco==2){        //barco3 o 3_0
             ancho=3;
             largo=1;
         }
        if (barco==3){        //barco4
             ancho=4;
             largo=1;
         }
        //Colisi�n giro impar del que se pone
        if (    ((cx>=cuadroxPuesto[barcoPuesto]-ancho +1 && cx<=cuadroxPuesto[barcoPuesto] )&&
                cy >= cuadroyPuesto[barcoPuesto] && cy <= cuadroyPuesto[barcoPuesto]+largoPuesto-1) &&
                giroPuesto[barcoPuesto]%2==0)       //Posicion barco puesto giro par
            i=0;

        else if ( (cx >= cuadroxPuesto[barcoPuesto]-ancho+1 && cx <= cuadroxPuesto[barcoPuesto] + anchoPuesto-1) &&
                cy == cuadroyPuesto[barcoPuesto]  &&
                  giroPuesto[barcoPuesto] % 2 != 0 )       //Posicion barco puesto giro impar
            i=0;

        else if (cx > 8-ancho+1)
            i=0;
        else i=1;
    }

    return i;
}
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// Funci�n para detectar colisiones con barcos puestos
// devuelve 0 si colisi�n y 1 si libre
//
int Colisionbarcos(int barcoPuesto,int cadena1, int cadena2){
    // barcoPuesto, con qu� barco puesto se quiere evaluar la colisi�n (codificaci�n como en Colision_barcoPoner)
    // cadena1 es la componente x del cuadro marcado
    // cadena2 es la componente y del cuadro marcado

    int i=0;
    int anchoPuesto, largoPuesto;

    int cx = cadena1;
    int cy = cadena2;

        if(giroPuesto[barcoPuesto]%2==0){
            if (barcoPuesto==0){        //barco2
                anchoPuesto=1;
                largoPuesto=2;
            }
            else if (barcoPuesto==1 || barcoPuesto==2){        //barco3 o 3_0
                 anchoPuesto=1;
                 largoPuesto=3;
             }
            else if (barcoPuesto==3){        //barco3 o 3_0
                 anchoPuesto=1;
                 largoPuesto=4;
             }
        }
        else{
            if (barcoPuesto==0){        //barco2
                anchoPuesto=2;
                largoPuesto=1;
            }
            else if (barcoPuesto==1 || barcoPuesto==2){        //barco3 o 3_0
                 anchoPuesto=3;
                 largoPuesto=1;
             }
            else if (barcoPuesto==3){        //barco3 o 3_0
                 anchoPuesto=4;
                 largoPuesto=1;
             }
        }

        //Colisi�n giro impar del que se pone
        if (    ((cx>=cuadroxPuesto[barcoPuesto] && cx<=cuadroxPuesto[barcoPuesto]+anchoPuesto-1 )&&
                cy >= cuadroyPuesto[barcoPuesto] && cy <= cuadroyPuesto[barcoPuesto]+largoPuesto-1))       //Posicion barco puesto giro par
            i=0;

        else i=1;


    return i;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// Variables que necesitan inicializaci�n, aqu� o encima del main
//
    int i;
    int k;
    int j;
    int salir;
    int cuadrox= 37 +22*4, cuadroy= 37 +22*4, giro=0;
    int cuadrox2=37 + 22*4, cuadroy2=37 + 22*4, giro2=0, cuadrox3=37 + 22*4, cuadroy3=37 + 22*4, giro3=0, cuadrox3_0=37 + 22*4, cuadroy3_0=37 + 22*4, giro3_0=0, cuadrox4=37 + 22*4, cuadroy4=37 + 22*4, giro4=0;
    int confirmar, colisiones=0, colisiones2=0;
    int poscolisiones[12][2]={{0,0},
                              {0,0},
                              {0,0},
                              {0,0},
                              {0,0},
                              {0,0},
                              {0,0},
                              {0,0},
                              {0,0},
                              {0,0},
                              {0,0},
                              {0,0},
    };

    int poscolisiones2[12][2]={{0,0},
                               {0,0},
                               {0,0},
                               {0,0},
                               {0,0},
                               {0,0},
                               {0,0},
                               {0,0},
                               {0,0},
                               {0,0},
                               {0,0},
                               {0,0},
    };
    int condMismaColision=0, res=0, unacadena=0;
    int leido=1, reintento=0, Confpuls=0;
    char buffer[9];
    int bloqueoMsgReint=0;
    int aux1, aux2, aux3;
    int marcado;

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

int Battleship(){

    int salir=0;

    switch (estadobattleship){

    case colocar:   //Estado para colocar los barcos

        //Pintar mapa de agua
        ComBITMAP(dir_mapaagua, MAPAAGUA_WIDTH, MAPAAGUA_HEIGHT,1, 0, 37, 37);

        // Un if para que se pinten 4 barcos k de 0 a 4
        if (k==0){
            // Si no se pulsa en el mapa al menos una vez no se pinta el provisional
            if(POSX>37 && POSX<(37 + 22*9) && POSY>37 && POSY<(37 + 22*9))
                marcado=1;
            // Si no se pulsa colocar no se almacena la posici�n
            if(!confirmar){
                if(POSX>37 && POSX<(37 + 22*9) && POSY>37 && POSY<(37 + 22*9)){
                    cuadrox= (POSX-37)/22;
                    cuadroy= (POSY-37)/22;

                    //Colisiones con las paredes, s�lo aqu�, en el resto de barcos se incluye dentro de la funci�n de colisi�n
                    if(giro==0 || giro==2)
                        if (cuadroy>7)
                            cuadroy=7;
                    if(giro==1 || giro==3)
                        if (cuadrox>7)
                            cuadrox=7;
                }
                // Si no se pulsa en el recuadro se mantienen las posiciones
                else {cuadrox=cuadrox; cuadroy=cuadroy;}

                //Pintar barco provisional
                if (giro==0 || giro==2)
                    ComBITMAP(dir2[k][giro], dimbarcos[k][1], dimbarcos[k][0],1, 0, 37+22*cuadrox ,37+22*cuadroy );
                else
                    ComBITMAP(dir2[k][giro], dimbarcos[k][0], dimbarcos[k][1],1, 0, 37+22*cuadrox ,37+22*cuadroy );

            }
            // Guardar posiciones
            cuadrox2=cuadrox; cuadroy2=cuadroy; giro2=giro;
        }
        if (k==1){
            // Si no se pulsa en el mapa al menos una vez no se pinta el provisional
            if(POSX>37 && POSX<(37 + 22*9) && POSY>37 && POSY<(37 + 22*9))
                marcado=1;
            // Si no se pulsa colocar no se almacena la posici�n
            if(!confirmar){
                if(POSX>37 && POSX<(37 + 22*9) && POSY>37 && POSY<(37 + 22*9)){

                    // if de colisi�n con el barco anterior
                        if(Colision_barcoPoner(0,1,giro)==0){
                            cuadrox=cuadrox;
                            cuadroy=cuadroy;
                        }
                        else{
                            cuadrox=(POSX-37)/22;
                            cuadroy=(POSY-37)/22;
                        }
                }
                else {cuadrox=cuadrox; cuadroy=cuadroy;}

                //Pintar barco provisional

                if (giro==0 || giro==2)
                    ComBITMAP(dir2[k][giro], dimbarcos[k][1], dimbarcos[k][0],1, 0, 37+22*cuadrox ,37+22*cuadroy );
                else
                    ComBITMAP(dir2[k][giro], dimbarcos[k][0], dimbarcos[k][1],1, 0, 37+22*cuadrox ,37+22*cuadroy );

                // Pintar barcos ya puestos

                if (giro2==0 || giro2==2)
                    ComBITMAP(dir2[0][giro2], dimbarcos[0][1], dimbarcos[0][0],1, 0, 37+22*cuadrox2 ,37+22*cuadroy2 );
                else
                    ComBITMAP(dir2[0][giro2], dimbarcos[0][0], dimbarcos[0][1],1, 0, 37+22*cuadrox2 ,37+22*cuadroy2 );

            }
            // Guardar posiciones
            cuadrox3=cuadrox; cuadroy3=cuadroy; giro3=giro;

        }

        // hom�logo al barco anterior pero con m�s condiciones
        if (k==2){
            if(!confirmar){
                if(POSX>37 && POSX<(37 + 22*9) && POSY>37 && POSY<(37 + 22*9))
                    marcado=1;
                if(POSX>37 && POSX<(37 + 22*9) && POSY>37 && POSY<(37 + 22*9)){
                    if(Colision_barcoPoner(0,2,giro) && Colision_barcoPoner(1,2,giro)){
                        cuadrox=(POSX-37)/22;
                        cuadroy=(POSY-37)/22;

                    }
                    else{
                        cuadrox=cuadrox;
                        cuadroy=cuadroy;
                    }
                }
                else {cuadrox=cuadrox; cuadroy=cuadroy;}
                if (giro==0 || giro==2)
                    ComBITMAP(dir2[k][giro], dimbarcos[k][1], dimbarcos[k][0],1, 0, 37+22*cuadrox ,37+22*cuadroy );
                else
                    ComBITMAP(dir2[k][giro], dimbarcos[k][0], dimbarcos[k][1],1, 0, 37+22*cuadrox ,37+22*cuadroy );

                if (giro3==0 || giro3==2)
                    ComBITMAP(dir2[1][giro3], dimbarcos[1][1], dimbarcos[1][0],1, 0, 37+22*cuadrox3 ,37+22*cuadroy3 );
                else
                    ComBITMAP(dir2[1][giro3], dimbarcos[1][0], dimbarcos[1][1],1, 0, 37+22*cuadrox3 ,37+22*cuadroy3 );

                if (giro2==0 || giro2==2)
                    ComBITMAP(dir2[0][giro2], dimbarcos[0][1], dimbarcos[0][0],1, 0, 37+22*cuadrox2 ,37+22*cuadroy2 );
                else
                    ComBITMAP(dir2[0][giro2], dimbarcos[0][0], dimbarcos[0][1],1, 0, 37+22*cuadrox2 ,37+22*cuadroy2 );
            }
            cuadrox3_0=cuadrox; cuadroy3_0=cuadroy; giro3_0=giro;
        }
        if (k==3){
            if(POSX>37 && POSX<(37 + 22*9) && POSY>37 && POSY<(37 + 22*9))
                marcado=1;
            if(!confirmar){
                if(POSX>37 && POSX<(37 + 22*9) && POSY>37 && POSY<(37 + 22*9)){
                    if(Colision_barcoPoner(0,3,giro) && Colision_barcoPoner(1,3,giro) && Colision_barcoPoner(2,3,giro)){
                        cuadrox=(POSX-37)/22;
                        cuadroy=(POSY-37)/22;
                    }
                    else{
                        cuadrox=cuadrox;
                        cuadroy=cuadroy;
                    }
                }
                else {cuadrox=cuadrox; cuadroy=cuadroy;}
                if (giro==0 || giro==2)
                    ComBITMAP(dir2[k][giro], dimbarcos[k][1], dimbarcos[k][0],1, 0, 37+22*cuadrox ,37+22*cuadroy );
                else
                    ComBITMAP(dir2[k][giro], dimbarcos[k][0], dimbarcos[k][1],1, 0, 37+22*cuadrox ,37+22*cuadroy );

                if (giro3_0==0 || giro3_0==2)
                    ComBITMAP(dir2[2][giro3_0], dimbarcos[2][1], dimbarcos[2][0],1, 0, 37+22*cuadrox3_0 ,37+22*cuadroy3_0 );
                else
                    ComBITMAP(dir2[2][giro3_0], dimbarcos[2][0], dimbarcos[2][1],1, 0, 37+22*cuadrox3_0 ,37+22*cuadroy3_0 );

                if (giro3==0 || giro3==2)
                    ComBITMAP(dir2[1][giro3], dimbarcos[1][1], dimbarcos[1][0],1, 0, 37+22*cuadrox3 ,37+22*cuadroy3 );
                else
                    ComBITMAP(dir2[1][giro3], dimbarcos[1][0], dimbarcos[1][1],1, 0, 37+22*cuadrox3 ,37+22*cuadroy3 );

                if (giro2==0 || giro2==2)
                    ComBITMAP(dir2[0][giro2], dimbarcos[0][1], dimbarcos[0][0],1, 0, 37+22*cuadrox2 ,37+22*cuadroy2 );
                else
                    ComBITMAP(dir2[0][giro2], dimbarcos[0][0], dimbarcos[0][1],1, 0, 37+22*cuadrox2 ,37+22*cuadroy2 );
            }
            cuadrox4=cuadrox; cuadroy4=cuadroy; giro4=giro;
        }


        ComColor(0,0,0);
        ComTXT(290,25,29,0,"Pulse en el");
        ComTXT(305,60,29,0,"recuadro");

        // Bot�n de giro sin rebote
        ComColor(255,255,255);
        ComFgcolor(80,80,200);
        if (Boton(325,100,50,50,29,"Giro") && aux2==0)
            aux2=1;
        else if (!Boton(325,100,50,50,29,"Giro") && aux2==1){
            giro++;
            if(giro==4)
                giro=0;
            aux2=0;
        }

        // Bot�n de giro sin rebote bloqueado si no se ha pulsado una vez en la pantalla
        if (marcado==0){
            ComButton(300,170,100,50,29,OPT_FLAT,"Colocar");
            confirmar=0;
        }
        else if (Boton(300,170,100,50,29,"Colocar") && aux1==0)
            aux1=1;
        else if (!Boton(300,170,100,50,29,"Colocar") && aux1==1){
            // Si se pulsa se aumenta k y si se llega a los 4 se pasa de estado
            k++;
            if(k==4)
                estadobattleship=validar;
            // inicializaci�n de variables para otros estados
            confirmar=1;
            aux1=0;
            giro=0;
            cuadrox=37 + 22*4;
            cuadroy=37 + 22*4;
            marcado=0;
            t_nota=0;

        }

        if (confirmar==1 && k<4){
            t_nota++; //Aumentar el tiempo de la nota

            if(t_nota<50)
                TocaNota(S_PIANO, N_SOL);
            else
                FinNota();
        }

        // Marco del agua
        ComColor(200,200,80);
        ComRect(33,33,238,238,0);
        ComRect(30,30,242,242,0);
        ComRect(27,27,245,245,0);

        break;

    case validar:
        //Pintar mapa de agua
        ComBITMAP(dir_mapaagua, MAPAAGUA_WIDTH, MAPAAGUA_HEIGHT,1, 0, 37, 37);

        //Pintar todos los barcos puestos
        if (giro4==0 || giro4==2)
            ComBITMAP(dir2[3][giro4], dimbarcos[3][1], dimbarcos[3][0],1, 0, 37+22*cuadrox4 ,37+22*cuadroy4 );
        else
            ComBITMAP(dir2[3][giro4], dimbarcos[3][0], dimbarcos[3][1],1, 0, 37+22*cuadrox4 ,37+22*cuadroy4 );

        if (giro3_0==0 || giro3_0==2)
             ComBITMAP(dir2[2][giro3_0], dimbarcos[2][1], dimbarcos[2][0],1, 0, 37+22*cuadrox3_0 ,37+22*cuadroy3_0 );
         else
             ComBITMAP(dir2[2][giro3_0], dimbarcos[2][0], dimbarcos[2][1],1, 0, 37+22*cuadrox3_0 ,37+22*cuadroy3_0 );

         if (giro3==0 || giro3==2)
             ComBITMAP(dir2[1][giro3], dimbarcos[1][1], dimbarcos[1][0],1, 0, 37+22*cuadrox3 ,37+22*cuadroy3 );
         else
             ComBITMAP(dir2[1][giro3], dimbarcos[1][0], dimbarcos[1][1],1, 0, 37+22*cuadrox3 ,37+22*cuadroy3 );

         if (giro2==0 || giro2==2)
             ComBITMAP(dir2[0][giro2], dimbarcos[0][1], dimbarcos[0][0],1, 0, 37+22*cuadrox2 ,37+22*cuadroy2 );
         else
             ComBITMAP(dir2[0][giro2], dimbarcos[0][0], dimbarcos[0][1],1, 0, 37+22*cuadrox2 ,37+22*cuadroy2 );

         //Manda mensaje de comunicaci�n
         Leermsg();

         //Si le llega mensaje de comunicaci�n manda un 0 al final
         if(cadenaleida[0]=='2' && cadenaleida[7]=='1' && ind==0){
             Mandarmsg("21111110a");
         }
         //Si le llega mensaje de comunicaci�n recibida manda un 2 y pasa de estado
         else if (cadenaleida[0]=='2' && cadenaleida[7]=='0' && ind==0){
             estadobattleship=esperar;
             Mandarmsg("21111112a");
             // inicializaci�n de variables para otros estados
             unacadena=0;
         }
         //Si le llega mensaje de pasar estado, el otro ya est� esperando a que marques, pasa de estado
         else if (cadenaleida[0]=='2' && cadenaleida[7]=='2' && ind==0){
             estadobattleship=marcar;
             // inicializaci�n de variables para otros estados
             bloqueoMsgReint=0;
         }
         // Si no se recibe nada se manda continuamente el mensaje de comunicaci�n
         else{
             if(auxManda==0){
             Mandarmsg("21111111a");auxManda=1;}
             ComColor(0,0,0);
             ComTXT(280,110,29,0,"Comunicando...");
         }

         // Marco del agua
         ComColor(200,200,80);
         ComRect(33,33,238,238,0);
         ComRect(30,30,242,242,0);
         ComRect(27,27,245,245,0);


        break;

    case marcar:
        Leermsg();

        // Pinatr mapa del agua m�s oscuro
        ComColor(255,0,255);
        ComBITMAP(dir_mapaagua, MAPAAGUA_WIDTH, MAPAAGUA_HEIGHT,1, 0, 37, 37);

        // Pintar colisiones anteriores en rojo
        ComColor(200,0,0);
        for(k=0;k<colisiones; k++){
            ComBITMAP(dir_agua, AGUA_WIDTH, AGUA_HEIGHT,1, 0, 37 + 22*poscolisiones[k][0], 37 + 22*poscolisiones[k][1]);
        }

        // Pintar en verde el cuadro que se haya pulsado
        ComColor(0,255,0);
        if(POSX>37 && POSX<(37 + 22*9) && POSY>37 && POSY<(37 + 22*9)){
            marcado=1;
            cuadrox=(POSX-37)/22;
            cuadroy=(POSY-37)/22;
        }
        // Si no se pulsa en el recuadro se mantiene la posici�n
        else{
            cuadrox=cuadrox;
            cuadroy=cuadroy;
        }
        // Pintar cuadro provisional
        ComBITMAP(dir_agua, AGUA_WIDTH, AGUA_HEIGHT,1, 0, 37 + 22*cuadrox, 37 + 22*cuadroy);

        // Bot�n de confirmar el cuadro
        ComColor(255,255,255);
        ComFgcolor(80,80,200);
        //Se bloquea si se ha pulsado una vez o si no se ha pulsado nunca en el mapa
        if (marcado==0 || Confpuls==1){
            ComButton(300,150,110,50,29,OPT_FLAT,"Confirmar");
        }
        else if (Boton(300,150,110,50,29,"Confirmar") && aux1==0)
            aux1=1;
        else if (!Boton(300,150,110,50,29,"Confirmar") && aux1==1){
            // Manda tras el bit de estado la posici�n del cuadro
            sprintf(buffer, "2%d%d00011a", cuadrox, cuadroy);
            Mandarmsg(buffer);
            // Manda tras la posici�n un mensaje basura para no recibir infinitos mensajes del otro micro
            //Mandarmsg("20000000a");
            // actualizaci�n de variables
            aux1=0;
            leido=0;
            Confpuls=1;
            unacadena=0;
            // Si se pulsa tras un mensaje de reintento se bloquea el reintento y se esperan s�lo cuadros v�lidos
            if (reintento==1){
                bloqueoMsgReint=1;
                reintento=0;
            }
        }

        // Si el cuadro marcado es una posici�n que se ha marcado antes y ha colisionado vuelve a dar la opci�n de marcar
        // se muestra mensaje
        if (reintento==1){
            ComColor(0,0,0);
            ComTXT(260,45,29,0,"Ya ha pulsado ahi,");
            ComTXT(260,80,29,0,"pulse otro cuadro");
            Confpuls=0;
        }
        // si ya se ha pulsado el bot�n se bloquea esperando respuesta
        if (Confpuls==1){
            ComColor(0,0,0);
            ComTXT(300,43,29,0,"Esperando");
            ComTXT(305,80,29,0,"respuesta");
        }

        // S�lo analiza la cadena leida si se ha pulsado previamente confirmar
        if (leido==0){
            // Se ha recibido una colisi�n, se pasa de estado y se actualizan variables
            if (cadenaleida[7]=='0' && cadenaleida[6]=='0' && ind==0){
                colisiones++;
                estadobattleship=resultado;
                poscolisiones[colisiones-1][0]=cuadrox;
                poscolisiones[colisiones-1][1]=cuadroy;
                reintento=0;
                Confpuls=0;
                res=1;
                leido=1;
                t_nota=0;
                marcado=0;
                // Si se han realizado las 12 colisiones posibles se pasa al �ltimo estado y manda mensaje de perdedor al otro micro
                if(colisiones==12){
                    estadobattleship=final;
                    Mandarmsg("20000000a");
                    Confpuls=0;
                }

            }
            // Se ha recibido agua, se pasa de estado y se actualizan variables
            else if (cadenaleida[7]=='2' && cadenaleida[6]=='2' && ind==0){
                leido=1;
                estadobattleship=resultado;
                reintento=0;
                Confpuls=0;
                res=0;
                t_nota=0;
                marcado=0;
            }
            // Se ha recibido una colisi�n ya marcada se da una nueva oportunidad y se actualizan variables
            else if (cadenaleida[7]=='3' && cadenaleida[6]=='3' && ind==0 && bloqueoMsgReint==0){
                reintento=1;
                leido=1;
                Confpuls=0;
                aux1=0;
            }

        }

        // Marco del agua
        ComColor(200,200,80);
        ComRect(33,33,238,238,0);
        ComRect(30,30,242,242,0);
        ComRect(27,27,245,245,0);


        break;

    case resultado:
        // Pintar mapa de agua como en marcar
        ComColor(255,0,255);
        ComBITMAP(dir_mapaagua, MAPAAGUA_WIDTH, MAPAAGUA_HEIGHT,1, 0, 37, 37);

        // Pintar cuadro marcado previamente
        ComColor(0,255,255);
        ComBITMAP(dir_agua, AGUA_WIDTH, AGUA_HEIGHT,1, 0, 37 + 22*cuadrox, 37 + 22*cuadroy);

        // Pintar todas las colisiones
        ComColor(200,0,0);
        for(k=0;k<colisiones; k++){
            ComBITMAP(dir_agua, AGUA_WIDTH, AGUA_HEIGHT,1, 0, 37 + 22*poscolisiones[k][0], 37 + 22*poscolisiones[k][1]);
        }

        // Si fue una colisi�n se muestra un mensaje con el n�mero total de colisiones
        if (res==1){
            ComColor(0,0,0);
            sprintf(buffer, "%da Colision!!", colisiones);
            ComTXT(300,180,29,0,buffer);

            t_nota++; //Aumentar el tiempo de la nota

            if(t_nota<50)
                TocaNota(S_BELL, N_SI_8);
            else
                FinNota();
        }
        // Si fue agua se manda un mensaje de agua
        else if(res==0){
            ComColor(0,0,0);
            ComTXT(325,180,29,0,"Agua...");

            t_nota++; //Aumentar el tiempo de la nota

            if(t_nota<50)
                TocaNota(S_TUBA, N_DO);
            else
                FinNota();
        }

        // Bot�n de pasar turno
        ComColor(255,255,255);
        ComFgcolor(80,80,200);
        if (Boton(300,45,130,50,29,"Pasar turno") && aux1==0)
            aux1=1;
        else if (!Boton(300,45,130,50,29,"Pasar turno") && aux1==1){
            // Pasar de estado, actualizar variables mandar mensaje para que el otro micro pase de estado tambi�n
            estadobattleship=esperar;
            char buffer[9];
            sprintf(buffer, "2%d%d00001a", cuadrox, cuadroy);
            Mandarmsg(buffer);
            cuadrox=-1;
            cuadroy=-1;
            unacadena=0;
        }

        // Marco del agua
        ComColor(200,200,80);
        ComRect(33,33,238,238,0);
        ComRect(30,30,242,242,0);
        ComRect(27,27,245,245,0);
        break;

    case esperar:
        Leermsg();

        // Si se recibe un mensaje con todo unos, el otro micro ha ganado, se pasa al estado final
        if(cadenaleida[7]=='0' && cadenaleida[6]=='0' && cadenaleida[5]=='0' && cadenaleida[4]=='0' && cadenaleida[3]=='0' && cadenaleida[2]=='0' && cadenaleida[1]=='0' && ind==0)
            estadobattleship=final;

        // Pintar mapa de agua
        ComBITMAP(dir_mapaagua, MAPAAGUA_WIDTH, MAPAAGUA_HEIGHT,1, 0, 37, 37);

        // Pintar todos los barcos puestos
        if (giro4==0 || giro4==2)
            ComBITMAP(dir2[3][giro4], dimbarcos[3][1], dimbarcos[3][0],1, 0, 37+22*cuadrox4 ,37+22*cuadroy4 );
        else
            ComBITMAP(dir2[3][giro4], dimbarcos[3][0], dimbarcos[3][1],1, 0, 37+22*cuadrox4 ,37+22*cuadroy4 );

        if (giro3_0==0 || giro3_0==2)
             ComBITMAP(dir2[2][giro3_0], dimbarcos[2][1], dimbarcos[2][0],1, 0, 37+22*cuadrox3_0 ,37+22*cuadroy3_0 );
         else
             ComBITMAP(dir2[2][giro3_0], dimbarcos[2][0], dimbarcos[2][1],1, 0, 37+22*cuadrox3_0 ,37+22*cuadroy3_0 );

         if (giro3==0 || giro3==2)
             ComBITMAP(dir2[1][giro3], dimbarcos[1][1], dimbarcos[1][0],1, 0, 37+22*cuadrox3 ,37+22*cuadroy3 );
         else
             ComBITMAP(dir2[1][giro3], dimbarcos[1][0], dimbarcos[1][1],1, 0, 37+22*cuadrox3 ,37+22*cuadroy3 );

         if (giro2==0 || giro2==2)
             ComBITMAP(dir2[0][giro2], dimbarcos[0][1], dimbarcos[0][0],1, 0, 37+22*cuadrox2 ,37+22*cuadroy2 );
         else
             ComBITMAP(dir2[0][giro2], dimbarcos[0][0], dimbarcos[0][1],1, 0, 37+22*cuadrox2 ,37+22*cuadroy2 );

         // Si se recibe una cadena con posici�n de cuadro, s�lo se eval�a una sola cadena auqnue se manden varias a no ser que sea una colisi�n previa
         if (cadenaleida[7]=='1' && cadenaleida[6]=='1' && ind==0 && unacadena==0){
             // Pasar las posiciones a enteros
             cuadrox=cadenaleida[1]-'0';
             cuadroy=cadenaleida[2]-'0';

             // Comparar con colisiones previas
             for(k=0;k<colisiones2; k++){
                 if (poscolisiones2[k][0]==cuadrox && poscolisiones2[k][1]==cuadroy)
                     condMismaColision++;
             }

             // Comprobar colisi�n con barcos si no es una colisi�n previa
             if(!(Colisionbarcos(0,cuadrox,cuadroy) && Colisionbarcos(1,cuadrox,cuadroy) &&
                Colisionbarcos(2,cuadrox,cuadroy) && Colisionbarcos(3,cuadrox,cuadroy)) &&
                condMismaColision==0){
                 // Si lo es se actualizan las colisiones, se almacena la posici�n y se manda mensaje de colisi�n
                 colisiones2++;

                 poscolisiones2[colisiones2-1][0]=cuadrox;
                 poscolisiones2[colisiones2-1][1]=cuadroy;
                 Mandarmsg("21111100a");

             }
             // Se manda mensaje de colisi�n previa
             else if (condMismaColision>0)
                 Mandarmsg("21111133a");
             // Se manda mensaje de agua
             else
                 Mandarmsg("21111122a");

             //Bloquear la lectura de m�s cadenas
             unacadena=1;
         }
         // Si se recibe paso de turno se cambia de estado y se actualizan variables
         else if (cadenaleida[7]=='1' && cadenaleida[6]=='0' && ind==0){
             Confpuls=0;
             aux1=0;
             cuadrox=1000;
             cuadroy=1000;
             estadobattleship=marcar;
             bloqueoMsgReint=0;
         }

         // Si se ha recibido una cadena se muestra qu� se ha marcado
         if (unacadena==1){
             ComColor(200,0,200);
             ComBITMAP(dir_agua, AGUA_WIDTH, AGUA_HEIGHT,1, 0, 37 + 22*cuadrox, 37 + 22*cuadroy);
         }

         // Se pintan las colisiones previas en rojo
         for(k=0;k<colisiones2; k++){
             ComColor(200,0,0);
             ComBITMAP(dir_agua, AGUA_WIDTH, AGUA_HEIGHT,1, 0, 37 + 22*poscolisiones2[k][0], 37 + 22*poscolisiones2[k][1]);
         }

         // Marco del mapa de agua
         ComColor(200,200,80);
         ComRect(33,33,238,238,0);
         ComRect(30,30,242,242,0);
         ComRect(27,27,245,245,0);

         ComColor(0,0,0);
         ComTXT(300,90,29,0,"Turno del");
         ComTXT(300,125,29,0,"oponente");

        break;

    case final:
        // Mapa de agua como en marcar
        ComColor(255,0,255);
        ComBITMAP(dir_mapaagua, MAPAAGUA_WIDTH, MAPAAGUA_HEIGHT,1, 0, 37, 37);

        // Se muestran las colisiones obtenidas en rojo
        ComColor(200,0,0);
        if(colisiones>0){
            for(k=0;k<colisiones; k++){
                ComBITMAP(dir_agua, AGUA_WIDTH, AGUA_HEIGHT,1, 0, 37 + 22*poscolisiones[k-1][0], 37 + 22*poscolisiones[k-1][1]);
            }
        }

        // Si se tienen todas las colisiones se gana, sino se pierde
        if(colisiones==12){
            ComColor(0,0,0);
            ComTXT(295,125,29,0,"Ganador!!!");
        }
        else{
            ComColor(0,0,0);
            ComTXT(295,125,29,0,"Perdedor :(");
        }

        // Marco del mapa de agua
        ComColor(200,200,80);
        ComRect(33,33,238,238,0);
        ComRect(30,30,242,242,0);
        ComRect(27,27,245,245,0);

        break;


    }

    ComColor(0,0,0);
    ComFgcolor(255,0,0);
    if (Boton(HSIZE-50,10,40,30,29,"X") && aux3==0)
        aux3=1;
    else if (!Boton(HSIZE-50,10,40,30,29,"X") && aux3==1){
        salir=1;
        aux3=0;
        i=0;
        k=0;
        j=0;
        cuadrox= 37 +22*4; cuadroy= 37 +22*4; giro=0;
        confirmar=0; colisiones=0; colisiones2=0;
        condMismaColision=0; res=0; unacadena=0;
        leido=1; reintento=0; Confpuls=0;
        bloqueoMsgReint=0;
        aux1=0; aux2=0;
        marcado=0;
        t_nota=0;
        estadobattleship=colocar;
    }

    return salir;
}

void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}


//Enum de estados
typedef enum {
    reposo = 0,
    pulso=1,
    depulso=2,

} EstadosLectura;
int estadoLecturaNumerosPantalla=reposo;

int aux[5];
int cont=0;
char auxc[4][2];
int pin[] = {1,2,3,4};
int numeroLeidoPantalla[50];
int hora=0,minuto=0,segundo=0,milisegundo=0;
int auxLectura=0;
int parpadeo,parpadeo2;
float lux;
int lux_i;



// BMI160/BMM150
int8_t returnValue;
struct bmi160_gyro_t        s_gyroXYZ;
struct bmi160_accel_t       s_accelXYZ;
struct bmi160_mag_xyz_s32_t s_magcompXYZ;
//Calibration off-sets
int8_t accel_off_x;
int8_t accel_off_y;
int8_t accel_off_z;
int16_t gyro_off_x;
int16_t gyro_off_y;
int16_t gyro_off_z;
bool BME_on = true;
uint8_t Bmi_OK;
int DevID=0;
char string[80];
char msg;
//---------BolaHueco-----------//
int dibujaBolaHueco();
int xHueco;
int yHueco;
int primera=0;
int xBola,yBola;
int calibracion=0;
float calibracionx,calibraciony;
int contadorCalibracion=0;
int estadoBolaHueco=0;
char lectura[50],cadena[50];
int linea[6][4];










int botoneraPin(){
    int aux[11];
    aux[0]=0;
    aux[1]=0;
    aux[2]=0;
    aux[3]=0;
    aux[4]=0;
    aux[5]=0;
    aux[6]=0;
    aux[7]=0;
    aux[8]=0;
    aux[9]=0;
    aux[10]=0;

    ComColor(255,255,255);
    ComFgcolor(0, 0, 180);

    aux[1]=Boton(HSIZE/2-150,100,50,50,30,"1");
    aux[2]=Boton(HSIZE/2-85,100,50,50,30,"2");
    aux[3]=Boton(HSIZE/2-25,100,50,50,30,"3");
    aux[4]=Boton(HSIZE/2+35,100,50,50,30,"4");
    aux[5]=Boton(HSIZE/2+100,100,50,50,30,"5");

    aux[6]=Boton(HSIZE/2-150,160,50,50,30,"6");
    aux[7]=Boton(HSIZE/2-85,160,50,50,30,"7");
    aux[8]=Boton(HSIZE/2-25,160,50,50,30,"8");
    aux[9]=Boton(HSIZE/2+35,160,50,50,30,"9");
    aux[0]=Boton(HSIZE/2+100,160,50,50,30,"0");

    ComFgcolor(0, 0, 180);
    aux[10]=Boton(HSIZE/2-50,4*VSIZE/5,100,45,30,"OK");






    if(aux[0]==1)return 0;
    else if(aux[1]==1)return 1;
    else if(aux[2]==1)return 2;
    else if(aux[3]==1)return 3;
    else if(aux[4]==1)return 4;
    else if(aux[5]==1)return 5;
    else if(aux[6]==1)return 6;
    else if(aux[7]==1)return 7;
    else if(aux[8]==1)return 8;
    else if(aux[9]==1)return 9;
    else if(aux[10]==1)return 10;   //Intro
    else return 11; //No se pulsa nada
}
void IntTimer(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    Flag_ints++;
}

int verificarPinInicial(){
        switch(cont){
        case 0:
            aux[0]=botoneraPin();
            if(aux[0]>=0 && aux[0]<=9){
                sprintf(auxc[cont],"%d",aux[0]);
                cont=6;
            }
            return 0;
            //break;
        case 1:
            aux[1]=botoneraPin();
            ComTXT(HSIZE/2,75,25, OPT_CENTER,auxc[0]);
            if(aux[1]>=0 && aux[1]<=9){
                sprintf(auxc[cont],"%d",aux[1]);
                cont=7;
            }
            return 0;
            //break;
        case 2:
            aux[2]=botoneraPin();
            ComTXT(HSIZE/2-10,75,25, OPT_CENTER,auxc[0]);
            ComTXT(HSIZE/2+10,75,25, OPT_CENTER,auxc[1]);
            if(aux[2]>=0 && aux[2]<=9){
                sprintf(auxc[cont],"%d",aux[2]);
                cont=8;
            }
            return 0;
            //break;
        case 3:
            aux[3]=botoneraPin();
            ComTXT(HSIZE/2-20,75,25, OPT_CENTER,auxc[0]);
            ComTXT(HSIZE/2,75,25, OPT_CENTER,auxc[1]);
            ComTXT(HSIZE/2+20,75,25, OPT_CENTER,auxc[2]);
            if(aux[3]>=0 && aux[3]<=9){
                sprintf(auxc[cont],"%d",aux[3]);
                cont=9;
            }
            return 0;
            //break;
        case 4:
            aux[4]=botoneraPin();
            ComTXT(HSIZE/2-30,75,25, OPT_CENTER,auxc[0]);
            ComTXT(HSIZE/2-10,75,25, OPT_CENTER,auxc[1]);
            ComTXT(HSIZE/2+10,75,25, OPT_CENTER,auxc[2]);
            ComTXT(HSIZE/2+30,75,25, OPT_CENTER,auxc[3]);
            if(aux[4]==10){
                cont=10;
            }
            return 0;
            //break;
        case 5:
            botoneraPin();
            if(aux[0]==pin[0] && aux[1]==pin[1] && aux[2]==pin[2] && aux[3]==pin[3]){
                cont=0;
                aux[0]=0;
                aux[1]=0;
                aux[2]=0;
                aux[3]=0;
                return 1;
            }
            else{
                cont=0;
                aux[0]=0;
                aux[1]=0;
                aux[2]=0;
                aux[3]=0;
                return 0;
            }

        case 6:
            if(botoneraPin()==11)cont=1;
            break;
        case 7:
            if(botoneraPin()==11)cont=2;
            ComTXT(HSIZE/2,75,25, OPT_CENTER,auxc[0]);
            break;
        case 8:
            if(botoneraPin()==11)cont=3;
            ComTXT(HSIZE/2-10,75,25, OPT_CENTER,auxc[0]);
            ComTXT(HSIZE/2+10,75,25, OPT_CENTER,auxc[1]);
            break;
        case 9:
            if(botoneraPin()==11)cont=4;
            ComTXT(HSIZE/2-20,75,25, OPT_CENTER,auxc[0]);
            ComTXT(HSIZE/2,75,25, OPT_CENTER,auxc[1]);
            ComTXT(HSIZE/2+20,75,25, OPT_CENTER,auxc[2]);
            break;
        case 10:
            if(botoneraPin()==11)cont=5;
            ComTXT(HSIZE/2-30,75,25, OPT_CENTER,auxc[0]);
            ComTXT(HSIZE/2-10,75,25, OPT_CENTER,auxc[1]);
            ComTXT(HSIZE/2+10,75,25, OPT_CENTER,auxc[2]);
            ComTXT(HSIZE/2+30,75,25, OPT_CENTER,auxc[3]);
            break;
        }
        return 0;
}


int lecturaNumerosPantalla(){

    char auxcc[2];

    if(numeroLeidoPantalla[i]==11) numeroLeidoPantalla[i]=0;

    hora=numeroLeidoPantalla[0]*10 + numeroLeidoPantalla[1];
    minuto=numeroLeidoPantalla[2]*10 + numeroLeidoPantalla[3];
    segundo=numeroLeidoPantalla[4]*10 + numeroLeidoPantalla[5];

    if(i<2){
        ComColor(255*parpadeo,255*parpadeo,255*parpadeo);
        ComRect(HSIZE/2-45,67,HSIZE/2-20,82,1);
        ComColor(255*parpadeo2,255*parpadeo2,255*parpadeo2);
        sprintf(auxcc,"%02d:",hora);
        ComTXT(HSIZE/2-30,75,28, OPT_CENTER,auxcc);

        ComColor(255,255,255);
        sprintf(auxcc,"%02d:",minuto);
        ComTXT(HSIZE/2+3,75,28, OPT_CENTER,auxcc);
        sprintf(auxcc,"%02d",segundo);
        ComTXT(HSIZE/2+31,75,28, OPT_CENTER,auxcc);
    }
    else if(i<4){
        ComColor(255*parpadeo,255*parpadeo,255*parpadeo);
        ComRect(HSIZE/2-10,67,HSIZE/2+11,82,1);
        ComColor(255*parpadeo2,255*parpadeo2,255*parpadeo2);
        sprintf(auxcc,"%02d:",minuto);
        ComTXT(HSIZE/2+3,75,28, OPT_CENTER,auxcc);

        ComColor(255,255,255);
        sprintf(auxcc,"%02d:",hora);
        ComTXT(HSIZE/2-30,75,28, OPT_CENTER,auxcc);
        sprintf(auxcc,"%02d",segundo);
        ComTXT(HSIZE/2+31,75,28, OPT_CENTER,auxcc);
    }
    else if(i<6){
        ComColor(255*parpadeo,255*parpadeo,255*parpadeo);
        ComRect(HSIZE/2+23,67,HSIZE/2+42,82,1);
        ComColor(255*parpadeo2,255*parpadeo2,255*parpadeo2);
        sprintf(auxcc,"%02d",segundo);
        ComTXT(HSIZE/2+31,75,28, OPT_CENTER,auxcc);

        ComColor(255,255,255);
        sprintf(auxcc,"%02d:",hora);
        ComTXT(HSIZE/2-30,75,28, OPT_CENTER,auxcc);
        sprintf(auxcc,"%02d:",minuto);
        ComTXT(HSIZE/2+3,75,28, OPT_CENTER,auxcc);
    }
    else{
        ComColor(255,255,255);
        sprintf(auxcc,"%02d",segundo);
        ComTXT(HSIZE/2+31,75,28, OPT_CENTER,auxcc);
        sprintf(auxcc,"%02d:",hora);
        ComTXT(HSIZE/2-30,75,28, OPT_CENTER,auxcc);
        sprintf(auxcc,"%02d:",minuto);
        ComTXT(HSIZE/2+3,75,28, OPT_CENTER,auxcc);

    }


    switch (estadoLecturaNumerosPantalla){
        case reposo:
            numeroLeidoPantalla[i]=botoneraPin();
            if(numeroLeidoPantalla[i]>=0 && numeroLeidoPantalla[i]<=9){
                sprintf(auxc[i],"%d",numeroLeidoPantalla[i]);
                estadoLecturaNumerosPantalla=pulso;
            }
            else if(numeroLeidoPantalla[i]==10){
                estadoLecturaNumerosPantalla=pulso;
            }
            break;
        case pulso:
            if(numeroLeidoPantalla[i]==botoneraPin())estadoLecturaNumerosPantalla=pulso;
            else estadoLecturaNumerosPantalla=depulso;
            break;
        case depulso:
            if(numeroLeidoPantalla[i]==10){
                estadoLecturaNumerosPantalla=reposo;
                i=0;
                return 1;   //Si devuelve un 1 es que se ha pulsado intro y se ha terminado de escribir
            }
            else{
                estadoLecturaNumerosPantalla=reposo;
                i++;
                return 0;
            }

    }
    return 0;
}


void dibujaHora(){
    char auxcc[2];
    sprintf(auxcc,"%02d:",hora);
    ComTXT(40+320,20,28, 0,auxcc);
    sprintf(auxcc,"%02d:",minuto);
    ComTXT(70+320,20,28, 0,auxcc);
    sprintf(auxcc,"%02d",segundo);
    ComTXT(100+320,20,28, 0,auxcc);
}

void updateHora(){
    milisegundo=milisegundo+20;
    if(milisegundo>=1000){
        milisegundo=0;
        segundo++;
        if(parpadeo==0){
            parpadeo2=0;
            parpadeo++;}
        else{
            parpadeo2++;
            parpadeo=0;}
    }
    if(segundo>=60){
        segundo=0;
        minuto++;
    }
    if(minuto>=60){
        minuto=0;
        hora++;
    }
    if(hora>=24)hora=0;

}









int main(void)
{
    //
    // Run from the PLL at 120 MHz.
    //
    RELOJ = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
            SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
            SYSCTL_CFG_VCO_480), 120000000);


    //
    // Initialize the UART0
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, RELOJ);

    //
    // Initialize the UART 1.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(1, 9600, RELOJ);

    //
    // Initialize the UART2
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    GPIOPinConfigure(GPIO_PD4_U2RX);
    GPIOPinConfigure(GPIO_PD5_U2TX);
    GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    UARTStdioConfig(2, 115200, RELOJ);

    //-----------Timer 0--------------//
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/FREC)-1);
    TimerIntRegister(TIMER0_BASE,TIMER_A,IntTimer);
    IntEnable(INT_TIMER0A); //Habilitar las interrupciones globales de los timers
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
    IntMasterEnable();  //Habilitacion global de interrupciones
    TimerEnable(TIMER0_BASE, TIMER_A);  //Habilitar Timer0, 1, 2A y 2B


    SysCtlPeripheralClockGating(true);

    HAL_Init_SPI(1, RELOJ);  //Boosterpack a usar, Velocidad del MC
    Inicia_pantalla();       //Arranque de la pantalla

    PantOnOff(1);       //Habilitaci�n del altavoz
    VolNota(255);     //Volumen de la nota

    SysCtlDelay(RELOJ/3);

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// Variables que deben ejecutarse una �nica vez, antes del while(1), DENTRO DEL MAIN
//
    dir_agua=ComJPEG(agua,AGUA_DIM,0);
    dir_mapaagua=ComJPEG(mapaAgua,MAPAAGUA_DIM,0);

    dir_barco2=ComJPEG(barco2,BARCO2_DIM,0);
    dir_barco2_180=ComJPEG(barco2,BARCO2_DIM,1);
    dir_barco2_giro=ComJPEG(barco2_giro,BARCO2_DIM,0);
    dir_barco2_giro_180=ComJPEG(barco2_giro,BARCO2_DIM,1);

    dir_barco3=ComJPEG(barco3,BARCO3_DIM,0);
    dir_barco3_180=ComJPEG(barco3,BARCO3_DIM,1);
    dir_barco3_giro=ComJPEG(barco3_giro,BARCO3_DIM,0);
    dir_barco3_giro_180=ComJPEG(barco3_giro,BARCO3_DIM,1);

    dir_barco4=ComJPEG(barco4,BARCO4_DIM,0);
    dir_barco4_180=ComJPEG(barco4,BARCO4_DIM,1);
    dir_barco4_giro=ComJPEG(barco4_giro,BARCO4_DIM,0);
    dir_barco4_giro_180=ComJPEG(barco4_giro,BARCO4_DIM,1);

    dir2[0][0]= dir_barco2;         dir2[1][0]=dir_barco3;          dir2[2][0]=dir_barco3;          dir2[3][0]=dir_barco4;
    dir2[0][1]=dir_barco2_giro;     dir2[1][1]=dir_barco3_giro;     dir2[2][1]=dir_barco3_giro;     dir2[3][1]=dir_barco4_giro;
    dir2[0][2]=dir_barco2_180;      dir2[1][2]=dir_barco3_180;      dir2[2][2]=dir_barco3_180;      dir2[3][2]=dir_barco4_180;
    dir2[0][3]=dir_barco2_giro_180; dir2[1][3]=dir_barco3_giro_180; dir2[2][3]=dir_barco3_giro_180; dir2[3][3]=dir_barco4_giro_180;

    dimbarcos[0][0]=BARCO2_HEIGHT;      dimbarcos[0][1]=BARCO2_WIDTH;
    dimbarcos[1][0]=BARCO3_HEIGHT;      dimbarcos[1][1]=BARCO3_WIDTH;
    dimbarcos[2][0]=BARCO3_HEIGHT;      dimbarcos[2][1]=BARCO3_WIDTH;
    dimbarcos[3][0]=BARCO4_HEIGHT;      dimbarcos[3][1]=BARCO4_WIDTH;

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

    // ================================================================================================================
    // PANTALLA INICIAL
    // ================================================================================================================
        Nueva_pantalla(180,180,220);
        //El numerito indica el numero dela fuente que estamos usando, ver libreria pag 140
        ComColor(255,255,255);
        ComTXT(HSIZE/2,VSIZE/2-45,30, OPT_CENTERX,"TOCA LA PANTALLA");
        ComTXT(HSIZE/2,VSIZE/2,30, OPT_CENTERX,"PARA EMPEZAR");
        Dibuja();
        Espera_pant();

        for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);
        i=0;


        if(Detecta_BP(1))Conf_Boosterpack(1, RELOJ);
        else if(Detecta_BP(2))Conf_Boosterpack(2, RELOJ);

        OPT3001_init();
        bmi160_initialize_sensor();
        bmi160_config_running_mode(APPLICATION_NAVIGATION);
        readI2C(BMI160_I2C_ADDR2,BMI160_USER_CHIP_ID_ADDR, &DevID, 1);
        srand(time(NULL));



    while(1){

        SLEEP;
        Lee_pantalla();
        Nueva_pantalla(180,180,220);
        ComFgcolor(0, 255, 0);
        ComColor(255,255,255);
        //Ajuste brillo
        lux=OPT3001_getLux();
        lux_i=(int)round(lux);
        ajustaBrillo(lux_i);
        bmi160_bmm150_mag_compensate_xyz(&s_magcompXYZ);
        bmi160_read_accel_xyz(&s_accelXYZ);
        bmi160_read_gyro_xyz(&s_gyroXYZ);
//        s_accelXYZ.x=-s_accelXYZ.x;
//        s_accelXYZ.y=-s_accelXYZ.y;
        updateHora();
        if(UARTCharsAvail(UART1_BASE)){
            msg=UARTCharGetNonBlocking(UART1_BASE);
            UARTCharPutNonBlocking(UART0_BASE, msg);
            if(msg=='1')estado=paginaPrincipal;
            else if(msg=='2')estado=bolaHueco;
            else if(msg=='3'){
                estado=battleship;
                i=0;
                k=0;
                j=0;
                cuadrox= 37 +22*4; cuadroy= 37 +22*4; giro=0;
                confirmar=0; colisiones=0; colisiones2=0;
                condMismaColision=0; res=0; unacadena=0;
                leido=1; reintento=0; Confpuls=0;
                bloqueoMsgReint=0;
                aux1=0; aux2=0;
                marcado=0;
                t_nota=0;
                estadobattleship=colocar;
            }
        }



        switch (estado){
            case pedirPin:
                ComColor(255,255,255);
                ComTXT(HSIZE/2,40,30, OPT_CENTER,"Introduzca su pin:");
                ComColor(0,0,0);
                ComRect(HSIZE/2-50,62,HSIZE/2+50,87,1);
                ComColor(0,0,180);
                ComRect(HSIZE/2-48,64,HSIZE/2+48,85,1);
                if(verificarPinInicial())estado=acceso;
                else estado=pedirPin;
                segundo=0;
                break;
            case acceso:
                ComTXT(HSIZE/2,VSIZE/2-45,30, OPT_CENTERX,"ESPERA 5 SEGUNDOS");
                ComTXT(HSIZE/2,VSIZE/2,30, OPT_CENTERX,"E INTRODUCE HORA FORMATO:");
                ComTXT(HSIZE/2,VSIZE/2+45,30, OPT_CENTERX,"HHMMSS");
                if(segundo==5)estado=pedirHora;
                break;
            case pedirHora:
                auxLectura=lecturaNumerosPantalla();
                if(auxLectura && numeroLeidoPantalla[6]==10){
                    estado=paginaPrincipal;
                    hora=numeroLeidoPantalla[0]*10 + numeroLeidoPantalla[1];
                    minuto=numeroLeidoPantalla[2]*10 + numeroLeidoPantalla[3];
                    segundo=numeroLeidoPantalla[4]*10 + numeroLeidoPantalla[5];
                }
                else if(auxLectura==1){
                    estado=acceso;
                    segundo=0;
                }
                break;
            case paginaPrincipal:
                dibujaHora();
                ComTXT(HSIZE/2,VSIZE/2-45-50,30, OPT_CENTERX,"BIENVENIDO");
                ComTXT(HSIZE/2,VSIZE/2-50,30, OPT_CENTERX,"A LA PAGINA");
                ComTXT(HSIZE/2,VSIZE/2+45-50,30, OPT_CENTERX,"PRINCPIAL");
                ComFgcolor(0, 0, 255);
                if(Boton(20,200,200,40,30,"Bola Hueco"))estado=bolaHueco;
                if(Boton(250,200,200,40,30,"Battleship"))estado=battleship;

                break;

            case bolaHueco:
                if(dibujaBolaHueco())estado=paginaPrincipal;
                break;

            case battleship:
                cuadroxPuesto [0]=cuadrox2;
                cuadroxPuesto [1]=cuadrox3;
                cuadroxPuesto [2]=cuadrox3_0;
                cuadroxPuesto [3]=cuadrox4;

                cuadroyPuesto [0]=cuadroy2;
                cuadroyPuesto [1]=cuadroy3;
                cuadroyPuesto [2]=cuadroy3_0;
                cuadroyPuesto [3]=cuadroy4;

                giroPuesto [0]=giro2;
                giroPuesto [1]=giro3;
                giroPuesto [2]=giro3_0;
                giroPuesto [3]=giro4;

                if(Battleship())estado=paginaPrincipal;

                break;

        }

        Dibuja();
        }


}





int dibujaBolaHueco(){
    ComColor(0,0,0);
    ComFgcolor(0, 0, 180);
    int aux=0;
    float incrementox,incrementoy;
    float lecturax,lecturay;
    lecturax=s_accelXYZ.y;
    lecturay=s_accelXYZ.x;

    int zonaMuerta=1;
    int precision=3;
    float sensibilidad=0.005;

    switch (estadoBolaHueco){
    case 0:
        ComColor(255,255,255);
        contadorCalibracion++;
        if(contadorCalibracion<16)ComTXT(HSIZE/2-80,VSIZE/2-45,30, 0,"Cargando.");
        else if(contadorCalibracion<33)ComTXT(HSIZE/2-80,VSIZE/2-45,30, 0,"Cargando..");
        else ComTXT(HSIZE/2-80,VSIZE/2-45,30, 0,"Cargando...");
        calibracionx+=lecturax;
        calibraciony+=lecturay;
        if(contadorCalibracion==50){
            calibracionx=calibracionx/50;
            calibraciony=calibraciony/50;
            contadorCalibracion=0;
            estadoBolaHueco=1;
        }
        break;

    case 1:
        ComColor(0,0,0);
        xHueco=30 + rand() % (420 - 30 + 1);
        yHueco=30 + rand() % (240 - 30 + 1);
        if(xHueco<85 && xHueco>35 || xHueco<165 && xHueco>115 || xHueco<375 && xHueco>325 || yHueco<225 && yHueco>175 || yHueco<125 && yHueco>75 || yHueco>25 && yHueco<75){
            xHueco= 90+rand() % (320 - 90 + 1);
            yHueco= 140+rand() % (175 - 140 + 1);
        }
        xBola=20;
        yBola=20;
        //Lineas horizontales
        linea[0][0]=60; linea[0][1]=200; linea[0][2]=250;  linea[0][3]=200;
        linea[1][0]=140; linea[1][1]=100; linea[1][2]=300;  linea[1][3]=100;
        linea[2][0]=250; linea[2][1]=50; linea[2][2]=350;  linea[2][3]=50;
        //Lineas verticales
        linea[3][0]=60; linea[3][1]=40; linea[3][2]=60;  linea[3][3]=200;
        linea[4][0]=140; linea[4][1]=40; linea[4][2]=140;  linea[4][3]=100;
        linea[5][0]=350; linea[5][1]=50; linea[5][2]=350;  linea[5][3]=230;
        estadoBolaHueco=2;
        break;
    case 2:
        ComCirculo(xHueco,yHueco, 15);
        ComColor(255,255,255);
        int dist=20;
        //Movimiento de la bola blanca
        if(lecturax < calibracionx +zonaMuerta && lecturax > calibracionx -zonaMuerta )incrementox=0;
        else incrementox=lecturax*sensibilidad;
        if(lecturay < calibraciony +zonaMuerta && lecturax > calibraciony -zonaMuerta )incrementoy=0;
        else incrementoy=lecturay*sensibilidad;

        int xBolaAnterior=xBola;
        int yBolaAnterior=yBola;

        if(incrementox>15)incrementox=15;
        if(incrementoy>15)incrementoy=15;

        xBola=xBola+incrementox;
        yBola=yBola+incrementoy;

        if(xBola>420)xBola=420;
        if(xBola<20)xBola=20;
        if(yBola>255)yBola=255;
        if(yBola<20)yBola=20;

        //Lineas horizontales
            if(xBola>linea[0][0]-dist && xBola<linea[0][2]+dist && yBola>linea[0][1]-dist && yBola<linea[0][1]+dist){

                //yBola=yBolaAnterior;
                if(yBola-yBolaAnterior>0)yBola=linea[0][1]-dist;
                if(yBola-yBolaAnterior<0)yBola=linea[0][1]+dist;   //Limite por debajo
            }
            if(xBola>linea[1][0]-dist && xBola<linea[1][2]+dist && yBola>linea[1][1]-dist && yBola<linea[1][1]+dist){

                //yBola=yBolaAnterior;
                if(yBola-yBolaAnterior>0)yBola=linea[1][1]-dist;
                if(yBola-yBolaAnterior<0)yBola=linea[1][1]+dist;   //Limite por debajo
            }
            if(xBola>linea[2][0]-dist && xBola<linea[2][2]+dist && yBola>linea[2][1]-dist && yBola<linea[2][1]+dist){

                //yBola=yBolaAnterior;
                if(yBola-yBolaAnterior>0)yBola=linea[2][1]-dist;
                if(yBola-yBolaAnterior<0)yBola=linea[2][1]+dist;   //Limite por debajo
            }


            //Lineas vcerticales
            if(xBola>linea[3][0]-dist && xBola<linea[3][2]+dist && yBola>linea[3][1]-dist && yBola<linea[3][3]+dist){

                //yBola=yBolaAnterior;
                if(xBola-xBolaAnterior>0)xBola=linea[3][0]-dist;
                if(xBola-xBolaAnterior<0)xBola=linea[3][0]+dist;   //Limite por derecha
            }
            if(xBola>linea[4][0]-dist && xBola<linea[4][2]+dist && yBola>linea[4][1]-dist && yBola<linea[4][3]+dist){

                //yBola=yBolaAnterior;
                if(xBola-xBolaAnterior>0)xBola=linea[4][0]-dist;
                if(xBola-xBolaAnterior<0)xBola=linea[4][0]+dist;   //Limite por derecha
            }
            if(xBola>linea[5][0]-dist && xBola<linea[5][2]+dist && yBola>linea[5][1]-dist && yBola<linea[5][3]+dist){

                //yBola=yBolaAnterior;
                if(xBola-xBolaAnterior>0)xBola=linea[5][0]-dist;
                if(xBola-xBolaAnterior<0)xBola=linea[5][0]+dist;   //Limite por derecha
            }






        //Pintamos las paredes
        ComColor(0,0,255);
        ComLine(linea[0][0], linea[0][1], linea[0][2],  linea[0][3], 10);
        ComLine(linea[1][0], linea[1][1], linea[1][2],  linea[1][3], 10);
        ComLine(linea[2][0], linea[2][1], linea[2][2],  linea[2][3], 10);
        ComLine(linea[3][0], linea[3][1], linea[3][2],  linea[3][3], 10);
        ComLine(linea[4][0], linea[4][1], linea[4][2],  linea[4][3], 10);
        ComLine(linea[5][0], linea[5][1], linea[5][2],  linea[5][3], 10);

        ComColor(0,0,0);
        ComFgcolor(255, 0, 0);
        aux=Boton(440,0,30,30,30,"X");

        ComColor(255,255,255);
        ComCirculo(xBola,yBola, 10);

        if(xBola>xHueco-precision && xBola<xHueco+precision && yBola>yHueco-precision && yBola<yHueco+precision )estadoBolaHueco=1;
        if(aux==1)estadoBolaHueco=3;
        break;

    case 3:
        if(!(Boton(440,0,30,30,30,"X"))){
            estadoBolaHueco=0;
            return 1;
        }


        break;

    }



    return 0;

}
