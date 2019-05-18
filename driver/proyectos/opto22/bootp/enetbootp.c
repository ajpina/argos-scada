/*
  module:     enetbootp.c

  desc:       bootp project module for IP configuring Opto 22 ethernet devices

  author:     Bryce A. Nakatani (2000-01-11)
  revised:

  notes:      the bootp packet data sections are stolen from OptoBootp, oh well...

              because the ports for bootp are defined within the system's port reservation, the sockets
              will fail unless this application is run at a root level

  DISCLAIMER:  This source code is supplied as shareware, use at your own risk without any gaurantees or support.
*/

// system includes
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

// local includes
#include "enetbootp.h"


#define HIBYTE(w)   ((BYTE) (((WORD) (w) >> 8) & 0xFF))
#define LOBYTE(w)   ((BYTE) (w))
#define HIWORD(l)   ((WORD) (((DWORD) (l) >> 16) & 0xFFFF))
#define LOWORD(l)   ((WORD) (l)) 

#define ENETBOOTP_MAX_DEVICES 1024

// globals for stuff
int     incoming;                  // socket handle for the incoming packets
int     outgoing;                  // socket handle for the outgoing broacast packets


// the structure to keep track of this stuff
typedef struct
{
  // stuff from the network
  char   buffer[BOOTP_BUF_SIZE];  // incoming buffer from the device
  int    received;                // number of times this device has bootp'd
  int    mac[6];                  // mac number from the device (for convenience)

  // netmask the user want's this device to be
  // sorry, I did the ipv4 brute force method, wanted to keep code endian independent as possible!
  int    ip[4];                   // only worry about ipv4
  int    netmask[4];              // netmask for this device
  int    gateway[4];              // gateway for this device
  int    dns[4];                  // dns server for this device
  int    sent;                    // flag for the configured state of the device
  long   tid;                     // transaction number from the device
} ENETBOOTP_ST;

// fixed list for the devices
ENETBOOTP_ST enetlist[ENETBOOTP_MAX_DEVICES];     // root pointer to the list
long         enet_new_counter;                    // enet new counter

// local prototypes
void enetbootp_read_cfg();
void enetbootp_configure();
void enetbootp_listen();
int  enetbootp_search(int mac3,int mac4,int mac5);
void enetbootp_send(int index);
void enetbootp_close_sockets();
void enetbootp_inventory();


/*
  function:  main()
  desc:      main function for the enet brain
  args:      none
  returns:   nothing
  notes:
*/
int  main()
{
  // read the configuration file
  enetbootp_read_cfg();

  // do brain configuration
  enetbootp_configure();

  // do the listen loop
  enetbootp_listen();

  // close the open sockets
  enetbootp_close_sockets();
}


/*
  function: enetbootp_configure()
  desc:     performs the listen/configuration of the brain
  args:     none
  returns:  nothing
  notes:    uses the data in the enet list for the configuration send
*/
void enetbootp_configure()
{
  struct sockaddr_in address;     // local copy to configure the server address
  int                flag = 1;    // constant for the setsockopt        

  // create and configure the incoming server socket as UDP
  incoming = socket(AF_INET,SOCK_DGRAM,0);

  // set socket to be non-blocking
  if(fcntl(incoming,F_SETFL,O_NONBLOCK) == -1)
    {
      // inform the user
      printf("sorry, couldn't make the incoming socket unblocked\n");

      // terminate the application with error
      exit(-1);
    }


  // initialize the binding structure
  address.sin_family       = AF_INET;
  address.sin_addr.s_addr  = INADDR_ANY;
  address.sin_port         = htons(BOOTP_PORT_SERVER);

  // bind the server port to the incoming socket
  if(bind(incoming,(struct sockaddr_in *)&address,sizeof(address)) < 0)
    {
      // inform the user of the error
      printf("error with the binding the server port, are you running as root?\n");
      
      // terminate the application
      exit(-1);
    }

  // create the outgoing socket
  outgoing = socket(AF_INET,SOCK_DGRAM,0);

  // set the BROADCAST option so that we can broadcast enet bootp responses
  if(setsockopt(outgoing,SOL_SOCKET,SO_BROADCAST,&flag,sizeof flag) < 0) {
    // inform the user of this fault
    printf("can't set SO_BROADCAST option on enet bootp socket\n");

    // terminate the app
    exit(-1);
  }
}


