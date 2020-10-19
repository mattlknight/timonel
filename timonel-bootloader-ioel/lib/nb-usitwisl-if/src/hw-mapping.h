/*
 *  hw-mapping.h
 *  =================================
 *  Library hardware mapping
 *  ---------------------------------
 *  Version: 1.0.2 / 2020-09-19
 *  ---------------------------------
 */

#ifndef HARDWARE_MAPPING_H
#define HARDWARE_MAPPING_H

// USI TWI hardware mapping
// ------------------------
// DDR_USI = I2C data direction register
// PORT_USI = I2C output register
// PIN_USI = I2C input register
// PORT_USI_SDA = I2C SDA output register
// PORT_USI_SCL = I2C SCL output register
// PIN_USI_SDA = I2C SDA input register
// PIN_USI_SCL = I2C SDL input register
// TWI_START_COND_FLAG = Status register flag: indicates an I2C START condition on the bus (can trigger an interrupt)
// USI_OVERFLOW_FLAG = Status register flag: indicates a complete bit reception/transmission (can trigger an interrupt)
// TWI_STOP_COND_FLAG = Status register flag: indicates an I2C STOP condition on the bus
// TWI_COLLISION_FLAG = Status register flag: indicates a data output collision on the bus
// TWI_START_COND_INT = Control register bit: defines whether an I2C START condition triggers an interrupt
// USI_OVERFLOW_INT = Control register bit: defines whether a USI 4-bit counter overflow triggers an interrupt

// ATtinyX5
#if defined(__AVR_ATtiny25__) | \
    defined(__AVR_ATtiny45__) | \
    defined(__AVR_ATtiny85__)
#define DDR_USI DDRB
#define PORT_USI PORTB
#define PIN_USI PINB
#define PORT_USI_SDA PB0
#define PORT_USI_SCL PB2
#define PIN_USI_SDA PINB0
#define PIN_USI_SCL PINB2
#define TWI_START_COND_FLAG USISIF
#define USI_OVERFLOW_FLAG USIOIF
#define TWI_STOP_COND_FLAG USIPF
#define TWI_COLLISION_FLAG USIDC
#define TWI_START_COND_INT USISIE
#define USI_OVERFLOW_INT USIOIE
#endif

// ATtinyX4
#if defined(__AVR_ATtiny24__) | \
    defined(__AVR_ATtiny44__) | \
    defined(__AVR_ATtiny84__)
#define DDR_USI DDRA
#define PORT_USI PORTA
#define PIN_USI PINA
#define PORT_USI_SDA PORTA6
#define PORT_USI_SCL PORTA4
#define PIN_USI_SDA PINA6
#define PIN_USI_SCL PINA4
#define TWI_START_COND_FLAG USISIF
#define USI_OVERFLOW_FLAG USIOIF
#define TWI_STOP_COND_FLAG USIPF
#define TWI_COLLISION_FLAG USIDC
#define TWI_START_COND_INT USISIE
#define USI_OVERFLOW_INT USIOIE
#endif

// ATtiny2313
#if defined(__AVR_ATtiny2313__)
#define DDR_USI DDRB
#define PORT_USI PORTB
#define PIN_USI PINB
#define PORT_USI_SDA PB5
#define PORT_USI_SCL PB7
#define PIN_USI_SDA PINB5
#define PIN_USI_SCL PINB7
#define TWI_START_COND_FLAG USISIF
#define USI_OVERFLOW_FLAG USIOIF
#define TWI_STOP_COND_FLAG USIPF
#define TWI_COLLISION_FLAG USIDC
#define TWI_START_COND_INT USISIE
#define USI_OVERFLOW_INT USIOIE
#endif

// ATtinyX7
#if defined(__AVR_ATtiny87__) | \
    defined(__AVR_ATtiny167__)
#define DDR_USI DDRB
#define PORT_USI PORTB
#define PIN_USI PINB
#define PORT_USI_SDA PB0
#define PORT_USI_SCL PB2
#define PIN_USI_SDA PINB0
#define PIN_USI_SCL PINB2
#define TWI_START_COND_FLAG USISIF
#define USI_OVERFLOW_FLAG USIOIF
#define TWI_STOP_COND_FLAG USIPF
#define TWI_COLLISION_FLAG USIDC
#define TWI_START_COND_INT USISIE
#define USI_OVERFLOW_INT USIOIE
#endif

// ATtiny26
#if defined(__AVR_ATtiny26__)
#define DDR_USI DDRB
#define PORT_USI PORTB
#define PIN_USI PINB
#define PORT_USI_SDA PB0
#define PORT_USI_SCL PB2
#define PIN_USI_SDA PINB0
#define PIN_USI_SCL PINB2
#define TWI_START_COND_FLAG USISIF
#define USI_OVERFLOW_FLAG USIOIF
#define TWI_STOP_COND_FLAG USIPF
#define TWI_COLLISION_FLAG USIDC
#define TWI_START_COND_INT USISIE
#define USI_OVERFLOW_INT USIOIE
#endif

// ATtinyX61
#if defined(__AVR_ATtiny261__) | \
    defined(__AVR_ATtiny461__) | \
    defined(__AVR_ATtiny861__)
#define DDR_USI DDRB
#define PORT_USI PORTB
#define PIN_USI PINB
#define PORT_USI_SDA PB0
#define PORT_USI_SCL PB2
#define PIN_USI_SDA PINB0
#define PIN_USI_SCL PINB2
#define USI_START_COND_INT USISIF
#define USI_START_VECTOR USI_START_vect
#define USI_OVERFLOW_VECTOR USI_OVF_vect
#endif

// ATmega
#if defined(__AVR_ATmega165__) |  \
    defined(__AVR_ATmega169__) |  \
    defined(__AVR_ATmega325__) |  \
    defined(__AVR_ATmega3250__) | \
    defined(__AVR_ATmega645__) |  \
    defined(__AVR_ATmega6450__) | \
    defined(__AVR_ATmega329__) |  \
    defined(__AVR_ATmega3290__)
#define DDR_USI DDRE
#define PORT_USI PORTE
#define PIN_USI PINE
#define PORT_USI_SDA PE5
#define PORT_USI_SCL PE4
#define PIN_USI_SDA PINE5
#define PIN_USI_SCL PINE4
#define USI_START_COND_INT USISIF
#define USI_START_VECTOR USI_START_vect
#define USI_OVERFLOW_VECTOR USI_OVERFLOW_vect
#endif

#endif  // HARDWARE_MAPPING_H
