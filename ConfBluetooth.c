#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"

#include "FT800_TIVA.h"


// =======================================================================
// Function Declarations
// =======================================================================
#define dword long
#define byte char

volatile int Flag_ints=0;

#define SLEEP SysCtlSleep()
//#define SLEEP SysCtlSleepFake()

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

    // UART conectada al módulo bluetooth
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 38400, RELOJ);

    //UART usada para mandar datos desde el ordenador
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
    GPIOPinConfigure(GPIO_PJ0_U3RX);
    GPIOPinConfigure(GPIO_PJ1_U3TX);
    GPIOPinTypeUART(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
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

    UARTCharPutNonBlocking(UART3_BASE, "Dispositivo listo para recibir comando AT");

    while(1){
        SLEEP;

        if(UARTCharsAvail(UART0_BASE)){
                UARTCharPutNonBlocking(UART3_BASE, UARTCharGetNonBlocking(UART0_BASE));
                UARTCharPutNonBlocking(UART0_BASE, UARTCharGetNonBlocking(UART0_BASE));

        }

        if(UARTCharsAvail(UART3_BASE)){
                UARTCharPutNonBlocking(UART0_BASE, UARTCharGetNonBlocking(UART3_BASE));

        }
    }
}