/*
  function: enetbootp_close_sockets()
  desc:     closes the sockets (OS does this anyways)
  args:     none
  returns:  nothing
  notes:
*/
void enetbootp_close_sockets()
{
  close(incoming);
  close(outgoing);
}


/*
  function:  enetbootp_listen()
  desc:      function that listens for incoming requests
  args:      none
  returns:   nothing
  notes:
*/
void enetbootp_listen()
{
  char          Buf[BOOTP_BUF_SIZE];      // the working buffer
  int           BufLen = BOOTP_BUF_SIZE;  // length of the buffer
  long          tid;                      // transaction id from the device (move to struct sometime)
  int           index;                    // index of the enet list
  int           bytes;                    // number of bytes from receive

  // event loop to receive requests
  for(;;) {
    // attempt to receive data
    bytes = recv(incoming,Buf,BOOTP_BUF_SIZE,0);

    // see if the packet identifies as the buffer
    if(bytes > 0) {
      // if this is a request, of type ethernet
      if((0x01 == Buf[0x00]) & (0x01 == Buf[0x01])) {
          // make sure the request is from an Opto MAC address
        if(((char)OPTO_MAC_BYTE0 == Buf[BOOTP_OFFSET_HWD_ADDR]     ) &&
          ((char)OPTO_MAC_BYTE1 == Buf[BOOTP_OFFSET_HWD_ADDR + 1] ) &&
          ((char)OPTO_MAC_BYTE2 == Buf[BOOTP_OFFSET_HWD_ADDR + 2] )) {
          // search for the device in the list
          index = enetbootp_search(((int)Buf[BOOTP_OFFSET_HWD_ADDR + 3]) & 0xff,
                 ((int)Buf[BOOTP_OFFSET_HWD_ADDR + 4]) & 0xff,
                 ((int)Buf[BOOTP_OFFSET_HWD_ADDR + 5]) & 0xff);

          // inspect the search
          if(index < 0) {
            // print a short message
            printf("received response from unknown device mac: 00-a0-3d-%02x-%02x-%02x\n",
              Buf[BOOTP_OFFSET_HWD_ADDR + 3],
              Buf[BOOTP_OFFSET_HWD_ADDR + 4],
              Buf[BOOTP_OFFSET_HWD_ADDR + 5]);
          }
          else {
            // print a short message
            printf("received response from mac: 00-a0-3d-%02x-%02x-%02x\n",
              Buf[BOOTP_OFFSET_HWD_ADDR + 3] & 0xff,
              Buf[BOOTP_OFFSET_HWD_ADDR + 4] & 0xff,
              Buf[BOOTP_OFFSET_HWD_ADDR + 5] & 0xff);

            // received a message
            enetlist[index].received++;

            // save the packet tid to local
            memcpy(&enetlist[index].tid,Buf + BOOTP_OFFSET_TID,sizeof(enetlist[index].tid));

            // copy the buffer to the structure
            memcpy(enetlist[index].buffer,Buf,bytes);

            // send the response to this device
            enetbootp_send(index);
          }
        }
      }
    }
  }
} 


