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
char  cadena,msg;

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
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE,GPIO_PIN_1);

    //UART conectada al módulo bluetooth
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    GPIOPinConfigure(GPIO_PD4_U2RX);
    GPIOPinConfigure(GPIO_PD5_U2TX);
    GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    UARTStdioConfig(2, 9600, RELOJ);


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

    //SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART2);


    UARTprintf("\r\n[TM4C129] Sistema listo. Envia AT+\r\n");

    while(1){
        SLEEP;

        if(UARTCharsAvail(UART0_BASE)){
            msg=UARTCharGetNonBlocking(UART0_BASE);
            UARTCharPutNonBlocking(UART2_BASE, msg);
            UARTCharPutNonBlocking(UART2_BASE, '\n');
            if(msg=='1'){
                GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
                UARTCharPutNonBlocking(UART0_BASE, '1');
                UARTCharPutNonBlocking(UART0_BASE, '\n');
            }
            else{
                GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
                UARTCharPutNonBlocking(UART0_BASE, msg);
                UARTCharPutNonBlocking(UART0_BASE, '\n');
            }
        }


        if(UARTCharsAvail(UART2_BASE)){
            cadena=UARTCharGetNonBlocking(UART2_BASE);
            UARTCharPutNonBlocking(UART2_BASE, cadena);
            UARTCharPutNonBlocking(UART0_BASE, cadena);
        }
    }
}






