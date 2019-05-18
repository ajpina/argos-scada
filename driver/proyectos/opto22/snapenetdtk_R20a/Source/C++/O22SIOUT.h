//-----------------------------------------------------------------------------
//
// O22SIOUT.h
// Copyright (c) 1999 - 2002 by Opto 22
//
// Utility header for the Ethernet I/O Driver Toolkit C++ classes.  
// 
// While this code was developed on Microsoft Windows 32-bit operating 
// systems, it is intended to be as generic as possible.  For Windows specific
// code, search for "_WIN32" and "_WIN32_WCE".  For Linux specific code, search
// for "_LINUX".
//-----------------------------------------------------------------------------

#ifndef __O22SIOUT_H_
#define __O22SIOUT_H_

#ifdef _WIN32
#ifdef _WIN32_WCE
#include "wtypes.h"
#include "winsock.h"
#else
#include "winsock2.h"
#include "process.h"    
#include "stdio.h"
#endif 
#endif 

#ifdef _LINUX
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

// The following socket items are taken from Windows
typedef int SOCKET;
#define INVALID_SOCKET 0
#define SOCKET_ERROR   -1

// The following boolean items are taken from Windows
typedef int    BOOL;
#define FALSE  0
#define TRUE   1

#endif // _LINUX


// The different basic data types that will be used
typedef short               SHORT;
typedef long                LONG;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef unsigned char       BYTE;
typedef float               FLOAT;


// The following macros are useful for making values from smaller smarts, such as making
// a four-byte integer from the four individual bytes.

#ifndef O22MAKELONG // makes a long from 4 bytes
#define O22MAKELONG(b0, b1, b2, b3)  ((((DWORD)(b0)) << 24) | (((DWORD)(b1)) << 16) | (((DWORD)(b2)) << 8) | ((DWORD)(b3)))
#endif 

#ifndef O22MAKELONG2 // makes a long from 4 bytes
#define O22MAKELONG2(a, o)  ((((DWORD)(a[o])) << 24) | (((DWORD)(a[o+1])) << 16) | (((DWORD)(a[o+2])) << 8) | ((DWORD)(a[o+3])))
#endif 

#ifndef O22MAKEFLOAT // makes a float from 4 bytes
#define O22MAKEFLOAT(pf, b0, b1, b2, b3)  ((BYTE*)pf)[0] = b3 ; ((BYTE*)pf)[1] = b2 ; ((BYTE*)pf)[2] = b1 ; ((BYTE*)pf)[3] = b0 ;
#endif

extern float O22MAKEFLOAT2(BYTE * a, long o);


#ifndef O22MAKEWORD // makes a word from 2 bytes
#define O22MAKEWORD(b0, b1)  (((WORD)((BYTE)(b0))) << 8) | (WORD)(((BYTE)(b1)))
#endif 

#ifndef O22MAKEWORD2 // makes a word from 2 bytes
#define O22MAKEWORD2(a, o)  (((WORD)((BYTE)(a[o]))) << 8) | (WORD)(((BYTE)(a[o+1])))
#endif 

#ifndef O22MAKEBYTE2 // makes a byte from 1 byte, for consistency with other macros
#define O22MAKEBYTE2(a, o)  a[o]
#endif 

#ifndef O22MAKEDWORD // makes a dwords from 2 words
#define O22MAKEDWORD(w0, w1) (((DWORD)((WORD)(w0))) << 16) | (DWORD)(((WORD)(w1)))
#endif

// The following macros are useful for breaking up a value into smaller pieces, such as 
// getting a third byte out of a 32-bit integer

#ifndef O22LOWORD // gets the low word from a dword
#define O22LOWORD(l) ((WORD)(l))
#endif

#ifndef O22HIWORD // gets the high word from a dword
#define O22HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#endif

#ifndef O22LOBYTE // gets the low byte from a word
#define O22LOBYTE(w) ((BYTE)(w))
#endif

#ifndef O22HIBYTE // gets the high byte from a word
#define O22HIBYTE(w) ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#endif

#ifndef O22BYTE0 // gets the first byte from a dword
#define O22BYTE0(l) ((BYTE)(((DWORD)(l) >> 24) & 0xFF))
#endif

