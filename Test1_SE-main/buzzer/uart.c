#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h> 
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "inc/hw_memmap.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "inc/hw_ints.h"
#include "driverlib/adc.h"
#include "driverlib/pwm.h"
#include "driverlib/adc.h"
//Pines
//EJ2
//PL3 Calentador-Led
//PF1 Motor-PWM
//PL1 Ena1
//PL2 Ena2
//EJ4
//PL4 Led encender Boton
//EJ5
//PF2 LED1-PWM
//PF3 LED2-PWM
//PK3 Potenciometro

#define LED1_PWM_PIN GPIO_PIN_3  
#define LED2_PWM_PIN GPIO_PIN_2  
#define PWM_FREQUENCY 1000     
#define ADC_CHANNEL ADC_CTL_CH19 
#define ENA_PIN        GPIO_PIN_1       
#define IN1_PIN        GPIO_PIN_4       // Dirección IN1 (PF4)
#define IN2_PIN        GPIO_PIN_5       // Dirección IN2 (PF5)

char data[100];

uint32_t FS = 120000000 * 1;  
int cont = 0;
int cont2 = 0;
float distancia = 0.0;
volatile bool temporizadorActivo = false;
float time = (120000000*2)/3;
uint32_t g_ui32SysClock;
uint32_t g_ui32PWMIncrement;
void Timer0IntHandler(void);

// Función para enviar un string por UART0
void UARTSendString(const char *str) {
    while (*str) {
        UARTCharPut(UART0_BASE, *str++);
    }
}
void ConfigurePWM1(void) {
    uint32_t ui32PWMClockRate;
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinConfigure(GPIO_PF1_M0PWM1);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
    
    PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_8);
    ui32PWMClockRate = g_ui32SysClock / 8;
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, ui32PWMClockRate / 250);  // 250Hz
    
    g_ui32PWMIncrement = ((ui32PWMClockRate / 250) * 95) / 100;  // 95% duty cycle
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, g_ui32PWMIncrement);
    
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
}
void ConfigurePWM2(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0));
    
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));

    GPIOPinConfigure(GPIO_PF3_M0PWM3);  
    GPIOPinConfigure(GPIO_PF2_M0PWM2);  
    GPIOPinTypePWM(GPIO_PORTF_BASE, LED1_PWM_PIN | LED2_PWM_PIN);

    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    
    uint32_t pwmClock = SysCtlClockGet() / 64;  
    uint32_t period = pwmClock / PWM_FREQUENCY;  

    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, period);  // LED1
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, period);  // LED2

    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, period * 50 / 100);  // LED1
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, period * 50 / 100);  // LED2

    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, true);  // LED1
    PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);  // LED2
}
void ConfigureADC(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0));
    GPIOPinTypeADC(GPIO_PORTK_BASE, GPIO_PIN_3);

    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CHANNEL);
    ADCSequenceEnable(ADC0_BASE, 3);
    ADCIntClear(ADC0_BASE, 3);
}
int main(void)
{
    uint32_t ui32Value; 
    // Configuración del reloj y perifericos
    SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);
    
    // Habilitar periféricos necesarios
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    
    // Esperar a que los periféricos estén listos
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK));
    
    // Configuración de pines GPIO
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_7);    //PC7
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);   //PN0
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);   //PN0
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);   //PF1
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_4);  //PL4
    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, IN1_PIN | IN2_PIN); //PK4,PK5
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_3);  //PL3
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_2);  //PL3
    

    // Configuración de UART
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    
    // Configurar el reloj UART con PIOSC (16 MHz)
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTConfigSetExpClk(UART0_BASE, 16000000, 9600,
        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);

    // Configuración del puerto UART para lectura de datos
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Configuración del temporizador (Timer0)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));
    
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);  // Configuración de temporizador periódico
    TimerLoadSet(TIMER0_BASE, TIMER_A, FS);  // Inicializar temporizador con frecuencia base
    IntEnable(INT_TIMER0A);  // Habilitar interrupciones del temporizador
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);  // Habilitar la interrupción de tiempo de espera del temporizador
    IntMasterEnable();  // Habilitar interrupciones globales
    
    ConfigurePWM1();
    //Ejercicio5
    ConfigurePWM2();
    ConfigureADC();
    while (1)
    {
        if (UARTCharsAvail(UART0_BASE)) {
            memset(data, 0, sizeof(data));  // Limpiar el buffer de datos
            uint32_t i = 0;
            char c;

            // Leer los caracteres recibidos por UART
            while (UARTCharsAvail(UART0_BASE)) {
                c = UARTCharGet(UART0_BASE);
                if (c == '\r') continue;
                if (c == '\n' || i >= sizeof(data) - 1)
                    break;
                data[i++] = c;
            }

            data[i] = '\0';  // Terminar la cadena con '\0'

            // Enviar de vuelta el dato recibido por UART
            UARTSendString(data);

            if (strcmp(data, "A") == 0) {
              GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_4, GPIO_PIN_4);
              UARTSendString("Comando A recibido\n"); 
              SysCtlDelay(time);
              GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_4, 0x0); 
            }
            if (strcmp(data, "B") == 0) {
              if (!temporizadorActivo) {
                  cont = 0;
                  TimerEnable(TIMER0_BASE, TIMER_A);
                  temporizadorActivo = true;
                  UARTSendString("Comando B recibido, temporizador activado\n");
              } else {
                  UARTSendString("Temporizador ya en ejecución\n");
            }
            }else{
              GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
            }
            if (strcmp(data, "C") == 0) {
              GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_3, GPIO_PIN_3);
              UARTSendString("Comando C recibido\n"); 
              SysCtlDelay(time);
              GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_3, 0x0);
            }
            if (strcmp(data, "D") == 0) {
              //Ventilador
              GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, GPIO_PIN_2);
              SysCtlDelay(time);
              GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2, 0);
              UARTSendString("Comando D recibido\n");
            }
            memset(data, 0, sizeof(data));
        }
        //Ejercicio5
        ADCProcessorTrigger(ADC0_BASE, 3);
        while (!ADCIntStatus(ADC0_BASE, 3, false)) {
        }
        ADCIntClear(ADC0_BASE, 3);

        ADCSequenceDataGet(ADC0_BASE, 3, &ui32Value);

        uint32_t period = PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1);

        uint32_t pulseWidth = (ui32Value * period) / 4095;

        // Establecer el ciclo de trabajo para ambos LEDs
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, pulseWidth);  // LED1
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, pulseWidth);  // LED2
    }
}

void Timer0IntHandler(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);  
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
    cont++;
    if (cont < 5) { 
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
    }
    
    if (cont == 5) {
        FS = 120000000 * 0.5;  
        TimerLoadSet(TIMER0_BASE, TIMER_A, FS);
        UARTSendString("Frecuencia del temporizador ajustada a 0.5 segundos\n");  // Depuración
    }

    if (cont > 5) {
        if (cont % 2 == 0) { 
            GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
        } else {

            GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0x00);  // Apagar PC7
        }
    }

    if (cont > 15) {  // Después de 10 ciclos (10 segundos)
        // Detener el temporizador después de 10 segundos
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0x00);  // Apagar PC7
        cont = 0;  // Resetear el contador
        temporizadorActivo = false;
        UARTSendString("Temporizador detenido después de 10 segundos\n");  // Depuración
        TimerDisable(TIMER0_BASE, TIMER_A);
    }
}