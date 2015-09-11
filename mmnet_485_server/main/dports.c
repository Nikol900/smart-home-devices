/**
 *
 * DZ-MMNET-MODBUS: Modbus/TCP I/O module based on MMNet101.
 *
 * Digital ports I/O.
 *
**/

#include "util.h"
#include "dports.h"
#include "defs.h"
#include "net_io.h"

#include "runtime_cfg.h"

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>


#if SERVANT_NDIG > 0

static unsigned char get_dig_in(unsigned char port_num);

static unsigned char get_dig_out(unsigned char port_num);
static void set_dig_out(unsigned char port_num, unsigned char data);

static unsigned char get_ddr(unsigned char port_num);
static void set_ddr(unsigned char port_num, unsigned char data);



#define add_exclusion_pin(p,b) add_exclusion_mask(p,_BV(b))

// Contains reverse bitmask for each port.
// bit == 1 means do not trigger data send on bit value changed
static unsigned char exclude_port_bits[SERVANT_NDIG];

// exclPos is 0 for port A and so on
static void add_exclusion_mask( unsigned char exclPos, unsigned char bitmask )
{
    if( exclPos >= SERVANT_NDIG ) return;
    exclude_port_bits[exclPos] |= bitmask;
}




void dio_init(void)
{
    // Totally unavailable on MMNET101 ports

    add_exclusion_mask( 0, 0xFF ); // Port A
    add_exclusion_mask( 2, 0xFF ); // Port C


    add_exclusion_pin( ETHERNET_INTERRUPT_EXCLPOS, ETHERNET_INTERRUPT_PIN );

    add_exclusion_pin( DATAFLASH_EXCLPOS, DATAFLASH_SCK_PIN );
    add_exclusion_pin( DATAFLASH_EXCLPOS, DATAFLASH_MOSI_PIN );
    add_exclusion_pin( DATAFLASH_EXCLPOS, DATAFLASH_MISO_PIN );
    add_exclusion_pin( DATAFLASH_EXCLPOS, DATAFLASH_CS_PIN );

#if ALARM_OUTPUTS_USED
    add_exclusion_pin( SERVANT_WARN_EXCLPOS, SERVANT_WARN_PIN );
    add_exclusion_pin( SERVANT_ALARM_EXCLPOS, SERVANT_ALARM_PIN );
#endif

    add_exclusion_pin( OW_EXCLPOS, OW_PIN );
    add_exclusion_pin( DHT_EXCLPOS, DHT_PIN );

    // todo fixme
    //add_exclusion_mask( UART1_EXCL_EXCLPOS, UART1_EXCL_MASK );
    //add_exclusion_mask( UART2_EXCL_EXCLPOS, UART2_EXCL_MASK );

#if HALF_DUPLEX
    add_exclusion_pin( HALF_DUPLEX1_EXCLPOS, HALF_DUPLEX1_PIN );
    add_exclusion_pin( HALF_DUPLEX2_EXCLPOS, HALF_DUPLEX2_PIN );
#endif

    add_exclusion_pin( LED_EXCLPOS, LED );

#ifdef FAIL_LED_EXCLPOS
    add_exclusion_pin( FAIL_LED_EXCLPOS, FAIL_LED );
#endif


    unsigned char i;
    for( i = 0; i < SERVANT_NDIG; i++ )
    {
        set_ddr( i, 0 );
        //set_dig( i, 0xFF );
    }

    // TODO fixme read DDR mask from EEPROM

    // Set port values BEFORE enabling DDE
    PORTB = ee_cfg.start_b;
    PORTD = ee_cfg.start_d;
    PORTE = ee_cfg.start_e;
    PORTF = ee_cfg.start_f;
    PORTG = ee_cfg.start_g;

    DDRB = ee_cfg.ddr_b;
    DDRD = ee_cfg.ddr_d;
#ifdef __AVR_ATmega128__
    DDRE = ee_cfg.ddr_e;
    DDRF = ee_cfg.ddr_f;
    DDRG = ee_cfg.ddr_g;
#endif


    // Now set dde for pins we use in a dedicated way

#if HALF_DUPLEX
    HALF_DUPLEX1_DDR |= _BV(HALF_DUPLEX1_PIN);
    HALF_DUPLEX2_DDR |= _BV(HALF_DUPLEX2_PIN);
#endif

    led_ddr_init(); // again - first time was in main
}









// Direct
unsigned char   dio_read_port( unsigned char port ) { return get_dig_in(port); }