/*
  function: enetbootp_send()
  desc:     sends enet bootp packet to the brain
  args:     index   the struct number to send the response to
  returns:  nothing
  notes:
*/
void enetbootp_send(int index)
{
  char packet[BOOTP_BUF_SIZE];    // packet of data to be sent
  struct sockaddr_in address;     // local copy to configure the broadcast address
  long          VendorIndex;      // index for the vendor data area of the packet

  // Fill in only the fields we require, all other are 0x00
  packet[BOOTP_OFFSET_OP] = BOOTP_VALUE_REPLY;
  // HTYPE field
  packet[BOOTP_OFFSET_HTYPE] = BOOTP_VALUE_HTYPE_ENET;
  // HLEN field
  packet[BOOTP_OFFSET_HLEN] = BOOTP_VALUE_HLEN;
  // HOPS field
  packet[BOOTP_OFFSET_HOPS] = BOOTP_VALUE_HOPS;
  // Transaction ID field
  memcpy(&packet[BOOTP_OFFSET_TID], &enetlist[index].tid,sizeof(enetlist[index].tid));

  // the ip address
  packet[BOOTP_OFFSET_YOUR_IP]       = (char)enetlist[index].ip[0];
  packet[(BOOTP_OFFSET_YOUR_IP + 1)] = (char)enetlist[index].ip[1];
  packet[(BOOTP_OFFSET_YOUR_IP + 2)] = (char)enetlist[index].ip[2];
  packet[(BOOTP_OFFSET_YOUR_IP + 3)] = (char)enetlist[index].ip[3];

  // gateway address
  packet[BOOTP_OFFSET_GATEWAY]       = (char)enetlist[index].gateway[0];
  packet[(BOOTP_OFFSET_GATEWAY + 1)] = (char)enetlist[index].gateway[1];
  packet[(BOOTP_OFFSET_GATEWAY + 2)] = (char)enetlist[index].gateway[2];
  packet[(BOOTP_OFFSET_GATEWAY + 3)] = (char)enetlist[index].gateway[3];

  // client hardware address field
  packet[BOOTP_OFFSET_HWD_ADDR + 0] = OPTO_MAC_BYTE0;
  packet[BOOTP_OFFSET_HWD_ADDR + 1] = OPTO_MAC_BYTE1;
  packet[BOOTP_OFFSET_HWD_ADDR + 2] = OPTO_MAC_BYTE2;
  packet[BOOTP_OFFSET_HWD_ADDR + 3] = (char)enetlist[index].mac[3];
  packet[BOOTP_OFFSET_HWD_ADDR + 4] = (char)enetlist[index].mac[4];
  packet[BOOTP_OFFSET_HWD_ADDR + 5] = (char)enetlist[index].mac[5];

  // Fill in the vendor specific area
  VendorIndex = BOOTP_OFFSET_VENDOR_DATA;

  // first the "Magic Cookie"
  packet[ VendorIndex++ ] = HIBYTE(HIWORD(MAGIC_COOKIE));
  packet[ VendorIndex++ ] = LOBYTE(HIWORD(MAGIC_COOKIE));
  packet[ VendorIndex++ ] = HIBYTE(LOWORD(MAGIC_COOKIE));
  packet[ VendorIndex++ ] = LOBYTE(LOWORD(MAGIC_COOKIE));

  // now the sub-net
  packet[ VendorIndex++ ] = VENDOR_TAG_SUBNET;
  packet[ VendorIndex++ ] = 4; // length of following data for this tag.
  packet[ VendorIndex++ ] = (char)enetlist[index].netmask[0];
  packet[ VendorIndex++ ] = (char)enetlist[index].netmask[1];
  packet[ VendorIndex++ ] = (char)enetlist[index].netmask[2];
  packet[ VendorIndex++ ] = (char)enetlist[index].netmask[3];

  //now the domain name server
  packet[ VendorIndex++ ] = VENDOR_TAG_DNS;
  packet[ VendorIndex++ ] = 4; // length of following data for this tag.
  packet[ VendorIndex++ ] = (char)enetlist[index].dns[0];
  packet[ VendorIndex++ ] = (char)enetlist[index].dns[1];
  packet[ VendorIndex++ ] = (char)enetlist[index].dns[2];
  packet[ VendorIndex++ ] = (char)enetlist[index].dns[3];

  // Now the END tag
  packet[ VendorIndex++ ] = VENDOR_TAG_END; // terminate with 'FF'.

  // initialize the binding structure for the send
  address.sin_family      = AF_INET;
  address.sin_port        = htons(BOOTP_PORT_CLIENT);
  address.sin_addr.s_addr = htonl (INADDR_BROADCAST);
  memset (address.sin_zero, 0, sizeof (address.sin_zero));

  // broadcast the packet
  if(sendto(outgoing,packet,BOOTP_PACKET_SIZE,0,&address,sizeof(address)) == -1) {
    // inform the user of the broadcast fault
    printf("send failed %s; %d\n",strerror(errno),errno);

    // terminate the application
    exit(-1);
  }
  else {
    // sent packet to device
    printf("configuration packet successfully sent to device\n");

    // increment the sent number
    enetlist[index].sent++;
  }
}


