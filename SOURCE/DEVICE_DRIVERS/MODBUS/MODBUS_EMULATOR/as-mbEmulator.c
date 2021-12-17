#include "argos.h"

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "modbus-tcp.h"
#include "json.h"

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++
 * The size of the tables we will allocate is tightly
 * co-ordinated with the values in the MODBUS standard
 * and are defined in the file "modbus.h"
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
 
#define MB_NB_BITS             MODBUS_MAX_READ_BITS         /*  Discrete Bits    1 bit  Read Only  */
#define MB_NB_INBITS           MODBUS_MAX_WRITE_BITS        /*  Coils            1 bit  Read Write */
#define MB_NB_REGS             MODBUS_MAX_READ_REGISTERS    /*  Registers       16 bit  Read Only  */
#define MB_NB_INREGS           MODBUS_MAX_WRITE_REGISTERS   /*  Input Registers 16 bit  Read Write */
#define REQUEST_BUFFER_SIZE    MODBUS_MAX_PDU_LENGTH+16

int                 tickFrequency  =    0,
                    forever        =    1,
                    debugMode      =    0,
                    fdmax          =   -1;
                    
long                tickCounter    =   0L;

float               mbSeed         = FLT_MIN;

char              * configFilename = NULL;

fd_set              allSockets;
fd_set              readySockets;

modbus_t          * mbContext      = NULL;
modbus_mapping_t  * mbData         = NULL;


void tickTock( int sig ) {
uint8_t  oneByte;
uint16_t twoBytes;

   tickCounter++;
   if ( tickCounter > LONG_MAX - 100 ) {
      tickCounter = 1L;
   } 
   
   mbSeed   = sinf( mbSeed );
   oneByte  =  ( uint8_t )  ( mbSeed * SCHAR_MAX );
   twoBytes =  ( uint16_t ) ( mbSeed * SHRT_MAX );
   
   for( int i = MB_NB_BITS - 1; i > 0; i-- ) {
      mbData->tab_bits[i] = mbData->tab_bits[i-1];
   }
   mbData->tab_bits[0] = oneByte; 
   
   for( int i = MB_NB_INBITS -1; i > 0; i-- ) {
      mbData->tab_input_bits[i] = mbData->tab_input_bits[i-1];
   }
   mbData->tab_input_bits[0] = oneByte;
   
   for( int i = MB_NB_REGS -1; i > 0; i-- ) {
      mbData->tab_registers[i] = mbData->tab_registers[i-1];
   }
   mbData->tab_registers[0] = twoBytes;
   
   for( int i = MB_NB_INREGS -1; i > 0; i-- ) {
      mbData->tab_input_registers[i] = mbData->tab_input_registers[i-1];
   }
   mbData->tab_input_registers[0] = twoBytes;
   
   if ( mbSeed > ( float ) ( FLT_MAX - 100.0 ) ) {
      mbSeed = FLT_MIN;
   }
   mbSeed += 0.1;
   
   alarm( tickFrequency );
}

void loopStopper( int sig ) {
  forever = 0;
}

void toggleDebug( int sig ) {
int i;
   debugMode = debugMode?0:1;
   if ( debugMode ) { 
     if ( debugMode ) {
       i = modbus_set_debug( mbContext, TRUE );
     } else {
        i = modbus_set_debug( mbContext, FALSE );
     }
     if ( i != 0 ) {
        syslog( LOG_ERR, "MODBUS_SET_DEBUG did not work!\n");
        exit(0);
     } 
   }
}

void processCommandLine( int argc, char * * argv ) {
int moreOptions = 1;
int i;

  while ( moreOptions ) {
    i = getopt( argc, argv, "dj:" );
    switch ( i ) {
      case -1  :   /* No more valid options */
                   moreOptions = 0;
                   break; 
      case 'd' :   /* Set debug mode ON */
                   debugMode = 1;
                   break;
      case 'j' :   /* Configuration file name */
                   configFilename = strdup( optarg );             
    }
  }
  if ( configFilename == NULL ) {
     printf("You did not specify a configuration file!\n");
     exit(0);
  }
  if ( optind != argc ) {
     printf("Unrecognized command line options were ignored!\n");
  }
}

void daemonize( void ) {
pid_t pid, sid;
   

   pid = fork();
   if (pid < 0) {
           exit( 4 );
   }

   if (pid > 0) {
      exit( 0 );   /* Master exits    */
   }
   umask(0);       /* Child continues */  

   sid = setsid();
   if (sid < 0) {
      exit( 4 );
   }
   /* Close out the standard file descriptors */
   close(STDIN_FILENO);
   close(STDOUT_FILENO);
   close(STDERR_FILENO);
}

