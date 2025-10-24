
//
// hello.c - Simple hello world example.
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"
#include <stdio.h>



/*
 * EJEMPLO 4: Manejo simple del terminal usando las funciones del UARTstdio
 * Es necesario incluir el fichero uartstdio.c, que está en el directorio
 * <TIVAWARE_INSTALL>/utils/
 * Para ello: add files
 *              -> se busca
 *                  ->link to files
 *                      -> create link locations relative to
 *                          -> TIVAWARE_INSTALL
 */


//*****************************************************************************
//
// System clock rate in Hz.
//
//*****************************************************************************
uint32_t RELOJ;


//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()

#define B1_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0)
#define B1_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0))
#define B2_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1)
#define B2_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1))

#define FREC 50 //Frecuencia en hercios del tren de pulsos: 20ms
volatile int Flag_ints=0;
unsigned long REG_TT[6];
const int32_t REG_CAL[6]= {CAL_DEFAULTS};

void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

//Enum de estados
typedef enum {
    pedirPin = 0,
    acceso=1,

} Estados;

int estado=pedirPin;

int aux[5];
int cont=0;
char auxc[4][2];
int pin[] = {1,2,3,4};





int repintarPantallaBienvenida(){
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
    ComFgcolor(0, 255, 0);

    aux[1]=Boton(HSIZE/2-70-30,20,60,60,30,"1");
    aux[2]=Boton(HSIZE/2-30,20,60,60,30,"2");
    aux[3]=Boton(HSIZE/2+70-30,20,60,60,30,"3");

    aux[4]=Boton(HSIZE/2-70-30,90,60,60,30,"4");
    aux[5]=Boton(HSIZE/2-30,90,60,60,30,"5");
    aux[6]=Boton(HSIZE/2+70-30,90,60,60,30,"6");

    aux[7]=Boton(HSIZE/2-70-30,160,60,60,30,"7");
    aux[8]=Boton(HSIZE/2-30,160,60,60,30,"8");
    aux[9]=Boton(HSIZE/2+70-30,160,60,60,30,"9");

    aux[0]=Boton(HSIZE/2-30+140,160,60,60,30,"0");

    ComFgcolor(255, 255, 0);
    aux[10]=Boton(HSIZE/2-30+140,160-70,120,60,30,"OK");



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
    else if(aux[10]==1)return 10;
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
            aux[0]=repintarPantallaBienvenida();
            if(aux[0]>=0 && aux[0]<=9){
                sprintf(auxc[cont],"%d",aux[0]);
                cont=6;
            }
            return 0;
            //break;
        case 1:
            aux[1]=repintarPantallaBienvenida();
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            if(aux[1]>=0 && aux[1]<=9){
                sprintf(auxc[cont],"%d",aux[1]);
                cont=7;
            }
            return 0;
            //break;
        case 2:
            aux[2]=repintarPantallaBienvenida();
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            ComTXT(20+20,20,25, OPT_CENTER,auxc[1]);
            if(aux[2]>=0 && aux[2]<=9){
                sprintf(auxc[cont],"%d",aux[2]);
                cont=8;
            }
            return 0;
            //break;
        case 3:
            aux[3]=repintarPantallaBienvenida();
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            ComTXT(20+20,20,25, OPT_CENTER,auxc[1]);
            ComTXT(20+20+20,20,25, OPT_CENTER,auxc[2]);
            if(aux[3]>=0 && aux[3]<=9){
                sprintf(auxc[cont],"%d",aux[3]);
                cont=9;
            }
            return 0;
            //break;
        case 4:
            aux[4]=repintarPantallaBienvenida();
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            ComTXT(20+20,20,25, OPT_CENTER,auxc[1]);
            ComTXT(20+20+20,20,25, OPT_CENTER,auxc[2]);
            ComTXT(20+20+20+20,20,25, OPT_CENTER,auxc[3]);
            if(aux[4]==10){
                cont=10;
            }
            return 0;
            //break;
        case 5:
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
            if(repintarPantallaBienvenida()==11)cont=1;
            break;
        case 7:
            if(repintarPantallaBienvenida()==11)cont=2;
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            break;
        case 8:
            if(repintarPantallaBienvenida()==11)cont=3;
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            ComTXT(20+20,20,25, OPT_CENTER,auxc[1]);
            break;
        case 9:
            if(repintarPantallaBienvenida()==11)cont=4;
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            ComTXT(20+20,20,25, OPT_CENTER,auxc[1]);
            ComTXT(20+20+20,20,25, OPT_CENTER,auxc[2]);
            break;
        case 10:
            if(repintarPantallaBienvenida()==11)cont=5;
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            ComTXT(20+20,20,25, OPT_CENTER,auxc[1]);
            ComTXT(20+20+20,20,25, OPT_CENTER,auxc[2]);
            ComTXT(20+20+20+20,20,25, OPT_CENTER,auxc[3]);
            break;
        }
        return 0;
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




    // ================================================================================================================
    // PANTALLA INICIAL
    // ================================================================================================================
        Nueva_pantalla(16,16,16);
        ComColor(153,76,0);
        ComRect(10, 10, HSIZE-10, VSIZE-10, true);
        //El numerito indica el numero dela fuente que estamos usando, ver libreria pag 140
    //    ComTXT(HSIZE/2,50+VSIZE/5, 22, OPT_CENTERX," SEPA GIERM. 2024 ");
    //    ComTXT(HSIZE/2,100+VSIZE/5, 20, OPT_CENTERX,"M.A.P.E.");
        ComColor(255,255,255);
    //    ComRect(40,60, HSIZE-40, VSIZE-160, true);
        ComTXT(HSIZE/2,VSIZE/2-45,30, OPT_CENTERX,"TOCA LA PANTALLA");
        ComTXT(HSIZE/2,VSIZE/2,30, OPT_CENTERX,"PARA EMPEZAR");
        Dibuja();
        Espera_pant();
        int i;
        for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);





    while(1){
        SLEEP;
        Lee_pantalla();
        Nueva_pantalla(16,16,16);
        ComColor(153,76,0);
        ComRect(10, 10, HSIZE-10, VSIZE-10, true);
        ComColor(255,255,255);
        ComFgcolor(0, 255, 0);
        ComColor(255,255,255);



        switch (estado){
            case pedirPin:
                if(verificarPinInicial())estado=acceso;
                else estado=pedirPin;
                break;
            case acceso:
                ComTXT(HSIZE/2,VSIZE/2-45,30, OPT_CENTERX,"HOLA");
                ComTXT(HSIZE/2,VSIZE/2,30, OPT_CENTERX,"NOCI");

                break;

        }

        Dibuja();
    }

}










