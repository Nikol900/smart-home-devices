/**
 *
 * DZ-MMNET-MODBUS: Modbus/TCP I/O module based on MMNet101.
 *
 * HTTPD/CGI code for web configuration.
 *
 * URL: cgi-bin/form.cgi
 *
**/


#include "defs.h"
#include "runtime_cfg.h"

#include "web.h"




static void form_element( FILE * stream, char *title, char *field_name, char *curr_value );


static char * itox( int i )
{
    static char x[4];
    sprintf( x, "%02X", i );
    return x;
}

static int htoi( const char *v )
{
    int i = -1;
    sscanf( v, "%x", &i );
    return i;
}

#define DEC_X( __name_, __dest_ ) do { if( 0 == strcmp( pname, (__name_) ) ) ee_cfg.__dest_ = htoi( pvalue ); } while(0)

static void (*boot_me)(void) = 0;


int ShowForm(FILE * stream, REQUEST * req)
{

    char buf[64];
    static char modified = 0;

    web_header_200(stream, req);

    HTML("<HTML><head><link href=\"/screen.css\" rel=\"stylesheet\" type=\"text/css\"></head><BODY>\r\n");
    HTML("<H2>Configuration</H2><BR>\r\n");


    if (req->req_query)
    {
        char *pname;
        char *pvalue;
        int i;
        int count;

        //strncpy( tmp, req->req_query, 99 );

        count = NutHttpGetParameterCount(req);

        for (i = 0; i < count; i++) {
            pname = NutHttpGetParameterName(req, i);
            pvalue = NutHttpGetParameterValue(req, i);

            modified = 1;

            /* Send the parameters back to the client. */
            //fprintf_P(stream, PSTR("%s: %s<BR>\r\n"), pname, pvalue);

            DEC_X( "ddrb", ddr_b );
            DEC_X( "ddrd", ddr_d );
            DEC_X( "ddre", ddr_e );
            DEC_X( "ddrf", ddr_f );
            DEC_X( "ddrg", ddr_g );


            DEC_X( "initb", start_b );
            DEC_X( "initd", start_d );
            DEC_X( "inite", start_e );
            DEC_X( "initf", start_f );
            DEC_X( "initg", start_g );

            if( 0 == strcmp( pname, "ip_addr" ) )		    ee_cfg.ip_addr	= inet_addr( pvalue );
            if( 0 == strcmp( pname, "ip_mask" ) )		    ee_cfg.ip_mask	= inet_addr( pvalue );
            if( 0 == strcmp( pname, "ip_route" ) )		    ee_cfg.ip_route	= inet_addr( pvalue );

            if( 0 == strcmp( pname, "mac5" ) )		    	    ee_cfg.mac_addr[5]	= atoi( pvalue );
            if( 0 == strcmp( pname, "tz" ) )		    	    ee_cfg.timezone	= atoi( pvalue );

            if( 0 == strcmp( pname, "ip_syslog" ) )		    ee_cfg.ip_syslog	= inet_addr( pvalue );
            if( 0 == strcmp( pname, "ip_nntp" ) )		    ee_cfg.ip_nntp	= inet_addr( pvalue );

            if( 0 == strcmp( pname, "eeprom" ))
            {
                if( 0 == strcmp( pvalue, "save" ) )
                {
                    if( 0 == runtime_cfg_eeprom_write() )
                    {
                        modified = 0;
                        HTML("<p>EEPROM write done</p>");
                    }
                    else

                        HTML("<p>EEPROM write FAILED!</p>");

                }

                if( 0 == strcmp( pvalue, "load" ) )
                {
                    if( 0 == runtime_cfg_eeprom_read() )
                    {
                        modified = 0;
                        HTML("<p>EEPROM read done</p>");
                    }
                    else
                        HTML("<p>EEPROM read FAILED!</p>");

                }

                if( 0 == strcmp( pvalue, "init" ) )
                {
                    init_runtime_cfg();         // Load defaults
                    HTML("<p>Done setting factory defaults</p>");
                }
            }

            if( 0 == strcmp( pname, "os" ))
            {
                if( 0 == strcmp( pvalue, "boot" ) )
                {
                    cli();
                    boot_me();
                }
            }

        }
    }

    HTML("<form>\r\n");
    HTML("<table>\r\n");


    sprintf( buf, "%d", (signed char)ee_cfg.timezone );
    form_element( stream, "TimeZone", "tz", buf );

    sprintf( buf, "%u", ee_cfg.mac_addr[5] );
    form_element( stream, "MAC byte", "mac5", buf );

    form_element( stream, "IP Address", "ip_addr", inet_ntoa(ee_cfg.ip_addr) );
    form_element( stream, "Net Mask", "ip_mask", inet_ntoa(ee_cfg.ip_mask) );
    form_element( stream, "IP gateway", "ip_route", inet_ntoa(ee_cfg.ip_route) );

    form_element( stream, "Syslog server", "ip_syslog", inet_ntoa(ee_cfg.ip_syslog) );
    form_element( stream, "NNTP server", "ip_nntp", inet_ntoa(ee_cfg.ip_nntp) );

    form_element( stream, "DDR B", "ddrb", itox(ee_cfg.ddr_b) );
    form_element( stream, "DDR D", "ddrd", itox(ee_cfg.ddr_d) );
    form_element( stream, "DDR E", "ddre", itox(ee_cfg.ddr_e) );
    form_element( stream, "DDR F", "ddrf", itox(ee_cfg.ddr_f) );
    form_element( stream, "DDR G", "ddrg", itox(ee_cfg.ddr_g) );

    form_element( stream, "Init B", "initb", itox(ee_cfg.start_b) );
    form_element( stream, "Init D", "initd", itox(ee_cfg.start_d) );
    form_element( stream, "Init E", "inite", itox(ee_cfg.start_e) );
    form_element( stream, "Init F", "initf", itox(ee_cfg.start_f) );
    form_element( stream, "Init G", "initg", itox(ee_cfg.start_g) );


    HTML("</table><BR>");
    HTML("<input type = \"submit\" value=\"Send\" size=\"8\">\r\n");
    HTML("</form><\r\n");



    HTML("<p></p>");
    if( modified )
        HTML("<a href=\"/cgi-bin/form.cgi?eeprom=save\"><button>Save to EEPROM</button></a> ");
    HTML("<a href=\"/cgi-bin/form.cgi?eeprom=load\"><button>Load from EEPROM</button></a> ");
    HTML("<a href=\"/cgi-bin/form.cgi?eeprom=init\"><button>Load factory defaults</button></a> ");
    HTML("<a href=\"/cgi-bin/form.cgi?os=boot\"><button>Reboot</button></a> ");
    HTML("<p></p>");

    HTML("<p><a href=\"/\">Return to main</a></p>");
    HTML("</BODY></HTML></p>");

    fflush(stream);

    return 0;
}


static void form_element( FILE * stream, char *title, char *field_name, char *curr_value )
{
    static prog_char fmt[] =
        "<tr> <TD height=\"32\" width=\"160\">&nbsp;%s:&nbsp;</TD> <TD height=\"32\" width=\"140\"><input type=\"text\" name=\"%s\" size=\"16\" value=\"%s\"></TD> </tr>";

    fprintf_P( stream, fmt, title, field_name, curr_value );
}