int main( int argc, char * * argv ) {
int              i,
                 saveErrno,
                 emSocket,
                 newSocket,
                 backlogConnections = 0, 
                 requestLength;
                 
struct sigaction sact;

uint8_t       *  mbRequest = NULL;
char          *  ipAddress = NULL;
char          *  ipPort    = NULL;



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
   
   ipAddress          = json_extract("$.ip");
   ipPort             = json_extract("$.port");
   tickFrequency      = atoi( json_extract("$.tickFrequency") );
   backlogConnections = atoi( json_extract("$.backlog") );
   
   json_finalize();
    
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Initialize the MODBUS library
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
   mbRequest = ( uint8_t * ) calloc( REQUEST_BUFFER_SIZE, sizeof( uint8_t ) );

   mbContext = modbus_new_tcp_pi( ipAddress, ipPort );
   if ( debugMode ) {
      i = modbus_set_debug( mbContext, TRUE );
   } else {
      i = modbus_set_debug( mbContext, FALSE );
   }
   if ( i != 0 ) {
      printf("MODBUS_SET_DEBUG did not work!\n");
      exit(0);
   }
   mbData    = modbus_mapping_new( MB_NB_BITS, MB_NB_INBITS,
                                   MB_NB_REGS, MB_NB_INREGS );
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * This is the spot where we daemonize the program ...
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

   daemonize( );
                         
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * The real work starts here ...
 *
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
 
   sigemptyset( &sact.sa_mask );
   sact.sa_flags   = 0;
   sact.sa_handler = tickTock;
   sigaction( SIGALRM, &sact, NULL );
   
   sigemptyset( &sact.sa_mask );
   sact.sa_flags   = 0;
   sact.sa_handler = loopStopper;
   sigaction( SIGUSR1, &sact, NULL );
   
   sigemptyset( &sact.sa_mask );
   sact.sa_flags   = 0;
   sact.sa_handler = toggleDebug;
   sigaction( SIGUSR2, &sact, NULL );
   
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * From this point on we do not want any console printf traffic, so we
 * will begin using syslog facilities ... 
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
 
   openlog( "ARGOS MODBUS EMULATOR", LOG_NDELAY | LOG_PID, ARGOS_LOG );
   syslog( LOG_INFO, "Started ..." );    

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Now wait for and process client requests ...
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
     
reStart :
   emSocket = modbus_tcp_pi_listen( mbContext, backlogConnections );
   
   FD_ZERO( &allSockets );
   FD_ZERO( &readySockets );
   
   FD_SET( emSocket, &allSockets );
   fdmax = emSocket;
   
   alarm( tickFrequency );
   
   while ( forever ) {
     readySockets = allSockets;
     if ( select( fdmax+1, &readySockets,NULL,NULL,NULL) == -1 ) {
        saveErrno = errno;
        if ( saveErrno == 4 ) {  /* Interrupted call */
          continue;
        } else {
          syslog( LOG_ERR, "SELECT system call has failed  [%d] <%s>!\n",
                           saveErrno,strerror( saveErrno ) );
          alarm(0);
          forever = 0;   /* Orderly program termination */
          continue;
        }       
     }
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Check all the sockets to see who is asking for service
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */     
     for( int s = 0; s <= fdmax; s++ ) {
        if ( ! FD_ISSET( s, &readySockets ) ) {  /* Not socket s  */
           continue;
        }  
        if ( s == emSocket ) {   /* This means a new client wants to connect   */
          newSocket = modbus_tcp_pi_accept( mbContext, &emSocket );
          if( newSocket == -1 ) {
            saveErrno = errno;
            if ( saveErrno == 4 ) continue;
            alarm( 0 );  /*  Why?  */        
            syslog( LOG_ERR, "ACCEPT error <%d> <%s>\n",saveErrno, strerror( saveErrno ) );
            close( emSocket );
            modbus_close( mbContext );
            sleep( 10 );
            goto reStart;
          } 
          syslog( LOG_NOTICE, "New client has connected on socket <%d>", newSocket );
          FD_SET( newSocket, &allSockets );
          if ( newSocket > fdmax ) { fdmax = newSocket; }   
        } else {                 /* Already connected client wants to get data */
           modbus_set_socket( mbContext, s );
           requestLength = modbus_receive( mbContext, mbRequest );
           if ( requestLength == -1 ) {
             saveErrno = errno;
             if ( saveErrno == 4 ) continue;
             if ( saveErrno != EINTR ) {
                syslog( LOG_ERR, "RECEIVE error <%d> <%s>\n",saveErrno, strerror( saveErrno ) );
             } else {
                syslog( LOG_NOTICE, "CLIENT socket <%d> closed!\n", s );
             }
             close( s );  
             FD_CLR( s, &allSockets );
             if ( s == fdmax ) fdmax--;
             continue;     
           }
           modbus_reply( mbContext, mbRequest, requestLength, mbData );
        }
     }
   }
   
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * When we are told to shutdown ( via SIGUSR1 ) we clean up and exit
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
   syslog( LOG_INFO, "Terminating ..." );
   closelog();
   
   modbus_mapping_free( mbData );
   modbus_close( mbContext );
   modbus_free( mbContext );
   return 0;
}