void
dio_write_port( unsigned char port, unsigned char new_bits )
{
    if( port >= SERVANT_NDIG ) return;
    cli();

    unsigned char prev = get_dig_out(port);

    new_bits &= ~exclude_port_bits[port]; // reset bits that we can NOT change

    prev &= exclude_port_bits[port]; // reset bits that we can change
    prev |= new_bits;

    set_dig_out( port, prev );

    sei();
}


unsigned char   dio_get_port_ouput_mask( unsigned char port ) { return get_ddr(port); }

void
dio_set_port_ouput_mask( unsigned char port, unsigned char new_mask )
{
    if( port >= SERVANT_NDIG ) return;
    cli();

    unsigned char prev = get_ddr(port);

    new_mask &= ~exclude_port_bits[port]; // reset bits that we can NOT change

    prev &= exclude_port_bits[port]; // reset bits that we can change
    prev |= new_mask;

    set_ddr( port, prev );

    sei();
}




unsigned char
dio_read_port_bit( unsigned char port, unsigned char nBit )
{
    return 0x1 & (dio_read_port(port) >> nBit );
}

void
dio_write_port_bit( unsigned char port, unsigned char nBit, unsigned char value )
{
    cli();
    unsigned char prev = get_dig_out( port );

    if( value )         prev |= _BV(nBit);
    else         	prev &= ~(_BV(nBit));

    dio_write_port( port, prev );
    sei();
}

unsigned char
dio_get_port_ouput_mask_bit( unsigned char port, unsigned char nBit )
{
    return 0x1 & (get_ddr(port) >> nBit);
}


void
dio_set_port_ouput_mask_bit( unsigned char port, unsigned char nBit, unsigned char value )
{
    cli();
    unsigned char prev = get_ddr( port );

    if( value )         prev |= _BV(nBit);
    else         	prev &= ~(_BV(nBit));

    dio_set_port_ouput_mask( port, prev );
    sei();
}




















static unsigned char get_dig_in(unsigned char port_num)
{
    if( port_num >= SERVANT_NDIG ) return 0;
    switch (port_num) {
#ifdef __AVR_ATmega128__
    case 0:		return PINA;
#endif
    case 1:		return PINB;
    case 2:		return PINC;
    case 3:		return PIND;
#ifdef __AVR_ATmega128__
    case 4:		return PINE;
    case 5:		return PINF;
    case 6:		return PING;
#endif
    }
    return 0;
}





static void set_dig_out(unsigned char port_num, unsigned char data)
{
    if( port_num >= SERVANT_NDIG ) return;
    switch (port_num) {
#ifdef __AVR_ATmega128__
    case 0:		PORTA=data;		break;
#endif
    case 1:		PORTB=data;		break;
    case 2:		PORTC=data;		break;
    case 3:		PORTD=data;		break;
#ifdef __AVR_ATmega128__
    case 4:		PORTE=data;		break;
    case 5:		PORTF=data;		break;
    case 6:		PORTG=data;		break;
#endif
    }
}


static unsigned char get_dig_out(unsigned char port_num)
{
    if( port_num >= SERVANT_NDIG ) return 0;
    switch (port_num) {
#ifdef __AVR_ATmega128__
    case 0:		return PORTA;
#endif
    case 1:		return PORTB;
    case 2:		return PORTC;
    case 3:		return PORTD;
#ifdef __AVR_ATmega128__
    case 4:		return PORTE;
    case 5:		return PORTF;
    case 6:		return PORTG;
#endif
    }
    return 0;
}


static void set_ddr(unsigned char port_num, unsigned char data)
{
    if( port_num >= SERVANT_NDIG ) return;
    switch (port_num) {
#ifdef __AVR_ATmega128__
    case 0:		DDRA=data;		break;
#endif
    case 1:		DDRB=data;		break;
    case 2:		DDRC=data;		break;
    case 3:		DDRD=data;		break;
#ifdef __AVR_ATmega128__
    case 4:		DDRE=data;		break;
    case 5:		DDRF=data;		break;
    case 6:		DDRG=data;		break;
#endif
    }
}

static unsigned char get_ddr(unsigned char port_num)
{
    if( port_num >= SERVANT_NDIG ) return 0;
    switch (port_num) {
#ifdef __AVR_ATmega128__
    case 0:		return DDRA;
#endif
    case 1:		return DDRB;
    case 2:		return DDRC;
    case 3:		return DDRD;
#ifdef __AVR_ATmega128__
    case 4:		return DDRE;
    case 5:		return DDRF;
    case 6:		return DDRG;
#endif
    }
    return 0;
}





#endif // SERVANT_NDIG > 0













