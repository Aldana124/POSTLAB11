/* 
 * File:   POSTMASTER.c
 * Author: diego
 *
 * Created on 12 de mayo de 2022, 02:59 PM
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>

/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000 //Frecuencia Oscilador
#define FLAG_SPI 0xFF
/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
uint8_t POT = 0;               //Valores de pot en 8bits

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if(PIR1bits.ADIF){              // Revisión de ADC
        if(ADCON0bits.CHS == 0){    // Selección de canal para AN0        
           POT = ADRESH;            // Lectura analógica a contador de POT
        }
        PIR1bits.ADIF = 0;          // Limpieza de bandera
     }
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){
        if(ADCON0bits.GO == 0){       // Si no hay conversión...
            ADCON0bits.GO = 1;        // se inicia la conversión
        }
        
            PORTAbits.RA1 = 1;       // Inhibición de SS de slave
            PORTAbits.RA2 = 1;       // Inhibición de SS de slave
            __delay_ms(10);          // Delay para que logre identificar el cambio en la habilitación/inhibición del slave y se comunique
            PORTAbits.RA1 = 0;       // habilitación de SS de slave
            SSPBUF = 0xFF;           // Valor de comunicación ingresado a buffer
            while(!SSPSTATbits.BF){} // Espera en transcurso de envio de información (Master-Slave)
            
            PORTAbits.RA1 = 1;       // Inhibición de SS de slave
            PORTAbits.RA2 = 1;       // Inhibición de SS de slave
            __delay_ms(10);          // Delay para que logre identificar el cambio en la habilitación/inhibición del slave y se comunique
            PORTAbits.RA1 = 0;       // habilitación de SS de slave
            
            while(!SSPSTATbits.BF){} // Espera en transcurso de envio de información (Slave-Master)
            PORTD = SSPBUF;          // Mostrar valor de contador botones en PORTD (que viene del slave)
            
            PORTAbits.RA2 = 1;       // Inhibición de SS de slave
            PORTAbits.RA1 = 1;       // Inhibición de SS de slave
            __delay_ms(10);          // Delay para que logre identificar el cambio en la habilitación/inhibición del slave y se comunique
            PORTAbits.RA2 = 0;       // habilitación de SS de slave
           
            SSPBUF = POT;            // Carga de contador de POT en buffer
            while(!SSPSTATbits.BF){} // Espera en transcurso de envio de información (Master-Slave)
            
            PORTAbits.RA2 = 1;       // Inhibición de SS de slave
            PORTAbits.RA1 = 1;       // Inhibición de SS de slave
            __delay_ms(10);          // Delay para que logre identificar el cambio en la habilitación/inhibición del slave y se comunique
            PORTAbits.RA2 = 0;       // habilitación de SS de slave
            __delay_ms(10);          // Delay para que logre identificar el cambio en la habilitación/inhibición del slave y se comunique
    }
    return;
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){
    //Configuración Entradas/Salidas
    ANSEL = 0b00000001;         // AN0 como entrada analógicas
    ANSELH = 0;                 // I/O digitales
    TRISA = 0b00000001;         // SS y RA0 como entradas       
    PORTA = 0;                  // Limpieza PORTA
    TRISB = 0;                  // Habilitaciónde PORTB como salida
    PORTB = 0;                  // Limpieza PORTB
    TRISD = 0;                  // Habilitaciónde PORTD como salida
    PORTD = 0;                  // Limpieza PORTD
 
    
    //Configuración Oscilador
    OSCCONbits.IRCF = 0b100;    // 1MHz
    OSCCONbits.SCS = 1;         // Reloj interno
    
    //Configuración ADC
    ADCON0bits.ADCS = 0b00;     // Fosc/2
    ADCON1bits.VCFG0 = 0;       // VDD *Referencias internas
    ADCON1bits.VCFG1 = 1;       // VSS
    ADCON0bits.CHS = 0b0000;    // Seleccionamos AN0
    ADCON1bits.ADFM = 0;        // Justificado a la izquierda
    ADCON0bits.ADON = 1;        // Habilitamos modulo ADC
    __delay_us(40);

    // Configuración de SPI (MAESTRO)
    TRISC = 0b00010000;         // -> SDI entrada, SCK y SD0 como salida
    PORTC = 0;
    // SSPCON <5:0>
    SSPCONbits.SSPM = 0b0000;   // -> SPI Maestro, Reloj -> Fosc/4 (250kbits/s)
    SSPCONbits.CKP = 0;         // -> Reloj inactivo en 0
    SSPCONbits.SSPEN = 1;       // -> Habilitamos pines de SPI
    // SSPSTAT<7:6>
    SSPSTATbits.CKE = 1;        // -> Dato enviado cada flanco de subida
    SSPSTATbits.SMP = 1;        // -> Dato al final del pulso de reloj (MAESTRO)
    SSPBUF = POT;               // Enviamos un dato inicial   
    
    // Configuraciones de interrupciones
    PIR1bits.ADIF = 0;          // Limpieza bandera ADC
    PIE1bits.ADIE = 1;          // Habilitación interrupciones ADC
    INTCONbits.GIE = 1;         // Habilitación interrupciones globales
    INTCONbits.PEIE = 1;        // Habilitación interrupciones de perifericos
}