/*
  function: enetbootp_read_cfg()
  desc:     reads the configuration file and builds the list of hardware
  args:     none
  returns:  nothing
  notes:    assumes the cfg file is local to the application
*/
void enetbootp_read_cfg()
{
  FILE *input;              // std c pointer to the input file
  char buffer[256];         // buffer for file io

  // open the file
  input = fopen("enetbootp.cfg","r");

  // test for valid open
  if(input == NULL) {
    // inform the user of the error
    printf("enetbootp could not find the configuration file <*.cfg>\n");

    // terminate the app with error
    exit(-1);
  }

  // read loop
  for(;!feof(input);) {
    // get a string from the input
    fgets(buffer,255,input);

    // check if comment or blank line
    if(*buffer != '#' && *buffer != '\0') {
      // inspect if we have enough structs to go around
      if(enet_new_counter == ENETBOOTP_MAX_DEVICES) {
        // inform the user
        printf("sorry, out of enet device structures, resize ENETBOOTP_MAX_DEVICES\n");

        // terminate the app
        exit(-1);
      }

      // extract the data out of the string
      sscanf(buffer,"%x-%x-%x-%x-%x-%x %d.%d.%d.%d %d.%d.%d.%d %d.%d.%d.%d %d.%d.%d.%d",
        &enetlist[enet_new_counter].mac[0],&enetlist[enet_new_counter].mac[1],&enetlist[enet_new_counter].mac[2],
        &enetlist[enet_new_counter].mac[3],&enetlist[enet_new_counter].mac[4],&enetlist[enet_new_counter].mac[5],
        &enetlist[enet_new_counter].ip[0],&enetlist[enet_new_counter].ip[1],
        &enetlist[enet_new_counter].ip[2],&enetlist[enet_new_counter].ip[3],
        &enetlist[enet_new_counter].netmask[0],&enetlist[enet_new_counter].netmask[1],
        &enetlist[enet_new_counter].netmask[2],&enetlist[enet_new_counter].netmask[3],
        &enetlist[enet_new_counter].gateway[0],&enetlist[enet_new_counter].gateway[1],
        &enetlist[enet_new_counter].gateway[2],&enetlist[enet_new_counter].gateway[3],
        &enetlist[enet_new_counter].dns[0],&enetlist[enet_new_counter].dns[1],
        &enetlist[enet_new_counter].dns[2],&enetlist[enet_new_counter].dns[3]);

      // increment the struct counter
      enet_new_counter++;
    }
  }
  
  // close the input file
  fclose(input);

  // tell the user how many entries read
  printf("%d entries read in cfg file\n",enet_new_counter);
}


/*
  function:  enetbootp_search()
  desc:      searches for a device in the list that matches the lsb 3 bytes of the MAC
  args:      none
  returns:   index in the list if found or -1 for not found
  notes:
*/
int enetbootp_search(int mac3,int mac4,int mac5)
{
  int i;               // loop counter

  // loop for length of the list
  for(i = 0; i < enet_new_counter; i++) {
    // compare the bytes
    if((mac3 == (int)enetlist[i].mac[3]) && (mac4 == (int)enetlist[i].mac[4]) && (mac5 == (int)enetlist[i].mac[5])) {
      // mac found, return
      return i;
    }
  }
  // not found, return error
  return -1;
}


/*
  function:  enetbootp_inventory()
  desc:      gives a status of the bootp inventory
  args:      none
  returns:   nothing
  notes:
*/
void enetbootp_inventory()
{
  int  i;                     // loop counter

  // loop through all entries
  for(i = 0; i < enet_new_counter; i++) {
    printf("information for mac: 00-a0-3d-%02x-%02x-%02x\n",enetlist[i].mac[3],enetlist[i].mac[4],enetlist[i].mac[5]);
    printf("   status:    received [%s] sent [%s]\n",enetlist[i].received ? "yes":"no",enetlist[i].sent ? "yes" : "no");
    printf("   ip:        %d.%d.%d.%d\n",enetlist[i].ip[0],enetlist[i].ip[1],enetlist[i].ip[2],enetlist[i].ip[3]);
    printf("   netmask:   %d.%d.%d.%d\n",enetlist[i].netmask[0],enetlist[i].netmask[1],
                                         enetlist[i].netmask[2],enetlist[i].netmask[3]);
    printf("   gateway:   %d.%d.%d.%d\n",enetlist[i].gateway[0],enetlist[i].gateway[1],
                                         enetlist[i].gateway[2],enetlist[i].gateway[3]);
    printf("   dns:       %d.%d.%d.%d\n",enetlist[i].dns[0],enetlist[i].dns[1],
                                         enetlist[i].dns[2],enetlist[i].dns[3]);
  }
}