#ifndef O22BYTE1 // gets the second byte from a dword
#define O22BYTE1(l) ((BYTE)(((DWORD)(l) >> 16) & 0xFF))
#endif

#ifndef O22BYTE2 // gets the third byte from a dword
#define O22BYTE2(l) ((BYTE)(((DWORD)(l) >> 8) & 0xFF))
#endif

#ifndef O22BYTE3 // gets the fourth byte from a dword
#define O22BYTE3(l) ((BYTE)(l))
#endif

#ifndef O22FILL_ARRAY_FROM_SHORT // Fill the given byte array with a short
#define O22FILL_ARRAY_FROM_SHORT(a,i,n) a[i] = O22HIBYTE(n); a[i+1] = O22LOBYTE(n);
#endif

#ifndef O22FILL_ARRAY_FROM_LONG  // Fill the given byte array with a long
#define O22FILL_ARRAY_FROM_LONG(a,i,n)  a[i] = O22BYTE0(n); a[i+1] = O22BYTE1(n); a[i+2] = O22BYTE2(n); a[i+3] = O22BYTE3(n)
#endif

#ifndef O22FILL_ARRAY_FROM_FLOAT // Fill the given byte array with a float
#define O22FILL_ARRAY_FROM_FLOAT(a,i,f)  O22FILL_ARRAY_FROM_LONG(a, i, *((long*)(&f)))
#endif

#ifndef O22FILL_ARRAY_FROM_FLOATX // ??
#define O22FILL_ARRAY_FROM_FLOATX(a,i,f)  {DWORD dwt; memcpy(&dwt, &f, 4); O22FILL_ARRAY_FROM_LONG(a, i, dwt);}
#endif

#ifndef O22_SWAP_BYTES_LONG // swap the bytes in a long
#define O22_SWAP_BYTES_LONG(l)  O22MAKELONG(O22BYTE0(l), O22BYTE1(l), O22BYTE2(l), O22BYTE3(l))
#endif



// Error values generated by the classed in the driver toolkit
#define SIOMM_OK                             1
#define SIOMM_ERROR                         -1
#define SIOMM_TIME_OUT                      -2 
#define SIOMM_ERROR_NO_SOCKETS              -3  // Unable to access socket interface
#define SIOMM_ERROR_CREATING_SOCKET         -4
#define SIOMM_ERROR_CONNECTING_SOCKET       -5
#define SIOMM_ERROR_RESPONSE_BAD            -6
#define SIOMM_ERROR_NOT_CONNECTED_YET       -7
#define SIOMM_ERROR_OUT_OF_MEMORY           -8
#define SIOMM_ERROR_NOT_CONNECTED           -9
#define SIOMM_ERROR_STREAM_TYPE_BAD         -10

// Error values generated by the I/O brain itself
#define SIOMM_BRAIN_ERROR_UNDEFINED_CMD      0xE001
#define SIOMM_BRAIN_ERROR_INVALID_PT_TYPE    0xE002
#define SIOMM_BRAIN_ERROR_INVALID_FLOAT      0xE003
#define SIOMM_BRAIN_ERROR_PUC_EXPECTED       0xE004
#define SIOMM_BRAIN_ERROR_INVALID_ADDRESS    0xE005
#define SIOMM_BRAIN_ERROR_INVALID_CMD_LENGTH 0xE006
#define SIOMM_BRAIN_ERROR_RESERVED           0xE007
#define SIOMM_BRAIN_ERROR_BUSY               0xE008
#define SIOMM_BRAIN_ERROR_CANT_ERASE_FLASH   0xE009
#define SIOMM_BRAIN_ERROR_CANT_PROG_FLASH    0xE00A
#define SIOMM_BRAIN_ERROR_IMAGE_TOO_SMALL    0xE00B
#define SIOMM_BRAIN_ERROR_IMAGE_CRC_MISMATCH 0xE00C
#define SIOMM_BRAIN_ERROR_IMAGE_LEN_MISMATCH 0xE00D
#define SIOMM_BRAIN_ERROR_FEATURE_NOT_IMPL   0xE00E
#define SIOMM_BRAIN_ERROR_WATCHDOG_TIMEOUT   0xE00F

// The types of socket connections
#define SIOMM_TCP   SOCK_STREAM
#define SIOMM_UDP   SOCK_DGRAM

#endif // __O22SIOUT_H_
