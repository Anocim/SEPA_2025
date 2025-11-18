#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"
#include <stdio.h>
#include <string.h>
#include <imagenes.h>

//*****************************************************************************
//
// System clock rate in Hz.
//
//*****************************************************************************
uint32_t RELOJ;


//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()
#define RGB565               7
#define BITMAPS              1
#define BITMAP_SOURCE(addr) ((1<<24)|(((addr)&1048575)<<0))
#define BITMAP_LAYOUT(format,linestride,height) ((7<<24)|(((format)&31)<<19)|(((linestride)&1023)<<9)|(((height)&511)<<0))
#define BITMAP_SIZE(filter,wrapx,wrapy,width,height) ((8<<24)|(((filter)&1)<<20)|(((wrapx)&1)<<19)|(((wrapy)&1)<<18)|(((width)&511)<<9)|(((height)&511)<<0))

#define BEGIN(prim) ((31<<24)|(((prim)&15)<<0))
#define END() ((33<<24))
uint32_t destino =RAM_G + 16;
uint32_t adress=RAM_G, width_dir=RAM_G+4, height_dir=RAM_G+8, ptr, ancho, alto;
unsigned int CMD_Offset_G = 0;

uint32_t dir_agua;
uint32_t dir_barco2, dir_barco2_180, dir_barco2_giro, dir_barco2_giro_180;
uint32_t dir_barco3, dir_barco3_180, dir_barco3_giro, dir_barco3_giro_180;
uint32_t dir_barco4, dir_barco4_180, dir_barco4_giro, dir_barco4_giro_180;


#define FREC 50 //Frecuencia en hercios del tren de pulsos: 20ms
volatile int Flag_ints=0;
unsigned long REG_TT[6];
const int32_t REG_CAL[6]= {CAL_DEFAULTS};


uint32_t LeeRam32(uint32_t addr)
{
    uint32_t valor;

    HAL_SPI_CSLow();
    FT800_SPI_SendAddressRD(addr);
    valor = FT800_SPI_Read32();
    HAL_SPI_CSHigh();

    return valor;
}

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

uint32_t EscribeRam16_G(uint16_t dato)
{
    HAL_SPI_CSLow();
    uint32_t adress=RAM_G + CMD_Offset_G;
    FT800_SPI_SendAddressWR(RAM_G + CMD_Offset_G);  // Writing to next available location in FIFO (FIFO base address + offset)
    FT800_SPI_Write16(dato);                  // Vertex 2F    01XXXXXX XXXXXXXX XYYYYYYY YYYYYYYY
    HAL_SPI_CSHigh();

    CMD_Offset_G = FT800_IncCMDOffset_G(CMD_Offset_G, 2); // Move the CMD Offset since we have just added 4 bytes to the FIFO
    return adress;
}



