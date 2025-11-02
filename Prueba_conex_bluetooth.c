#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"
#include <stdio.h>
#include <string.h>

// =======================================================================
// Function Declarations
// =======================================================================
#define dword long
#define byte char

volatile int Flag_ints=0;

//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()

void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

#define NUM_SSI_DATA            3

int RELOJ;
#define FREC 50 //Frecuencia en hercios del tren de pulsos: 20ms

void IntTimer(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    Flag_ints++;
}

int Load;   // Variable para comprobar el % de carga del ciclo de trabajo



typedef enum{conexión, uno , dos ,tres ,cuatro} fases;
fases estado=conexión;

int t_espera=0;


int main(void)
 {

    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    // =======================================================================
    // Delay before we begin to display anything
    // =======================================================================

    SysCtlDelay(RELOJ/3);

    // ================================================================================================================
    // PANTALLA INICIAL
    // ================================================================================================================
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    //LEDs
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 |GPIO_PIN_4); //F0 y F4: salidas
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 |GPIO_PIN_1); //N0 y N1: salidas

    //UART conectada al módulo bluetooth
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
    GPIOPinConfigure(GPIO_PJ0_U3RX);
    GPIOPinConfigure(GPIO_PJ1_U3TX);
    GPIOPinTypeUART(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 9600, RELOJ);

    // UART usada para mandar datos desde el ordenador
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 9600, RELOJ);


    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/FREC)-1);
    TimerIntRegister(TIMER0_BASE,TIMER_A,IntTimer);

    IntEnable(INT_TIMER0A); //Habilitar las interrupciones globales de los timers
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
    IntMasterEnable();  //Habilitacion global de interrupciones
    TimerEnable(TIMER0_BASE, TIMER_A);  //Habilitar Timer0, 1, 2A y 2B

    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART3);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPION);

    while(1){
        SLEEP;

        switch(estado){

            case conexión:

                if (UARTCharGetNonBlocking(UART3_BASE)!= '1'){
                    if(UARTCharsAvail(UART3_BASE))
                        UARTCharPutNonBlocking(UART3_BASE,'1');
                }
                else if (UARTCharGetNonBlocking(UART3_BASE)== '1'){
                    if(UARTCharsAvail(UART3_BASE))
                        UARTCharPutNonBlocking(UART3_BASE,'1');

                    if(UARTCharsAvail(UART0_BASE))
                        UARTCharPutNonBlocking(UART0_BASE,'B');

                    estado = uno;
                }

                break;

            case uno:

                t_espera++;

                if (t_espera>150){
                    if(UARTCharsAvail(UART3_BASE))
                        UARTCharPutNonBlocking(UART3_BASE,'1');

                    if (UARTCharGetNonBlocking(UART3_BASE)== '1'){
                        estado= dos;
                        t_espera=0;

                        if(UARTCharsAvail(UART0_BASE))
                            UARTCharPutNonBlocking(UART0_BASE,'2');
                    }

                }
                GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);

                break;

            case dos:

                t_espera++;

                if (t_espera>150){
                    if(UARTCharsAvail(UART3_BASE))
                        UARTCharPutNonBlocking(UART3_BASE,'1');

                    if (UARTCharGetNonBlocking(UART3_BASE)== '1'){
                        estado= tres;
                        t_espera=0;

                        if(UARTCharsAvail(UART0_BASE))
                            UARTCharPutNonBlocking(UART0_BASE,'3');
                    }

                }
                GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);

                break;

            case tres:

                t_espera++;

                if (t_espera>150){
                    if(UARTCharsAvail(UART3_BASE))
                        UARTCharPutNonBlocking(UART3_BASE,'1');

                    if (UARTCharGetNonBlocking(UART3_BASE)== '1'){
                        estado= cuatro;
                        t_espera=0;

                        if(UARTCharsAvail(UART0_BASE))
                            UARTCharPutNonBlocking(UART0_BASE,'4');
                    }

                }
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4);

                break;


            case cuatro:

                t_espera++;

                if (t_espera>150){
                    if(UARTCharsAvail(UART3_BASE))
                        UARTCharPutNonBlocking(UART3_BASE,'1');

                    if (UARTCharGetNonBlocking(UART3_BASE)== '1'){
                        estado= uno;
                        t_espera=0;
                        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
                        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0);
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);7

                        if(UARTCharsAvail(UART0_BASE))
                            UARTCharPutNonBlocking(UART0_BASE,'4');
                    }

                }
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);

                break;

        }

    }
}