=======
//*****************************************************************************
//
// hello.c - Simple hello world example.
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"
#include <stdio.h>



/*
 * EJEMPLO 4: Manejo simple del terminal usando las funciones del UARTstdio
 * Es necesario incluir el fichero uartstdio.c, que está en el directorio
 * <TIVAWARE_INSTALL>/utils/
 * Para ello: add files
 *              -> se busca
 *                  ->link to files
 *                      -> create link locations relative to
 *                          -> TIVAWARE_INSTALL
 */


//*****************************************************************************
//
// System clock rate in Hz.
//
//*****************************************************************************
uint32_t RELOJ;


//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()

#define B1_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0)
#define B1_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0))
#define B2_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1)
#define B2_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1))

#define FREC 50 //Frecuencia en hercios del tren de pulsos: 20ms
volatile int Flag_ints=0;
unsigned long REG_TT[6];
const int32_t REG_CAL[6]= {CAL_DEFAULTS};

void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

//Enum de estados
typedef enum {
    pedirPin = 0,
    acceso=1,

} Estados;

int estado=pedirPin;

int aux[5];
int cont=0;
char auxc[4][2];
int pin[] = {1,2,3,4};





int repintarPantallaBienvenida(){
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
    ComFgcolor(0, 255, 0);

    aux[1]=Boton(HSIZE/2-70-30,20,60,60,30,"1");
    aux[2]=Boton(HSIZE/2-30,20,60,60,30,"2");
    aux[3]=Boton(HSIZE/2+70-30,20,60,60,30,"3");

    aux[4]=Boton(HSIZE/2-70-30,90,60,60,30,"4");
    aux[5]=Boton(HSIZE/2-30,90,60,60,30,"5");
    aux[6]=Boton(HSIZE/2+70-30,90,60,60,30,"6");

    aux[7]=Boton(HSIZE/2-70-30,160,60,60,30,"7");
    aux[8]=Boton(HSIZE/2-30,160,60,60,30,"8");
    aux[9]=Boton(HSIZE/2+70-30,160,60,60,30,"9");

    aux[0]=Boton(HSIZE/2-30+140,160,60,60,30,"0");

    ComFgcolor(255, 255, 0);
    aux[10]=Boton(HSIZE/2-30+140,160-70,120,60,30,"OK");



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
    else if(aux[10]==1)return 10;
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
            aux[0]=repintarPantallaBienvenida();
            if(aux[0]>=0 && aux[0]<=9){
                sprintf(auxc[cont],"%d",aux[0]);
                cont=6;
            }
            return 0;
            //break;
        case 1:
            aux[1]=repintarPantallaBienvenida();
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            if(aux[1]>=0 && aux[1]<=9){
                sprintf(auxc[cont],"%d",aux[1]);
                cont=7;
            }
            return 0;
            //break;
        case 2:
            aux[2]=repintarPantallaBienvenida();
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            ComTXT(20+20,20,25, OPT_CENTER,auxc[1]);
            if(aux[2]>=0 && aux[2]<=9){
                sprintf(auxc[cont],"%d",aux[2]);
                cont=8;
            }
            return 0;
            //break;
        case 3:
            aux[3]=repintarPantallaBienvenida();
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            ComTXT(20+20,20,25, OPT_CENTER,auxc[1]);
            ComTXT(20+20+20,20,25, OPT_CENTER,auxc[2]);
            if(aux[3]>=0 && aux[3]<=9){
                sprintf(auxc[cont],"%d",aux[3]);
                cont=9;
            }
            return 0;
            //break;
        case 4:
            aux[4]=repintarPantallaBienvenida();
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            ComTXT(20+20,20,25, OPT_CENTER,auxc[1]);
            ComTXT(20+20+20,20,25, OPT_CENTER,auxc[2]);
            ComTXT(20+20+20+20,20,25, OPT_CENTER,auxc[3]);
            if(aux[4]==10){
                cont=10;
            }
            return 0;
            //break;
        case 5:
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
            if(repintarPantallaBienvenida()==11)cont=1;
            break;
        case 7:
            if(repintarPantallaBienvenida()==11)cont=2;
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            break;
        case 8:
            if(repintarPantallaBienvenida()==11)cont=3;
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            ComTXT(20+20,20,25, OPT_CENTER,auxc[1]);
            break;
        case 9:
            if(repintarPantallaBienvenida()==11)cont=4;
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            ComTXT(20+20,20,25, OPT_CENTER,auxc[1]);
            ComTXT(20+20+20,20,25, OPT_CENTER,auxc[2]);
            break;
        case 10:
            if(repintarPantallaBienvenida()==11)cont=5;
            ComTXT(20,20,25, OPT_CENTER,auxc[0]);
            ComTXT(20+20,20,25, OPT_CENTER,auxc[1]);
            ComTXT(20+20+20,20,25, OPT_CENTER,auxc[2]);
            ComTXT(20+20+20+20,20,25, OPT_CENTER,auxc[3]);
            break;
        }
        return 0;
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




    // ================================================================================================================
    // PANTALLA INICIAL
    // ================================================================================================================
        Nueva_pantalla(16,16,16);
        ComColor(153,76,0);
        ComRect(10, 10, HSIZE-10, VSIZE-10, true);
        //El numerito indica el numero dela fuente que estamos usando, ver libreria pag 140
    //    ComTXT(HSIZE/2,50+VSIZE/5, 22, OPT_CENTERX," SEPA GIERM. 2024 ");
    //    ComTXT(HSIZE/2,100+VSIZE/5, 20, OPT_CENTERX,"M.A.P.E.");
        ComColor(255,255,255);
    //    ComRect(40,60, HSIZE-40, VSIZE-160, true);
        ComTXT(HSIZE/2,VSIZE/2-45,30, OPT_CENTERX,"TOCA LA PANTALLA");
        ComTXT(HSIZE/2,VSIZE/2,30, OPT_CENTERX,"PARA EMPEZAR");
        Dibuja();
        Espera_pant();
        int i;
        for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);





    while(1){
        SLEEP;
        Lee_pantalla();
        Nueva_pantalla(16,16,16);
        ComColor(153,76,0);
        ComRect(10, 10, HSIZE-10, VSIZE-10, true);
        ComColor(255,255,255);
        ComFgcolor(0, 255, 0);
        ComColor(255,255,255);



        switch (estado){
            case pedirPin:
                if(verificarPinInicial())estado=acceso;
                else estado=pedirPin;
                break;
            case acceso:
                ComTXT(HSIZE/2,VSIZE/2-45,30, OPT_CENTERX,"HOLA");
                ComTXT(HSIZE/2,VSIZE/2,30, OPT_CENTERX,"NOCI");

                break;

        }

        Dibuja();
    }

}










>>>>>>> 914f89e838b14f2b611ed72dc99736879487bad6