uint32_t ComJPEG(const uint16_t *jpg, const uint32_t longitud, int vuelta)
{
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

void ComBITMAP(uint32_t direccion, int ancho_im, int largo_im, int escala, int filtro, int x, int y){

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


void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

//Enum de estados
typedef enum {
    pedirPin = 0,
    acceso=1,
    pedirHora=2,
    paginaPrincipal=3,
    laberinto=4,

} Estados;

int estado=pedirPin;


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
int i,k=0,j=0,t_espera=0;
int hora=0,minuto=0,segundo=0,milisegundo=0;
int auxLectura=0;
int parpadeo,parpadeo2;






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
        ComRect(HSIZE/2-90,30,HSIZE/2-35,60,1);
        ComColor(255*parpadeo2,255*parpadeo2,255*parpadeo2);
        sprintf(auxcc,"%02d:",hora);
        ComTXT(HSIZE/2-60,45,31, OPT_CENTER,auxcc);

        ComColor(255,255,255);
        sprintf(auxcc,"%02d:",minuto);
        ComTXT(HSIZE/2+3,45,31, OPT_CENTER,auxcc);
        sprintf(auxcc,"%02d",segundo);
        ComTXT(HSIZE/2+60,45,31, OPT_CENTER,auxcc);
    }
    else if(i<4){
        ComColor(255*parpadeo,255*parpadeo,255*parpadeo);
        ComRect(HSIZE/2-25,30,HSIZE/2+28,60,1);
        ComColor(255*parpadeo2,255*parpadeo2,255*parpadeo2);
        sprintf(auxcc,"%02d:",minuto);
        ComTXT(HSIZE/2+3,45,31, OPT_CENTER,auxcc);

        ComColor(255,255,255);
        sprintf(auxcc,"%02d:",hora);
        ComTXT(HSIZE/2-60,45,31, OPT_CENTER,auxcc);
        sprintf(auxcc,"%02d",segundo);
        ComTXT(HSIZE/2+60,45,31, OPT_CENTER,auxcc);
    }
    else if(i<6){
        ComColor(255*parpadeo,255*parpadeo,255*parpadeo);
        ComRect(HSIZE/2+38,30,HSIZE/2+85,60,1);
        ComColor(255*parpadeo2,255*parpadeo2,255*parpadeo2);
        sprintf(auxcc,"%02d",segundo);
        ComTXT(HSIZE/2+60,45,31, OPT_CENTER,auxcc);

        ComColor(255,255,255);
        sprintf(auxcc,"%02d:",hora);
        ComTXT(HSIZE/2-60,45,31, OPT_CENTER,auxcc);
        sprintf(auxcc,"%02d:",minuto);
        ComTXT(HSIZE/2+3,45,31, OPT_CENTER,auxcc);
    }
    else{
        ComColor(255,255,255);
        sprintf(auxcc,"%02d",segundo);
        ComTXT(HSIZE/2+60,45,31, OPT_CENTER,auxcc);
        sprintf(auxcc,"%02d:",hora);
        ComTXT(HSIZE/2-60,45,31, OPT_CENTER,auxcc);
        sprintf(auxcc,"%02d:",minuto);
        ComTXT(HSIZE/2+3,45,31, OPT_CENTER,auxcc);

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
    // Initialize the UART.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, RELOJ);


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
    SysCtlDelay(RELOJ/3);

    dir_agua=ComJPEG(agua,AGUA_DIM,0);

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
    while(1){
        SLEEP;
        Lee_pantalla();
        Nueva_pantalla(180,180,220);

        if(estado==pedirPin){
            for(i=0;i<9;i++)
                for(j=0;j<9;j++)
                        ComBITMAP(dir_agua, AGUA_WIDTH, AGUA_HEIGHT,1, 0, 37 + 22*i, 37 + 22*j);

        ComBITMAP(dir_barco2, BARCO2_WIDTH, BARCO2_HEIGHT,1, 0, 37, 37);
        ComBITMAP(dir_barco2_giro_180, BARCO2_GIRO_WIDTH, BARCO2_GIRO_HEIGHT, 8, 1, 37, 103);


        /*ComBITMAP(dir_barco2_180, BARCO2_WIDTH, BARCO2_HEIGHT, 0, 1, 0, 0, 0, 0, 0, 59, 37);
        ComBITMAP(dir_barco2_giro, BARCO2_GIRO_WIDTH, BARCO2_GIRO_HEIGHT, 0, 1, 0, 0, 0, 0, 0, 37, 81);
        ComBITMAP(dir_barco2_giro_180, BARCO2_GIRO_WIDTH, BARCO2_GIRO_HEIGHT, 0, 1, 0, 0, 0, 0, 0, 37, 103);

        ComBITMAP(dir_barco3, BARCO3_WIDTH, BARCO3_HEIGHT, 0, 1, 0, 0, 0, 0, 0, 109, 37);
        ComBITMAP(dir_barco3_180, BARCO3_WIDTH, BARCO3_HEIGHT, 0, 1, 0, 0, 0, 0, 0, 131, 37);
        ComBITMAP(dir_barco3_giro, BARCO3_GIRO_WIDTH, BARCO3_GIRO_HEIGHT, 0, 1, 0, 0, 0, 0, 0, 109, 103);
        ComBITMAP(dir_barco3_giro_180, BARCO3_GIRO_WIDTH, BARCO3_GIRO_HEIGHT, 0, 1, 0, 0, 0, 0, 0, 109, 125);

        ComBITMAP(dir_barco4, BARCO4_WIDTH, BARCO4_HEIGHT, 0, 1, 0, 0, 0, 0, 0, 181, 37);
        ComBITMAP(dir_barco4_180, BARCO4_WIDTH, BARCO4_HEIGHT, 0, 1, 0, 0, 0, 0, 0, 203, 37);
        ComBITMAP(dir_barco4_giro, BARCO4_GIRO_WIDTH, BARCO4_GIRO_HEIGHT, 0, 1, 0, 0, 0, 0, 0, 181, 125);
        ComBITMAP(dir_barco4_giro_180, BARCO4_GIRO_WIDTH, BARCO4_GIRO_HEIGHT, 0, 1, 0, 0, 0, 0, 0, 181, 147);*/



        }

        else{
        ComFgcolor(0, 255, 0);
        ComColor(255,255,255);


        updateHora();

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
                ComTXT(HSIZE/2,VSIZE/2-45,30, OPT_CENTERX,"BIENVENIDO");
                ComTXT(HSIZE/2,VSIZE/2,30, OPT_CENTERX,"A LA PAGINA");
                ComTXT(HSIZE/2,VSIZE/2+45,30, OPT_CENTERX,"PRINCPIAL");
                if (Boton(HSIZE/2-30,30,60,60,30,"Laberinto")){
                    t_espera=0;
                    estado=laberinto;
                }
                break;

           case laberinto:

                break;
        }


        }
        Dibuja();

    }

}





