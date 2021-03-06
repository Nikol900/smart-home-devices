/**
 *
 * DZ-MMNET-CHARGER: Acc charger module based on MMNet101.
 *
 * Hardware PWM.
 *
**/

#include "defs.h"
#include "runtime_cfg.h"
#include "servant.h"

#include "io_pwm.h"


#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include <sys/event.h>

//#warning use timer 1 instead of 0, timer 0 is OS


#define T3 1

#define DEBUG 0

#if SERVANT_NPWM > 0


#if SERVANT_NPWM != 2
#  error This implementation supports just 2 hw PWM outputs
#endif

static uint8_t          pwm_val[SERVANT_NPWM];

static void pwm_timer_3_force_out( uint8_t port, uint8_t val );





// TODO KILLME
void set_an(unsigned char port_num, unsigned char data)
{
    if( port_num >= SERVANT_NPWM )
        return;

    pwm_val[port_num] = data;

#warning set 0 value by turning off pwm out
    pwm_timer_3_force_out( port_num, data );
}


// ----------------------------------------------------------------------
// IO
// ----------------------------------------------------------------------

#if 0
static inline void pwm_init_timer_0(void)
{
    // fast PWM, non-inverting, max clock
    //CR0 = WGM00 | WGM01 | COM01 | CS00;

    // TODO move OS clock to timer 3

    // fast PWM, non-inverting, don't touch clock - network stack uses it, apparently
    TCCR0 |= _BV(WGM00) | _BV(WGM01) | _BV(COM01);
    TCCR0 &= ~_BV(COM00);

    DDRB |= _BV(PB4);
}

static inline void pwm_set_pwm_0( uint8_t val )
{
    OCR0 = val;
}
#endif
/*
static inline void pwm_init_timer_1(void)
{
    TCCR1A = _BV(COM1B1) | _BV(WGM10);
    TCCR1B = _BV(WGM12)  | _BV(CS10);
    DDRB |= _BV(PB6);
}

static inline void pwm_set_pwm_1( uint8_t val )
{
    //OCR1BH = val;
    //OCR1BL = 0;
    OCR1BH = 0;
    OCR1BL = val;
}


static inline void pwm_init_timer_2(void)
{
    // fast PWM, non-inverting, max clock
    TCCR2 = _BV(WGM20) | _BV(WGM21) | _BV(COM21) | _BV(CS20);

    DDRB |= _BV(PB7);
}


static inline void pwm_set_pwm_2( uint8_t val )
{
    OCR2 = val;
}

*/

static void pwm_timer_3_force_out( uint8_t port, uint8_t val )
{
    uint8_t force = (val == 0) || (val == 0xFF);

    if( port )
    {
        if( force )
        {
            if( val )     PORTE |= _BV(PE3);
            else          PORTE &= ~_BV(PE3);
            TCCR3A &= 	~_BV(COM3A1); // turn off output A, let it be staedy
        }
        else
            TCCR3A |= 	_BV(COM3A1); // turn on output A, let it work as PWM

    }
    else
    {
        if( force )
        {
            if( val )     PORTE |= _BV(PE4);
            else          PORTE &= ~_BV(PE4);
            TCCR3A &= 	~_BV(COM3B1); // turn off output B, let it be staedy
        }
        else
            TCCR3A |= 	_BV(COM3B1); // turn on output B, let it work as PWM
    }
}


static inline void pwm_init_timer_3(void)
{
    TCCR3A = _BV(COM3A1) |_BV(COM3B1) | _BV(WGM30);
    TCCR3B = _BV(WGM32)  | _BV(CS30);

    DDRE |= _BV(PE3);
    DDRE |= _BV(PE4);
}


static inline void pwm_set_pwm_3a( uint8_t val )
{
    OCR3A = val;
}

static inline void pwm_set_pwm_3b( uint8_t val )
{
    OCR3B = val;
}





// ----------------------------------------------------------------------
// Report status / set value
// ----------------------------------------------------------------------

static int8_t      pwm_to_string( struct dev_minor *sub, char *out, uint8_t out_size )   	// 0 - success
{
    //return dev_uint16_to_string( sub, out, out_size, pwm_val[sub->number] );

    if( sub->number )   return dev_uint16_to_string( sub, out, out_size, TCNT1L );
    else                return dev_uint16_to_string( sub, out, out_size, OCR1BL );
}


static int8_t      pwm_from_string( struct dev_minor *sub, const char *in)         			// 0 - success
{
    int data = atoi(in);

    uint16_t channel = sub->number;

    if( channel >= SERVANT_NPWM )
        return -1;

     pwm_val[channel] = data;

    return 0;
}



// ----------------------------------------------------------------------
// Init/start
// ----------------------------------------------------------------------

static int8_t pwm_init_dev( dev_major* d )
{
    if( init_subdev( d, SERVANT_NPWM, "pwm" ) )
        return -1;

    dev_init_subdev_getset( d, pwm_from_string, pwm_to_string );

    //pwm_init_timer_0();
#if T3
    pwm_init_timer_3();
#else
    pwm_init_timer_1();
    pwm_init_timer_2();
#endif

    return 0;
}

static int8_t pwm_start_dev( dev_major* d )
{
    (void) d;

    //return -1;
    return 0;
}

static void pwd_stop_dev( dev_major* d )
{
    (void) d;

    // Turn off outputs
    TCCR0 &= ~_BV(COM00);
    TCCR0 &= ~_BV(COM01);

    TCCR2 &= ~_BV(COM21);
    TCCR2 &= ~_BV(COM20);

//    DDRB &= ~_BV(PB4);
//    DDRB &= ~_BV(PB7);

}

// ----------------------------------------------------------------------
// Test
// ----------------------------------------------------------------------

static void pwm_test_data( dev_major* d )
{
    pwm_val[0]	= rand() / (RAND_MAX / 0xff + 1);
    pwm_val[1]	= rand() / (RAND_MAX / 0xff + 1);

#if T3
    pwm_set_pwm_3a( pwm_val[0] );
    pwm_set_pwm_3b( pwm_val[1] );
#else
    pwm_set_pwm_1( pwm_val[0] );
    pwm_set_pwm_2( pwm_val[1] );
#endif
    // TODO FIXME dio resets DDRs for us, fix it there and remove here
    // TODO really? try to remove it here
    DDRB |= _BV(PB4);
    DDRB |= _BV(PB7);
}


#endif // SERVANT_NPWM > 0



dev_major io_pwm =
{
    .name = "pwm",

#if SERVANT_NPWM > 0
    .init	= pwm_init_dev,
    .start	= pwm_start_dev,
    .stop	= pwd_stop_dev, // TODO
    .timer	= pwm_test_data,
#endif

    .started	= 0,

    .to_string 	 = 0,
    .from_string = 0,

    .minor_count = SERVANT_NPWM,
    .subdev 	 = 0,
};






















