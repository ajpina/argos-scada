


#include "modbus-tcp.h"
#include "json.h"

int               noTickTest     =    0;
char            * configFilename = NULL;
modbus_t        * mbContext      = NULL;
uint8_t         * mbRequest8     = NULL;
uint16_t        * mbRequest16    = NULL;

#define REQUEST_BUFFER_SIZE  1024


void processCommandLine( int argc, char * * argv ) {
int moreOptions = 1;
int i;

  while ( moreOptions ) {
    i = getopt( argc, argv, "tj:" );
    switch ( i ) {
      case -1  :   /* No more valid options */
                   moreOptions = 0;
                   break; 
      case 't' :   /* Do not test tick frequency  */
                   noTickTest = 1;
                   break;
      case 'j' :   /* Configuration file name */
                   configFilename = strdup( optarg );             
    }
  }
  if ( optind != argc ) {
     printf("Unrecognized options ignored!\n");
  }
  if ( configFilename == NULL ) {
     printf("You did not specify a configuration file!\n");
     exit(0);   
  }
}

void CHECK_ALL_ZERO( int l ) {
int i;
   for( i = 0; i < l; i++ ) {
      if ( mbRequest8[i] != 0x00 ) {
         printf("Non-zero result at index %d %02x\n",i,mbRequest8[i] );
      }
   }
}

void CHECK_ALL_ONE( int l ) {
int i;
   for( i = 0; i < l; i++ ) {
      if ( mbRequest8[i] != 0x01 ) {
         printf("Non-one result at index %d %02x\n",i,mbRequest8[i] );
      }
   }
}

void CHECK_AGAINST( int l, char * S ) {
   for( int i = 0; i < l; i++ ) {
      if ( mbRequest8[i] != S[i] ) {
         printf("Unequal result at index %d %02x  expecting %02x\n",i,mbRequest8[i], S[i] );
      }
   }
}

#define CLEAR(x) memset( mbRequest8, x, REQUEST_BUFFER_SIZE );

void testSingleBits( void ) {
int i;
/* Test 1 */
   CLEAR( 0x00 )
   i = modbus_read_bits( mbContext, 5, 32, mbRequest8 );  /* Function 01  */
   CHECK_ALL_ZERO( i );

/* Test 2 */   
   CLEAR( 0x01 )
   i = modbus_write_bits( mbContext, 5, 32, mbRequest8 ); /* Function 15  */
   CLEAR( 0x00 )
   i = modbus_read_bits( mbContext, 5, 32, mbRequest8 );  /* Function 01  */
   CHECK_ALL_ONE( i ); 
/* Test 3 */
   CLEAR( 0x00 )
   i = modbus_write_bit( mbContext, 20, ON  );           /* Function 05  */
   i = modbus_write_bit( mbContext, 21, ON );            /* Function 05  */
   i = modbus_write_bit( mbContext, 22, OFF );           /* Function 05  */
   CLEAR( 0x00 )
   i = modbus_read_bits( mbContext, 20, 3, mbRequest8 );  /* Function 01  */ 
   CHECK_AGAINST( i, "\x01\x01\x00" );
/* Test 4 */
   CLEAR( 0x00 )
   i = modbus_read_input_bits( mbContext, 20, 3, mbRequest8 );  /* Function 02  */
   CHECK_AGAINST( i, "\x00\x00\x00" );
   CLEAR( 0xFF )
   i = modbus_read_input_bits( mbContext, 20, 3, mbRequest8 );  /* Function 02  */
   CHECK_AGAINST( i, "\x00\x00\x00" );
}

void testRegisters( void ) {
int i,j;
/* Test 5 */
   mbRequest16[0] = 9;
   mbRequest16[1] = 8;
   mbRequest16[2] = 7;
   mbRequest16[3] = 6;
   mbRequest16[4] = 6;
   i =modbus_read_input_registers( mbContext, 26, 5, mbRequest16 ); /* Function 04 */
   for( j = 0; j < i; j++ ) {
     if ( mbRequest16[j] != 0 ) {
        printf("Input Register failure [%d]  [%d]\n",j, mbRequest16[j] );
     }
   } 
/* Test 6 */
   i = modbus_read_registers(  mbContext,  40, 1, &mbRequest16[0] );  /* Function 03  */
   i = modbus_write_register( mbContext,   40, 1234 );             /* Function 06  */
   i = modbus_read_registers(  mbContext,  40, 1, &mbRequest16[1] );  /* Function 03  */
   if ( mbRequest16[1] != 1234 ) {
      printf("ERROR : Register 40 BEFORE %d AFTER %d\n", mbRequest16[0], mbRequest16[1] );
   }
   i = modbus_read_registers(  mbContext,  43, 1, &mbRequest16[0] );
   i = modbus_write_register( mbContext,  43, 54321 );
   i = modbus_read_registers(  mbContext,  43, 1, &mbRequest16[1] );
   if ( mbRequest16[1] != 54321 ) {
      printf("ERROR : Register 43 BEFORE %d AFTER %d\n", mbRequest16[0], mbRequest16[1] );
   }   
   
   


}


int main( int argc, char * * argv ) {
int i, tF;

char     *  ipAddress = NULL;
char     *  ipPort    = NULL;



/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Process the Command Line Parameters
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
   processCommandLine( argc, argv );
   
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Make sure the configuration file is valid JSON
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
   if ( ! json_validator( configFilename ) ) {
      printf("The configuration file you have supplied is NOT valid json syntax!\n");
      printf("Correct the errors before re-trying ...\n");
      exit( 4 ); 
   }
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Process the json configuration file
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */ 
   json_tokenizer( configFilename );
   json_buildDom( );
   
   ipAddress         = json_extract("$.ip");
   ipPort            = json_extract("$.port");
   tF                = atoi( json_extract("$.tickFrequency") );
   
   json_finalize();

   if ( ! noTickTest ) {
     if ( tF != 0 ) {
        printf("The MODBUS emulator can only be tested when the tickFrequency is 0!\n");
        printf("Reset the configuration file and restart the emulator and the test!\n");
        exit(0);
     }   
   }
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Initialize the MODBUS library
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
   mbRequest8  = ( uint8_t * )  calloc( REQUEST_BUFFER_SIZE, sizeof( uint8_t ) );
   mbRequest16 = ( uint16_t * ) calloc( REQUEST_BUFFER_SIZE, sizeof( uint16_t ) );

   mbContext = modbus_new_tcp_pi( ipAddress, ipPort );
   i = modbus_set_debug( mbContext, TRUE );
   if ( i != 0 ) {
      printf("MODBUS_SET_DEBUG did not work!\n");
      exit(0);
   }
   
   i = modbus_connect( mbContext );
   
   testSingleBits( );
   
   testRegisters( );
   
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * When the test is over we want to keep the connection open for a while
 * so in a stress test we can see the servers behavior ...
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
    sleep( 60 );   /* 1 minute should do the trick ... */   
   
   i = modbus_flush( mbContext );
   
   modbus_close( mbContext );
   
   modbus_free( mbContext );
   return 0;
}
