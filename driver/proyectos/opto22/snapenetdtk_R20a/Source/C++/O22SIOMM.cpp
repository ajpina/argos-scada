//-----------------------------------------------------------------------------
//
// O22SIOMM.cpp
// Copyright (c) 1999 - 2002 by Opto 22
//
// Source for the O22SnapIoMemMap C++ class.  
// 
// The O22SnapIoMemMap C++ class is used to communicate from a computer to an
// Opto 22 SNAP Ethernet I/O unit.
//
// See the header file for more information on using this class.
//
// While this class was developed on Microsoft Windows 32-bit operating 
// systems, it is intended to be as generic as possible.  For Windows specific
// code, search for "_WIN32" and "_WIN32_WCE".  For Linux specific code, search
// for "_LINUX".
//-----------------------------------------------------------------------------

#ifndef __O22SIOMM_H_
#include "O22SIOMM.h"
#endif

#ifdef _WIN32
#define WINSOCK_VERSION_REQUIRED_MAJ 2
#define WINSOCK_VERSION_REQUIRED_MIN 0
#endif

O22SnapIoMemMap::O22SnapIoMemMap()
//-------------------------------------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------------------------------------
{
  // Set defaults
  m_Socket = INVALID_SOCKET;
  m_byTransactionLabel = 0;
  m_nRetries = 0;
  m_nOpenTime = 0;
  m_nOpenTimeOutMS = 0;
  m_nTimeOutMS = 1000;
  m_tvTimeOut.tv_sec  = m_nTimeOutMS / 1000;
  m_tvTimeOut.tv_usec = (m_nTimeOutMS % 1000) * 1000;
}
 

O22SnapIoMemMap::~O22SnapIoMemMap()
//-------------------------------------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------------------------------------
{
  CloseSockets();

#ifdef _WIN32
  WSACleanup();
#endif

}


LONG O22SnapIoMemMap::OpenEnet(char * pchIpAddressArg, long nPort, long nOpenTimeOutMS, long nAutoPUC)
//-------------------------------------------------------------------------------------------------
// Open a connection to a SNAP Ethernet I/O unit
//-------------------------------------------------------------------------------------------------
{
  m_nAutoPUCFlag    = nAutoPUC;
  m_nConnectionType = SIOMM_TCP;

  return OpenSockets(pchIpAddressArg, nPort, nOpenTimeOutMS);
}


LONG O22SnapIoMemMap::OpenEnet2(char * pchIpAddressArg, long nPort, long nOpenTimeOutMS, long nAutoPUC, long nConnectionType)
//-------------------------------------------------------------------------------------------------
// Open a connection to a SNAP Ethernet I/O unit
//-------------------------------------------------------------------------------------------------
{
  m_nAutoPUCFlag    = nAutoPUC;
  m_nConnectionType = nConnectionType;

  return OpenSockets(pchIpAddressArg, nPort, nOpenTimeOutMS);
}


LONG O22SnapIoMemMap::OpenSockets(char * pchIpAddressArg, long nPort, long nOpenTimeOutMS)
//-------------------------------------------------------------------------------------------------
// Use sockets to open a connection to the SNAP I/O unit
//-------------------------------------------------------------------------------------------------
{
  int       nResult; // for checking the return values of functions

#ifdef _WIN32
  // Initialize WinSock.dll
  WSADATA   wsaData; // for checking WinSock

  nResult = WSAStartup(  O22MAKEWORD( WINSOCK_VERSION_REQUIRED_MIN, 
                                      WINSOCK_VERSION_REQUIRED_MAJ ), &wsaData );
  if ( nResult != 0 ) 
  {
    // We couldn't find a socket interface. 
    return SIOMM_ERROR_NO_SOCKETS;  
  } 

  // Confirm that the WinSock DLL supports WINSOCK_VERSION 
  if (( O22LOBYTE( wsaData.wVersion ) != WINSOCK_VERSION_REQUIRED_MAJ) || 
      ( O22HIBYTE( wsaData.wVersion ) != WINSOCK_VERSION_REQUIRED_MIN)) 
  {
    // We couldn't find an acceptable socket interface. 
    WSACleanup( );
    return SIOMM_ERROR_NO_SOCKETS; 
  } 
#endif


  // If a socket is open, close it now.
  CloseSockets();

  m_nOpenTimeOutMS = nOpenTimeOutMS;

  // Create the socket via TCP or UDP
  m_Socket = socket(AF_INET, m_nConnectionType, 0);
  if (m_Socket == INVALID_SOCKET)
  {
    // Couldn't create the socket
#ifdef _WIN32
    WSACleanup( );
#endif

    return SIOMM_ERROR_CREATING_SOCKET;
  }

  // Make the socket non-blocking
#ifdef _WIN32
  // Windows uses ioctlsocket() to set the socket as non-blocking.
  // Other systems may use ioctl() or fcntl()
  unsigned long nNonBlocking = 1;
  if (SOCKET_ERROR == ioctlsocket(m_Socket, FIONBIO, &nNonBlocking))
#endif
#ifdef _LINUX
  if (-1 == fcntl(m_Socket,F_SETFL,O_NONBLOCK))
#endif
  {
    CloseSockets();

    return SIOMM_ERROR_CREATING_SOCKET;
  }

  // Setup the socket address structure
  m_SocketAddress.sin_addr.s_addr = inet_addr(pchIpAddressArg);
  m_SocketAddress.sin_family      = AF_INET;
  m_SocketAddress.sin_port        = htons((WORD)nPort);

  // attempt connection
  connect(m_Socket, (sockaddr*) &m_SocketAddress, sizeof(m_SocketAddress));

  // Get the time for the timeout logic in IsOpenDone()
#ifdef _WIN32
  m_nOpenTime = GetTickCount(); // GetTickCount returns the number of milliseconds that have
                                // elapsed since the system was started.
#endif
#ifdef _LINUX
  tms DummyTime;
  m_nOpenTime = times(&DummyTime); // times() returns the number of milliseconds that have
                                   // elapsed since the system was started.
#endif

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::CloseSockets()
//-------------------------------------------------------------------------------------------------
// Close the sockets connection
//-------------------------------------------------------------------------------------------------
{
  // Close up everything
  if (m_Socket != INVALID_SOCKET)
  { 
#ifdef _WIN32
    closesocket(m_Socket);
#endif
#ifdef _LINUX
    close(m_Socket);
#endif
  }
  
  // Reset our data members
  memset(&m_SocketAddress, 0, sizeof(m_SocketAddress));
  m_Socket = INVALID_SOCKET;
  m_nOpenTimeOutMS = 0;
  m_nOpenTime = 0;
  m_nRetries = 0;


  return SIOMM_OK;
}

LONG O22SnapIoMemMap::IsOpenDone()
//-------------------------------------------------------------------------------------------------
// Called after an OpenEnet() function to determine if the open process is completed yet.
//-------------------------------------------------------------------------------------------------
{
  fd_set  fds;
  timeval tvTimeOut;
  DWORD   nOpenTimeOutTemp;
  DWORD   dwPUCFlag;   // a flag for checking the status of PowerUp Clear on the I/O unit
  long    nResult;     // for checking the return values of functions

  // Check the open timeout
#ifdef _WIN32
  nOpenTimeOutTemp = GetTickCount(); // GetTickCount returns the number of milliseconds that have
                                     // elapsed since the system was started. 
#endif
#ifdef _LINUX
  tms DummyTime;
  nOpenTimeOutTemp = times(&DummyTime); // times() returns the number of milliseconds that have
                                        // elapsed since the system was started. 
#endif
  
  // Check for overflow of the system timer
  if (m_nOpenTime > nOpenTimeOutTemp) 
    m_nOpenTime = 0;

  // Check for timeout
  if (m_nOpenTimeOutMS < (nOpenTimeOutTemp - m_nOpenTime)) 
  {
    // Timeout has occured.
    CloseSockets();
    return SIOMM_TIME_OUT;
  }

  FD_ZERO(&fds);
  FD_SET(m_Socket, &fds);

  // We want the select to return immediately, so set the timeout to zero
  tvTimeOut.tv_sec  = 0;
  tvTimeOut.tv_usec = 100;

  // Use select() to check if the socket is connect and ready
  if (0 == select(m_Socket + 1, NULL, &fds, NULL, &tvTimeOut))
  {
    // we're not connected yet!
    return SIOMM_ERROR_NOT_CONNECTED_YET;
  }

  // Okay, we must be connected if we get past the select() above.
  
  if (m_nAutoPUCFlag)
  {
    // Now, check the PowerUp Clear flag of the brain.  It must be cleared 
    // before can do anything with it (other than read the status area.)

    // Read PowerUp Clear flag
    nResult = ReadQuad(SIOMM_STATUS_READ_PUC_FLAG, &dwPUCFlag);
  
    // Check for good result from the ReadQuad()
    if (SIOMM_OK == nResult)
    {
      // the PUC flag will be TRUE if PUC is needed
      if (dwPUCFlag) 
      {
        // Send PUC
        nResult = WriteQuad(SIOMM_STATUS_WRITE_OPERATION, 1);

        return nResult;
      }
      else
      {
        // Everything must be okay
        return SIOMM_OK;
      }
    }
    else // the ReadQuad() had an error
    {
      return SIOMM_ERROR;
    }
  }
  else
  {
    // Everything must be okay
    return SIOMM_OK;
  }
}


LONG O22SnapIoMemMap::SetCommOptions(LONG nTimeOutMS, LONG nReserved)
//-------------------------------------------------------------------------------------------------
// Set communication options
//-------------------------------------------------------------------------------------------------
{
  // m_nRetries   = nReserved;
  m_nTimeOutMS = nTimeOutMS;
  
  // Set the timeout that sockets will use
  m_tvTimeOut.tv_sec  = m_nTimeOutMS / 1000;
  m_tvTimeOut.tv_usec = (m_nTimeOutMS % 1000) * 1000;

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::BuildReadBlockRequest(BYTE * pbyReadBlockRequest,
                                            BYTE   byTransactionLabel, 
                                            DWORD  dwDestinationOffset,
                                            WORD   wDataLength)
//-------------------------------------------------------------------------------------------------
// Build a read block request packet
//-------------------------------------------------------------------------------------------------
{
  // Destination Id
  pbyReadBlockRequest[0]  = 0x00;
  pbyReadBlockRequest[1]  = 0x00;

  // Transaction Label
  pbyReadBlockRequest[2]  = byTransactionLabel << 2;

  // Transaction Code
  pbyReadBlockRequest[3]  = SIOMM_TCODE_READ_BLOCK_REQUEST << 4;

  // Source Id
  pbyReadBlockRequest[4]  = 0x00;
  pbyReadBlockRequest[5]  = 0x00;

  // Destination Offset
  pbyReadBlockRequest[6]  = 0xFF;
  pbyReadBlockRequest[7]  = 0xFF;
  pbyReadBlockRequest[8]  = O22BYTE0(dwDestinationOffset);
  pbyReadBlockRequest[9]  = O22BYTE1(dwDestinationOffset);
  pbyReadBlockRequest[10] = O22BYTE2(dwDestinationOffset);
  pbyReadBlockRequest[11] = O22BYTE3(dwDestinationOffset);

  // Data length
  pbyReadBlockRequest[12] = O22HIBYTE(wDataLength);
  pbyReadBlockRequest[13] = O22LOBYTE(wDataLength);

  // Extended Transaction Code
  pbyReadBlockRequest[14] = 0x00;
  pbyReadBlockRequest[15] = 0x00;

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::BuildReadQuadletRequest(BYTE * pbyReadQuadletRequest,
                                              BYTE byTransactionLabel, 
                                              DWORD dwDestinationOffset)
//-------------------------------------------------------------------------------------------------
// Build a read quadlet request packet
//-------------------------------------------------------------------------------------------------
{
  pbyReadQuadletRequest[0]  = 0x00;
  pbyReadQuadletRequest[1]  = 0x00;
  pbyReadQuadletRequest[2]  = byTransactionLabel << 2;
  pbyReadQuadletRequest[3]  = SIOMM_TCODE_READ_QUAD_REQUEST << 4;
  pbyReadQuadletRequest[4]  = 0x00;
  pbyReadQuadletRequest[5]  = 0x00;
  pbyReadQuadletRequest[6]  = 0xFF;
  pbyReadQuadletRequest[7]  = 0xFF;
  pbyReadQuadletRequest[8]  = O22BYTE0(dwDestinationOffset);
  pbyReadQuadletRequest[9]  = O22BYTE1(dwDestinationOffset);
  pbyReadQuadletRequest[10] = O22BYTE2(dwDestinationOffset);
  pbyReadQuadletRequest[11] = O22BYTE3(dwDestinationOffset);

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::BuildWriteBlockRequest(BYTE *  pbyWriteBlockRequest,
                                             BYTE    byTransactionLabel,
                                             DWORD   dwDestinationOffset,
                                             WORD    wDataLength,
                                             BYTE  * pbyBlockData)
//-------------------------------------------------------------------------------------------------
// Build a write block request packet
//-------------------------------------------------------------------------------------------------
{
  // Destination Id
  pbyWriteBlockRequest[0]  = 0x00;
  pbyWriteBlockRequest[1]  = 0x00;

  // Transaction Label
  pbyWriteBlockRequest[2]  = byTransactionLabel << 2;

  // Transaction Code
  pbyWriteBlockRequest[3]  = SIOMM_TCODE_WRITE_BLOCK_REQUEST << 4;

  // Source Id
  pbyWriteBlockRequest[4]  = 0x00;
  pbyWriteBlockRequest[5]  = 0x00;

  // Destination Offset
  pbyWriteBlockRequest[6]  = 0xFF;
  pbyWriteBlockRequest[7]  = 0xFF;
  pbyWriteBlockRequest[8]  = O22BYTE0(dwDestinationOffset);
  pbyWriteBlockRequest[9]  = O22BYTE1(dwDestinationOffset);
  pbyWriteBlockRequest[10] = O22BYTE2(dwDestinationOffset);
  pbyWriteBlockRequest[11] = O22BYTE3(dwDestinationOffset);

  // Data length
  pbyWriteBlockRequest[12] = O22HIBYTE(wDataLength);
  pbyWriteBlockRequest[13] = O22LOBYTE(wDataLength);
  
  // Extended Transaction Code
  pbyWriteBlockRequest[14] = 0x00;
  pbyWriteBlockRequest[15] = 0x00;

  // Block data to write
  memcpy(&(pbyWriteBlockRequest[16]), pbyBlockData, wDataLength);


  return SIOMM_OK;
}



LONG O22SnapIoMemMap::BuildWriteQuadletRequest(BYTE * pbyWriteQuadletRequest,
                                               BYTE byTransactionLabel, 
                                               WORD wSourceId,
                                               DWORD dwDestinationOffset,
                                               DWORD dwQuadletData)
//-------------------------------------------------------------------------------------------------
// Build a write quadlet request packet
//-------------------------------------------------------------------------------------------------
{
  pbyWriteQuadletRequest[0]  = 0x00;
  pbyWriteQuadletRequest[1]  = 0x00;
  pbyWriteQuadletRequest[2]  = byTransactionLabel << 2;
  pbyWriteQuadletRequest[3]  = SIOMM_TCODE_WRITE_QUAD_REQUEST << 4;
  pbyWriteQuadletRequest[4]  = O22HIBYTE(wSourceId);
  pbyWriteQuadletRequest[5]  = O22LOBYTE(wSourceId);
  pbyWriteQuadletRequest[6]  = 0xFF;
  pbyWriteQuadletRequest[7]  = 0xFF;
  pbyWriteQuadletRequest[8]  = O22BYTE0(dwDestinationOffset);
  pbyWriteQuadletRequest[9]  = O22BYTE1(dwDestinationOffset);
  pbyWriteQuadletRequest[10] = O22BYTE2(dwDestinationOffset);
  pbyWriteQuadletRequest[11] = O22BYTE3(dwDestinationOffset);
  pbyWriteQuadletRequest[12] = O22BYTE0(dwQuadletData);
  pbyWriteQuadletRequest[13] = O22BYTE1(dwQuadletData);
  pbyWriteQuadletRequest[14] = O22BYTE2(dwQuadletData);
  pbyWriteQuadletRequest[15] = O22BYTE3(dwQuadletData);

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::UnpackReadQuadletResponse(BYTE  * pbyReadQuadletResponse,
                                                BYTE  * pbyTransactionLabel, 
                                                BYTE  * pbyResponseCode,
                                                DWORD * pdwQuadletData)
//-------------------------------------------------------------------------------------------------
// Unpack a read quadlet response from the brain
//-------------------------------------------------------------------------------------------------
{
  // Check for correct Transaction Code
  BYTE byTransactionCode = pbyReadQuadletResponse[3] >> 4;
  if (SIOMM_TCODE_READ_QUAD_RESPONSE != byTransactionCode)
  {
    return SIOMM_ERROR;
  }

  *pbyTransactionLabel = pbyReadQuadletResponse[2];
  *pbyTransactionLabel >>= 2;

  *pbyResponseCode = pbyReadQuadletResponse[6];
  *pbyResponseCode >>= 4;

  *pdwQuadletData = O22MAKELONG(pbyReadQuadletResponse[12], pbyReadQuadletResponse[13],
                                pbyReadQuadletResponse[14], pbyReadQuadletResponse[15]);

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::UnpackReadBlockResponse(BYTE  * pbyReadBlockResponse,
                                              BYTE  * pbyTransactionLabel, 
                                              BYTE  * pbyResponseCode,
                                              WORD  * pwDataLength,
                                              BYTE  * pbyBlockData)
//-------------------------------------------------------------------------------------------------
// Unpack a read block response from the brain
//-------------------------------------------------------------------------------------------------
{
  // Check for correct Transaction Code
  BYTE byTransactionCode = pbyReadBlockResponse[3] >> 4;
  if (SIOMM_TCODE_READ_BLOCK_RESPONSE != byTransactionCode)
  {
    return SIOMM_ERROR;
  }

  *pbyTransactionLabel = pbyReadBlockResponse[2];
  *pbyTransactionLabel >>= 2;

  *pbyResponseCode = pbyReadBlockResponse[6];
  *pbyResponseCode >>= 4;

  *pwDataLength = O22MAKEWORD(pbyReadBlockResponse[12], pbyReadBlockResponse[13]);

  memcpy(pbyBlockData, &(pbyReadBlockResponse[16]), *pwDataLength);

  return SIOMM_OK;
}



LONG O22SnapIoMemMap::UnpackWriteResponse(BYTE  * pbyWriteResponse,
                                          BYTE  * pbyTransactionLabel, 
                                          BYTE  * pbyResponseCode)
//-------------------------------------------------------------------------------------------------
// Unpack a write response from the brain
//-------------------------------------------------------------------------------------------------
{
  // Check for correct Transaction Code
  BYTE byTransactionCode = pbyWriteResponse[3] >> 4;
  if (SIOMM_TCODE_WRITE_RESPONSE != byTransactionCode)
  {
    return SIOMM_ERROR;
  }

  // Unpack Transaction Label
  *pbyTransactionLabel = pbyWriteResponse[2];
  *pbyTransactionLabel >>= 2;

  // Unpack Response Code
  *pbyResponseCode = pbyWriteResponse[6];
  *pbyResponseCode >>= 4;

  return SIOMM_OK;
}


LONG O22SnapIoMemMap::ReadFloat(DWORD dwDestOffset, float * pfValue)
//-------------------------------------------------------------------------------------------------
// Read a float value from a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  DWORD dwQuadlet; // A temp for getting the read value
  LONG  nResult;   // for checking the return values of functions
  
  nResult = ReadQuad(dwDestOffset, &dwQuadlet);

  // Check the result
  if (nResult == SIOMM_OK)
  {
    // If the ReadQuad was OK, copy the data
    memcpy(pfValue, &dwQuadlet, 4);
  }

  return nResult;
}


LONG O22SnapIoMemMap::WriteFloat(DWORD dwDestOffset, float fValue)
//-------------------------------------------------------------------------------------------------
// Write a float value to a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  DWORD dwQuadlet; // A temp for setting the write value

  // Copy the float into a DWORD for easy manipulation
  memcpy(&dwQuadlet, &fValue, 4);
  
  return WriteQuad(dwDestOffset, dwQuadlet);
}


LONG O22SnapIoMemMap::ReadBlock(DWORD dwDestOffset, WORD wDataLength, BYTE * pbyData)
//-------------------------------------------------------------------------------------------------
// Read a block of data from a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  BYTE  byReadBlockRequest[SIOMM_SIZE_READ_BLOCK_REQUEST];
  BYTE *pbyReadBlockResponse;
  BYTE  byTransactionLabel;
  BYTE  byResponseCode;
  BYTE *pbyDataTemp;
  LONG  nResult;
  fd_set fds;


  // Check that we have a valid socket
  if (INVALID_SOCKET == m_Socket)
  {
    return SIOMM_ERROR_NOT_CONNECTED;
  }

  // Increment the transaction label
  UpdateTransactionLabel();

  // The response will be padded to land on a quadlet boundary;
/*
  wDataLengthTemp = wDataLength;
  while ((wDataLengthTemp%4) != 0)
    wDataLengthTemp++;
*/

  // Allocate the response buffer
  pbyReadBlockResponse = new BYTE[wDataLength/*Temp*/ + SIOMM_SIZE_READ_BLOCK_RESPONSE];
  if (pbyReadBlockResponse == NULL)
    return SIOMM_ERROR_OUT_OF_MEMORY; // Couldn't allocate memory!

  FD_ZERO(&fds);
  FD_SET(m_Socket, &fds);

  // Build the request packet
  BuildReadBlockRequest(byReadBlockRequest, m_byTransactionLabel, dwDestOffset, wDataLength);

  // Send the packet to the Snap I/O unit
  nResult = send(m_Socket, (char*)byReadBlockRequest, SIOMM_SIZE_READ_BLOCK_REQUEST, 0 /*??*/ );
  if (SOCKET_ERROR == nResult)
  {
    delete [] pbyReadBlockResponse;

#ifdef _WIN32
    nResult = WSAGetLastError();  // for checking the specific error
#endif

    return SIOMM_ERROR; // This probably means we're not connected.
  }

#ifdef _LINUX
  // In the call to select, Linux modifies m_tvTimeOut.  It needs to be reset every time.
  m_tvTimeOut.tv_sec  = m_nTimeOutMS / 1000;
  m_tvTimeOut.tv_usec = (m_nTimeOutMS % 1000) * 1000;
#endif

  // Is the recv ready?
  if (0 == select(m_Socket + 1, &fds, NULL, NULL, &m_tvTimeOut)) // ?? Param#1 is ignored by Windows.  What should it be in other systems?
  {
    // we timed-out
    delete [] pbyReadBlockResponse;

    return SIOMM_TIME_OUT;
  }


  // The response is ready, so recv it.
  nResult = recv(m_Socket, (char*)pbyReadBlockResponse, 
                 wDataLength/*Temp*/ + SIOMM_SIZE_READ_BLOCK_RESPONSE, 0 /*??*/);

  if ((wDataLength/*Temp*/ + SIOMM_SIZE_READ_BLOCK_RESPONSE) != nResult)
  {
    // we got the wrong number of bytes back!
    
    delete [] pbyReadBlockResponse;

    return SIOMM_ERROR_RESPONSE_BAD;
  }

  // Allocate a temporary data buffer
  pbyDataTemp = new BYTE[wDataLength];
  if (pbyDataTemp == NULL)
  {
    delete [] pbyReadBlockResponse;

    return SIOMM_ERROR_OUT_OF_MEMORY; // Couldn't allocate memory!
  }

  // Unpack the response.
  nResult = UnpackReadBlockResponse(pbyReadBlockResponse,
                                    &byTransactionLabel, &byResponseCode, 
                                    &wDataLength/*Temp*/, pbyDataTemp);

  // Clean up memory
  delete [] pbyReadBlockResponse;

  // Check that the response was okay
  if ((SIOMM_OK == nResult) && 
      (SIOMM_RESPONSE_CODE_ACK == byResponseCode) && 
      (m_byTransactionLabel == byTransactionLabel))
  {
    // The response was good, so copy the data   
    memcpy(pbyData, pbyDataTemp, wDataLength/*Temp*/);
    delete [] pbyDataTemp;
    return SIOMM_OK;
  }
  else if ((SIOMM_OK == nResult) && (SIOMM_RESPONSE_CODE_ACK != byResponseCode))
  {
    // Clean up memory
    delete [] pbyDataTemp;

    // If a bad response from the brain, get its last error code
    long nErrorCode;
    nResult = GetStatusLastError(&nErrorCode);
    if (SIOMM_OK == nResult)
    {
      return nErrorCode;
    }
    else
    {
      return nResult;
    }
  }
  else
  {
    delete [] pbyDataTemp;
    return SIOMM_ERROR_RESPONSE_BAD;
  }
}


    
LONG O22SnapIoMemMap::ReadQuad(DWORD dwDestOffset, DWORD * pdwQuadlet)
//-------------------------------------------------------------------------------------------------
// Read a quadlet of data from a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  BYTE  byReadQuadletRequest[SIOMM_SIZE_READ_QUAD_REQUEST];
  BYTE  byReadQuadletResponse[SIOMM_SIZE_READ_QUAD_RESPONSE];
  BYTE  byTransactionLabel;
  BYTE  byResponseCode;
  DWORD dwQuadletTemp;
  LONG  nResult;
  fd_set fds;


  // Check that we have a valid socket
  if (INVALID_SOCKET == m_Socket)
  {
    return SIOMM_ERROR_NOT_CONNECTED;
  }

  // Increment the transaction label
  UpdateTransactionLabel();

  FD_ZERO(&fds);
  FD_SET(m_Socket, &fds);

  // Build the request packet
  BuildReadQuadletRequest(byReadQuadletRequest, m_byTransactionLabel, dwDestOffset);

  // Send the packet to the Snap I/O unit
  nResult = send(m_Socket, (char*)byReadQuadletRequest, SIOMM_SIZE_READ_QUAD_REQUEST, 0 /*??*/ );
  if (SOCKET_ERROR == nResult)
  {
#ifdef _WIN32
    nResult = WSAGetLastError();  // for checking the specific error
#endif

    return SIOMM_ERROR; // This probably means we're not connected.
  }

#ifdef _LINUX
  // In the call to select, Linux modifies m_tvTimeOut.  It needs to be reset every time.
  m_tvTimeOut.tv_sec  = m_nTimeOutMS / 1000;
  m_tvTimeOut.tv_usec = (m_nTimeOutMS % 1000) * 1000;
#endif

  // Is the recv ready?
  if (0 == select(m_Socket + 1, &fds, NULL, NULL, &m_tvTimeOut)) // ?? Param#1 is ignored by Windows.  What should it be in other systems?
  {
    // we timed-out
    return SIOMM_TIME_OUT;
  }

  // The response is ready, so recv it.
  nResult = recv(m_Socket, (char*)byReadQuadletResponse, SIOMM_SIZE_READ_QUAD_RESPONSE, 0 /*??*/);

  if (SIOMM_SIZE_READ_QUAD_RESPONSE != nResult)
  {
    // we got the wrong number of bytes back!
    return SIOMM_ERROR_RESPONSE_BAD;
  }


  // Unpack the response.
  nResult = UnpackReadQuadletResponse(byReadQuadletResponse,
                                      &byTransactionLabel, 
                                      &byResponseCode, &dwQuadletTemp);

  // Check that the response was okay
  if ((SIOMM_OK == nResult) && 
      (SIOMM_RESPONSE_CODE_ACK == byResponseCode) && 
      (m_byTransactionLabel == byTransactionLabel))
  {
    // The response was good, so copy the quadlet
    *pdwQuadlet = dwQuadletTemp;
    return SIOMM_OK;
  }
  else if ((SIOMM_OK == nResult) && (SIOMM_RESPONSE_CODE_ACK != byResponseCode))
  {
    // If a bad response from the brain, get its last error code
    long nErrorCode;
    nResult = GetStatusLastError(&nErrorCode);
    if (SIOMM_OK == nResult)
    {
      return nErrorCode;
    }
    else
    {
      return nResult;
    }
  }
  else
  {
    return SIOMM_ERROR_RESPONSE_BAD;
  }
}


LONG O22SnapIoMemMap::WriteBlock(DWORD dwDestOffset, WORD wDataLength, BYTE * pbyData)
//-------------------------------------------------------------------------------------------------
// Write a block of data to a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  BYTE *pbyWriteBlockRequest;
  BYTE  byWriteBlockResponse[SIOMM_SIZE_WRITE_RESPONSE];
  BYTE  byTransactionLabel;
  BYTE  byResponseCode;
  LONG  nResult;
  fd_set fds;

    // Check that we have a valid socket
  if (INVALID_SOCKET == m_Socket)
  {
    return SIOMM_ERROR_NOT_CONNECTED;
  }

  // Increment the transaction label
  UpdateTransactionLabel();

  FD_ZERO(&fds);
  FD_SET(m_Socket, &fds);

  pbyWriteBlockRequest = new BYTE[SIOMM_SIZE_WRITE_BLOCK_REQUEST + wDataLength];
  if (pbyWriteBlockRequest == NULL)
    return SIOMM_ERROR_OUT_OF_MEMORY; // Couldn't allocate memory!

  // Build the write request packet
  BuildWriteBlockRequest(pbyWriteBlockRequest, m_byTransactionLabel, 
                         dwDestOffset, wDataLength, pbyData);

  // Send the packet to the Snap I/O unit
  nResult = send(m_Socket, (char*)pbyWriteBlockRequest, SIOMM_SIZE_WRITE_BLOCK_REQUEST + wDataLength, 0 /*??*/ );

  // Cleanup memory
  delete [] pbyWriteBlockRequest;

  // Check the result from send()
  if (SOCKET_ERROR == nResult)
  {
    return SIOMM_ERROR; // oops!
  }


#ifdef _LINUX
  // In the call to select, Linux modifies m_tvTimeOut.  It needs to be reset every time.
  m_tvTimeOut.tv_sec  = m_nTimeOutMS / 1000;
  m_tvTimeOut.tv_usec = (m_nTimeOutMS % 1000) * 1000;
#endif

  // Is the recv ready?
  if (0 == select(m_Socket + 1, &fds, NULL, NULL, &m_tvTimeOut)) // ?? Param#1 is ignored by Windows.  What should it be in other systems?
  {
    // we timed-out
    return SIOMM_TIME_OUT;
  }

  // The response is ready, so recv it.
  nResult = recv(m_Socket, (char*)byWriteBlockResponse, SIOMM_SIZE_WRITE_RESPONSE, 0 /*??*/);

  if (SIOMM_SIZE_WRITE_RESPONSE != nResult)
  {
    // we got the wrong number of bytes back!
    return SIOMM_ERROR_RESPONSE_BAD;
  }
 
  // Unpack the response
  nResult = UnpackWriteResponse(byWriteBlockResponse, &byTransactionLabel, 
                                &byResponseCode);

  // Check that the response was okay
  if ((SIOMM_OK == nResult) && 
      (SIOMM_RESPONSE_CODE_ACK == byResponseCode) && 
      (m_byTransactionLabel == byTransactionLabel))
  {
    // The response was good!
    return SIOMM_OK;
  }
  else if ((SIOMM_OK == nResult) && (SIOMM_RESPONSE_CODE_ACK != byResponseCode))
  {
    // If a bad response from the brain, get its last error code
    long nErrorCode;
    nResult = GetStatusLastError(&nErrorCode);
    if (SIOMM_OK == nResult)
    {
      return nErrorCode;
    }
    else
    {
      return nResult;
    }
  }
  else
  {
    return SIOMM_ERROR_RESPONSE_BAD;
  }
}


LONG O22SnapIoMemMap::WriteQuad(DWORD dwDestOffset, DWORD dwQuadlet)
//-------------------------------------------------------------------------------------------------
// Write a quadlet of data to a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  BYTE  byWriteQuadletRequest[SIOMM_SIZE_WRITE_QUAD_REQUEST];
  BYTE  byWriteQuadletResponse[SIOMM_SIZE_WRITE_RESPONSE];
  BYTE  byTransactionLabel;
  BYTE  byResponseCode;
  LONG  nResult;
  fd_set fds;

  // Check that we have a valid socket
  if (INVALID_SOCKET == m_Socket)
  {
    return SIOMM_ERROR_NOT_CONNECTED;
  }

  // Increment the transaction label
  UpdateTransactionLabel();

  FD_ZERO(&fds);
  FD_SET(m_Socket, &fds);

  // Build the write request packet
  BuildWriteQuadletRequest(byWriteQuadletRequest, m_byTransactionLabel, 1, 
                           dwDestOffset, dwQuadlet);


  // Send the packet to the Snap I/O unit
  nResult = send(m_Socket, (char*)byWriteQuadletRequest, SIOMM_SIZE_WRITE_QUAD_REQUEST, 0 /*??*/ );
  if (SOCKET_ERROR == nResult)
  {
    return SIOMM_ERROR; // oops!
  }

#ifdef _LINUX
  // In the call to select, Linux modifies m_tvTimeOut.  It needs to be reset every time.
  m_tvTimeOut.tv_sec  = m_nTimeOutMS / 1000;
  m_tvTimeOut.tv_usec = (m_nTimeOutMS % 1000) * 1000;
#endif

  // Is the recv ready?
  if (0 == select(m_Socket + 1, &fds, NULL, NULL, &m_tvTimeOut)) // ?? Param#1 is ignored by Windows.  What should it be in other systems?
  {
    // we timed-out
    return SIOMM_TIME_OUT;
  }

  // The response is ready, so recv it.
  nResult = recv(m_Socket, (char*)byWriteQuadletResponse, SIOMM_SIZE_WRITE_RESPONSE, 0 /*??*/);


  if (SIOMM_SIZE_WRITE_RESPONSE != nResult)
  {
    // we got the wrong number of bytes back!
    return SIOMM_ERROR_RESPONSE_BAD;
  }
 
  // Unpack the response
  nResult = UnpackWriteResponse(byWriteQuadletResponse, &byTransactionLabel, &byResponseCode);

  // Check that the response was okay
  if ((SIOMM_OK == nResult) && 
      (SIOMM_RESPONSE_CODE_ACK == byResponseCode) && 
      (m_byTransactionLabel == byTransactionLabel))
  {
    // The response was good!
    return SIOMM_OK;
  }
  else if ((SIOMM_OK == nResult) && (SIOMM_RESPONSE_CODE_ACK != byResponseCode))
  {
    // If a bad response from the brain, get its last error code
    long nErrorCode;
    nResult = GetStatusLastError(&nErrorCode);
    if (SIOMM_OK == nResult)
    {
      return nErrorCode;
    }
    else
    {
      return nResult;
    }
  }
  else
  {
    return SIOMM_ERROR_RESPONSE_BAD;
  }
}


LONG O22SnapIoMemMap::Close()
//-------------------------------------------------------------------------------------------------
// Close the connection to the I/O unit
//-------------------------------------------------------------------------------------------------
{
  return CloseSockets();
}


LONG O22SnapIoMemMap::GetDigPtState(long nPoint, long *pnState)
//-------------------------------------------------------------------------------------------------
// Get the state of the specified digital point.
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_STATE + (SIOMM_DPOINT_READ_BOUNDARY * nPoint), 
                   (DWORD*)pnState);
}


LONG O22SnapIoMemMap::GetDigPtOnLatch(long nPoint, long *pnState)
//-------------------------------------------------------------------------------------------------
// Get the on-latch state of the specified digital point.
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_ONLATCH_STATE + (SIOMM_DPOINT_READ_BOUNDARY * nPoint), 
                   (DWORD*)pnState);

}

LONG O22SnapIoMemMap::GetDigPtOffLatch(long nPoint, long *pnState)
//-------------------------------------------------------------------------------------------------
// Get the off-latch state of the specified digital point.
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_OFFLATCH_STATE + (SIOMM_DPOINT_READ_BOUNDARY * nPoint), 
                   (DWORD*)pnState);

}

LONG O22SnapIoMemMap::GetDigPtCounterState(long nPoint, long *pnState)
//-------------------------------------------------------------------------------------------------
// Get the active counter state of the specified digital point.
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_ACTIVE_COUNTER + (SIOMM_DPOINT_READ_BOUNDARY * nPoint), 
                   (DWORD*)pnState);

}

LONG O22SnapIoMemMap::GetDigPtCounts(long nPoint, long *pnValue)
//-------------------------------------------------------------------------------------------------
// Get the counters of the specified digital point.
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_COUNTER_DATA + (SIOMM_DPOINT_READ_BOUNDARY * nPoint), 
                   (DWORD*)pnValue);

}

LONG O22SnapIoMemMap::GetDigPtReadAreaEx(long nPoint, SIOMM_DigPointReadArea * pData)
//-------------------------------------------------------------------------------------------------
// Get the read area for the specified digital point
//-------------------------------------------------------------------------------------------------
{

  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[20]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_DPOINT_READ_AREA_BASE + (SIOMM_DPOINT_READ_BOUNDARY * nPoint), 
                      20, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pData->nState        = O22MAKELONG2(arrbyData, 0);
    pData->nOnLatch      = O22MAKELONG2(arrbyData, 4);
    pData->nOffLatch     = O22MAKELONG2(arrbyData, 8);
    pData->nCounterState = O22MAKELONG2(arrbyData, 12);
    pData->nCounts       = O22MAKELONG2(arrbyData, 16);
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetDigPtState(long nPoint, long nState)
//-------------------------------------------------------------------------------------------------
// Set the state of the specified digital point
//-------------------------------------------------------------------------------------------------
{
  if (nState)
    return  WriteQuad(SIOMM_DPOINT_WRITE_TURN_ON_BASE  + (SIOMM_DPOINT_WRITE_BOUNDARY * nPoint), 1);
  else
    return  WriteQuad(SIOMM_DPOINT_WRITE_TURN_OFF_BASE + (SIOMM_DPOINT_WRITE_BOUNDARY * nPoint), 1);
}


LONG O22SnapIoMemMap::SetDigPtCounterState(long nPoint, long nState)
//-------------------------------------------------------------------------------------------------
// Set the active counter state of the specified digital point
//-------------------------------------------------------------------------------------------------
{
  if (nState)
    return  WriteQuad(SIOMM_DPOINT_WRITE_ACTIVATE_COUNTER   + (SIOMM_DPOINT_WRITE_BOUNDARY * nPoint), 1);
  else
    return  WriteQuad(SIOMM_DPOINT_WRITE_DEACTIVATE_COUNTER + (SIOMM_DPOINT_WRITE_BOUNDARY * nPoint), 1);
}


LONG O22SnapIoMemMap::GetBitmask64(DWORD dwDestOffset, long *pnPts63to32, long *pnPts31to0)
//-------------------------------------------------------------------------------------------------
// Get a 64-bit bitmask from a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[8]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(dwDestOffset, 8, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and make the longs
    *pnPts63to32 = O22MAKELONG2(arrbyData, 0);
    *pnPts31to0  = O22MAKELONG2(arrbyData, 4);
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetBitmask64(DWORD dwDestOffset, long nPts63to32, long nPts31to0)
//-------------------------------------------------------------------------------------------------
// Set a 64-bit bitmask to a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[8]; // buffer for the data to be read

  // Unpack the bytes
  O22FILL_ARRAY_FROM_LONG(arrbyData, 0,  nPts63to32);
  O22FILL_ARRAY_FROM_LONG(arrbyData, 4,  nPts31to0);

  // Write the data
  nResult = WriteBlock(dwDestOffset, 8, (BYTE*)arrbyData);

  return nResult;
}


LONG O22SnapIoMemMap::GetDigBankPointStates(long *pnPts63to32, long *pnPts31to0)
//-------------------------------------------------------------------------------------------------
// Get the digital point states in the digital bank
//-------------------------------------------------------------------------------------------------
{
  return GetBitmask64(SIOMM_DBANK_READ_POINT_STATES, pnPts63to32, pnPts31to0); 
}


LONG O22SnapIoMemMap::GetDigBankOnLatchStates(long *pnPts63to32, long *pnPts31to0)
//-------------------------------------------------------------------------------------------------
// Get the digital point on-latch states in the digital bank
//-------------------------------------------------------------------------------------------------
{
  return GetBitmask64(SIOMM_DBANK_READ_ON_LATCH_STATES, pnPts63to32, pnPts31to0); 
}


LONG O22SnapIoMemMap::GetDigBankOffLatchStates(long *pnPts63to32, long *pnPts31to0)
//-------------------------------------------------------------------------------------------------
// Get the digital point off-latch states in the digital bank
//-------------------------------------------------------------------------------------------------
{
  return GetBitmask64(SIOMM_DBANK_READ_OFF_LATCH_STATES, pnPts63to32, pnPts31to0); 
}


LONG O22SnapIoMemMap::GetDigBankActCounterStates(long *pnPts63to32, long *pnPts31to0)
//-------------------------------------------------------------------------------------------------
// Get the digital point active counter states in the digital bank
//-------------------------------------------------------------------------------------------------
{
  return GetBitmask64(SIOMM_DBANK_READ_ACTIVE_COUNTERS, pnPts63to32, pnPts31to0); 
}


LONG O22SnapIoMemMap::SetDigBankPointStates(long nPts63to32, long nPts31to0, 
                                            long nMask63to32, long nMask31to0)
//-------------------------------------------------------------------------------------------------
// Set the digital point states in the digital bank.  Only those points set in the mask parameters
// are set.
//-------------------------------------------------------------------------------------------------
{
  long nOnMask63to32;
  long nOnMask31to0;
  long nOffMask63to32;
  long nOffMask31to0;
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[16]; // buffer for the data to be read

  // Figure out the On Mask
  nOnMask63to32 = nPts63to32 & nMask63to32;
  nOnMask31to0 = nPts31to0 & nMask31to0;

  // Figure out the Off Mask
  nOffMask63to32 = (nPts63to32 ^ 0xFFFFFFFF) & nMask63to32;
  nOffMask31to0  = (nPts31to0  ^ 0xFFFFFFFF) & nMask31to0;

  // Pack the bytes
  O22FILL_ARRAY_FROM_LONG(arrbyData, 0,  nOnMask63to32);
  O22FILL_ARRAY_FROM_LONG(arrbyData, 4,  nOnMask31to0);
  O22FILL_ARRAY_FROM_LONG(arrbyData, 8,  nOffMask63to32);
  O22FILL_ARRAY_FROM_LONG(arrbyData, 12, nOffMask31to0);

  // Write the data
  nResult = WriteBlock(SIOMM_DBANK_WRITE_TURN_ON_MASK, 16, (BYTE*)arrbyData);

  return nResult;

}

    
LONG O22SnapIoMemMap::SetDigBankOnMask(long nPts63to32, long nPts31to0)
//-------------------------------------------------------------------------------------------------
// Turn on the given digital points
//-------------------------------------------------------------------------------------------------
{
  return SetBitmask64(SIOMM_DBANK_WRITE_TURN_ON_MASK, nPts63to32, nPts31to0); 
}

LONG O22SnapIoMemMap::SetDigBankOffMask(long nPts63to32, long nPts31to0)
//-------------------------------------------------------------------------------------------------
// Turn off the given digital points
//-------------------------------------------------------------------------------------------------
{
  return SetBitmask64(SIOMM_DBANK_WRITE_TURN_OFF_MASK, nPts63to32, nPts31to0); 
}

LONG O22SnapIoMemMap::SetDigBankActCounterMask(long nPts63to32, long nPts31to0)
//-------------------------------------------------------------------------------------------------
// Active counters for the given digital points
//-------------------------------------------------------------------------------------------------
{
  return SetBitmask64(SIOMM_DBANK_WRITE_ACT_COUNTERS_MASK, nPts63to32, nPts31to0); 
}

LONG O22SnapIoMemMap::SetDigBankDeactCounterMask(long nPts63to32, long nPts31to0)
//-------------------------------------------------------------------------------------------------
// Deactive counters for the given digital points
//-------------------------------------------------------------------------------------------------
{
  return SetBitmask64(SIOMM_DBANK_WRITE_DEACT_COUNTERS_MASK, nPts63to32, nPts31to0); 
}


LONG O22SnapIoMemMap::GetDigBankReadAreaEx(SIOMM_DigBankReadArea * pData)
//-------------------------------------------------------------------------------------------------
// Get the read area for the digital bank
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;       // for checking the return values of functions
  BYTE arrbyData[32]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_DBANK_READ_AREA_BASE, 32, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pData->nStatePts63to32          = O22MAKELONG2(arrbyData, 0);
    pData->nStatePts31to0           = O22MAKELONG2(arrbyData, 4);
    pData->nOnLatchStatePts63to32   = O22MAKELONG2(arrbyData, 8);
    pData->nOnLatchStatePts31to0    = O22MAKELONG2(arrbyData, 12);
    pData->nOffLatchStatePts63to32  = O22MAKELONG2(arrbyData, 16);
    pData->nOffLatchStatePts31to0   = O22MAKELONG2(arrbyData, 20);
    pData->nActiveCountersPts63to32 = O22MAKELONG2(arrbyData, 24);
    pData->nActiveCountersPts31to0  = O22MAKELONG2(arrbyData, 28);
  }

  return nResult;
}


LONG O22SnapIoMemMap::ReadClearDigPtCounts(long nPoint, long * pnState)
//-------------------------------------------------------------------------------------------------
// Read and clear counts for the specified digital point
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_CLEAR_COUNTS_BASE + (SIOMM_DPOINT_READ_CLEAR_BOUNDARY * nPoint), 
                   (DWORD*)pnState);
}


LONG O22SnapIoMemMap::ReadClearDigPtOnLatch(long nPoint, long * pnState)
//-------------------------------------------------------------------------------------------------
// Read and clear the on-latch state for the specified digital point
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_CLEAR_ON_LATCH_BASE + (SIOMM_DPOINT_READ_CLEAR_BOUNDARY * nPoint), 
                   (DWORD*)pnState);
}


LONG O22SnapIoMemMap::ReadClearDigPtOffLatch(long nPoint, long * pnState)
//-------------------------------------------------------------------------------------------------
// Read and clear the off-latch state for the specified digital point
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_DPOINT_READ_CLEAR_OFF_LATCH_BASE + (SIOMM_DPOINT_READ_CLEAR_BOUNDARY * nPoint), 
                   (DWORD*)pnState);
}



LONG O22SnapIoMemMap::GetAnaPtValue(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Get the value of the specified analog point.
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_VALUE_BASE + (SIOMM_APOINT_READ_BOUNDARY * nPoint),
                   pfValue);
}


LONG O22SnapIoMemMap::GetAnaPtCounts(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Get the raw counts of the specified analog point.
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_COUNTS_BASE + (SIOMM_APOINT_READ_BOUNDARY * nPoint),
                   pfValue);
}


LONG O22SnapIoMemMap::GetAnaPtMinValue(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Get the minimum value of the specified analog point.
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_MIN_VALUE_BASE + (SIOMM_APOINT_READ_BOUNDARY * nPoint),
                   pfValue);
}


LONG O22SnapIoMemMap::GetAnaPtMaxValue(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Get the maximum value of the specified analog point.
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_MAX_VALUE_BASE + (SIOMM_APOINT_READ_BOUNDARY * nPoint),
                   pfValue);
}


LONG O22SnapIoMemMap::GetAnaPtTpoPeriod(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Get the TPO period of the specified analog point.
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_TPO_PERIOD_BASE + (SIOMM_APOINT_READ_BOUNDARY * nPoint),
                   pfValue);
}


LONG O22SnapIoMemMap::GetAnaPtReadAreaEx(long nPoint, SIOMM_AnaPointReadArea * pData)
//-------------------------------------------------------------------------------------------------
// Get the read area for the specified analog point
//-------------------------------------------------------------------------------------------------
{
  LONG  nResult;       // for checking the return values of functions
  BYTE  arrbyData[16]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_APOINT_READ_AREA_BASE + (SIOMM_APOINT_READ_BOUNDARY * nPoint), 
                      16, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pData->fValue    = O22MAKEFLOAT2(arrbyData, 0);
    pData->fCounts   = O22MAKEFLOAT2(arrbyData, 4);
    pData->fMinValue = O22MAKEFLOAT2(arrbyData, 8);
    pData->fMaxValue = O22MAKEFLOAT2(arrbyData, 12);
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetAnaPtValue(long nPoint, float fValue)
//-------------------------------------------------------------------------------------------------
// Set the value for the specified analog point
//-------------------------------------------------------------------------------------------------
{
  return WriteFloat(SIOMM_APOINT_WRITE_VALUE_BASE + (SIOMM_APOINT_WRITE_BOUNDARY * nPoint), 
                    fValue);
}


LONG O22SnapIoMemMap::SetAnaPtCounts(long nPoint, float fValue)
//-------------------------------------------------------------------------------------------------
// Set the raw counts for the specified analog point
//-------------------------------------------------------------------------------------------------
{
  return WriteFloat(SIOMM_APOINT_WRITE_COUNTS_BASE + (SIOMM_APOINT_WRITE_BOUNDARY * nPoint), 
                    fValue);
}


LONG O22SnapIoMemMap::SetAnaPtTpoPeriod(long nPoint, float fValue)
//-------------------------------------------------------------------------------------------------
// Set the TPO period for the specified analog point
//-------------------------------------------------------------------------------------------------
{
  return WriteFloat(SIOMM_APOINT_WRITE_TPO_PERIOD_BASE + (SIOMM_APOINT_WRITE_BOUNDARY * nPoint), 
                    fValue);
}


LONG O22SnapIoMemMap::ReadClearAnaPtMinValue(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Read and clear the minimum value for the specified analog point
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_CLEAR_MIN_VALUE_BASE + (SIOMM_APOINT_READ_CLEAR_BOUNDARY * nPoint), 
                   pfValue);
}


LONG O22SnapIoMemMap::ReadClearAnaPtMaxValue(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Read and clear the maximum value for the specified analog point
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_CLEAR_MAX_VALUE_BASE + (SIOMM_APOINT_READ_CLEAR_BOUNDARY * nPoint), 
                   pfValue);
}


LONG O22SnapIoMemMap::ConfigurePoint(long nPoint, long nPointType)
//-------------------------------------------------------------------------------------------------
// Configure the specified point
//-------------------------------------------------------------------------------------------------
{
  return WriteQuad(SIOMM_POINT_CONFIG_WRITE_TYPE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                   nPointType);
}


LONG O22SnapIoMemMap::GetModuleType(long nPoint, long * pnModuleType)
//-------------------------------------------------------------------------------------------------
// Get the module type at the specified point position
//-------------------------------------------------------------------------------------------------
{
  return  ReadQuad(SIOMM_POINT_CONFIG_READ_MOD_TYPE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                   (DWORD*)pnModuleType);
}


LONG O22SnapIoMemMap::SetDigPtConfiguration(long nPoint, long nPointType, long nFeature)
//-------------------------------------------------------------------------------------------------
// Configure a digital point
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[8]; // buffer for the data to be read

  // Build the data area
  O22FILL_ARRAY_FROM_LONG (arrbyData, 0,  nPointType);
  O22FILL_ARRAY_FROM_LONG (arrbyData, 4,  nFeature);

  // Write the data
  nResult = WriteBlock(SIOMM_POINT_CONFIG_WRITE_TYPE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                       8, (BYTE*)arrbyData);

  return nResult;
}


LONG O22SnapIoMemMap::SetAnaPtConfiguration(long nPoint, long nPointType, 
                                            float fOffset, float fGain, 
                                            float fHiScale, float fLoScale)
//-------------------------------------------------------------------------------------------------
// Configure an analog point
//-------------------------------------------------------------------------------------------------
{
  BYTE arrbyData[24]; // buffer for the data to be read

  // Build the data area
  O22FILL_ARRAY_FROM_LONG (arrbyData, 0, nPointType);
  O22FILL_ARRAY_FROM_LONG (arrbyData, 4, 0);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 8,  fOffset);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 12, fGain);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 16, fHiScale);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 20, fLoScale);

  // Write the data
  return WriteBlock(SIOMM_POINT_CONFIG_WRITE_TYPE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                    24, (BYTE*)arrbyData);
}


LONG O22SnapIoMemMap::SetPtConfigurationEx2(long nPoint, SIOMM_PointConfigArea2 PtConfigData)
//-------------------------------------------------------------------------------------------------
// Configure a digital or analog point
//-------------------------------------------------------------------------------------------------
{
  BYTE arrbyData[SIOMM_POINT_CONFIG_BOUNDARY]; // buffer for the data to be read
  LONG  nResult;

  // Build the data area
  O22FILL_ARRAY_FROM_LONG (arrbyData, 0,  PtConfigData.nPointType);
  O22FILL_ARRAY_FROM_LONG (arrbyData, 4,  PtConfigData.nFeature);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 8,  PtConfigData.fOffset);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 12, PtConfigData.fGain);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 16, PtConfigData.fHiScale);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 20, PtConfigData.fLoScale);
  // Bytes 24-27 are not used at this time. 
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 28, PtConfigData.fFilterWeight);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 32, PtConfigData.fWatchdogValue);
  O22FILL_ARRAY_FROM_LONG (arrbyData, 36, PtConfigData.nWatchdogEnabled);

  memcpy(arrbyData + 44, PtConfigData.byName, SIOMM_POINT_CONFIG_NAME_SIZE);

  // Write the first section
  nResult = WriteBlock(SIOMM_POINT_CONFIG_WRITE_TYPE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint),
                       24, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK != nResult)
    return nResult;

  // Write the second section
  nResult = WriteBlock(SIOMM_POINT_CONFIG_WRITE_TYPE_BASE + 28 + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                       12, (BYTE*)(&(arrbyData[28])));

  // Check for error
  if (SIOMM_OK != nResult)
    return nResult;

  // Write the third section
  return WriteBlock(SIOMM_POINT_CONFIG_WRITE_TYPE_BASE + 44 + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                    SIOMM_POINT_CONFIG_NAME_SIZE, (BYTE*)(&(arrbyData[44])));

}


LONG O22SnapIoMemMap::SetPtConfigurationEx(long nPoint, SIOMM_PointConfigArea PtConfigData)
//-------------------------------------------------------------------------------------------------
// Configure a digital or analog point
//-------------------------------------------------------------------------------------------------
{
  BYTE arrbyData[40]; // buffer for the data to be read
  LONG  nResult;

  // Build the data area
  O22FILL_ARRAY_FROM_LONG (arrbyData, 0,  PtConfigData.nPointType);
  O22FILL_ARRAY_FROM_LONG (arrbyData, 4,  PtConfigData.nFeature);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 8,  PtConfigData.fOffset);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 12, PtConfigData.fGain);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 16, PtConfigData.fHiScale);
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 20, PtConfigData.fLoScale);
  // Bytes 24-31 are not used at this time. 
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 32, PtConfigData.fWatchdogValue);
  O22FILL_ARRAY_FROM_LONG (arrbyData, 36, PtConfigData.nWatchdogEnabled);

  // Write the first section
  nResult = WriteBlock(SIOMM_POINT_CONFIG_WRITE_TYPE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint),
                       24, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK != nResult)
    return nResult;

  // Write the second section
  return WriteBlock(SIOMM_POINT_CONFIG_WRITE_TYPE_BASE + 32 + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                    8, (BYTE*)(&(arrbyData[32])));
}


LONG O22SnapIoMemMap::GetPtConfigurationEx2(long nPoint, SIOMM_PointConfigArea2 * pPtConfigData)
//-------------------------------------------------------------------------------------------------
// Get the configuration for a point
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[SIOMM_POINT_CONFIG_BOUNDARY]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_POINT_CONFIG_READ_MOD_TYPE_BASE + 
                      (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                      SIOMM_POINT_CONFIG_BOUNDARY, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pPtConfigData->nModuleType      = O22MAKELONG2 (arrbyData, 0);
    pPtConfigData->nPointType       = O22MAKELONG2 (arrbyData, 4);
    pPtConfigData->nFeature         = O22MAKELONG2 (arrbyData, 8);
    pPtConfigData->fOffset          = O22MAKEFLOAT2(arrbyData, 12);
    pPtConfigData->fGain            = O22MAKEFLOAT2(arrbyData, 16);
    pPtConfigData->fHiScale         = O22MAKEFLOAT2(arrbyData, 20);
    pPtConfigData->fLoScale         = O22MAKEFLOAT2(arrbyData, 24);
    // Bytes 28-31 are not used at this time
    pPtConfigData->fFilterWeight    = O22MAKEFLOAT2(arrbyData, 32);
    pPtConfigData->fWatchdogValue   = O22MAKEFLOAT2(arrbyData, 36);
    pPtConfigData->nWatchdogEnabled = O22MAKELONG2 (arrbyData, 40);

    memcpy(pPtConfigData->byName, arrbyData + 48, 16);
  }

  return nResult;
}


LONG O22SnapIoMemMap::GetPtConfigurationEx(long nPoint, SIOMM_PointConfigArea * pPtConfigData)
//-------------------------------------------------------------------------------------------------
// Get the configuration for a point
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[44]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_POINT_CONFIG_READ_MOD_TYPE_BASE + 
                      (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                      44, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pPtConfigData->nModuleType = O22MAKELONG2(arrbyData, 0);
    pPtConfigData->nPointType  = O22MAKELONG2(arrbyData, 4);

    pPtConfigData->nFeature         = O22MAKELONG2 (arrbyData, 8);
    pPtConfigData->fOffset          = O22MAKEFLOAT2(arrbyData, 12);
    pPtConfigData->fGain            = O22MAKEFLOAT2(arrbyData, 16);
    pPtConfigData->fHiScale         = O22MAKEFLOAT2(arrbyData, 20);
    pPtConfigData->fLoScale         = O22MAKEFLOAT2(arrbyData, 24);
    // Bytes 28-35 are not used at this time
    pPtConfigData->fWatchdogValue   = O22MAKEFLOAT2(arrbyData, 36);
    pPtConfigData->nWatchdogEnabled = O22MAKELONG2 (arrbyData, 40);
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetPtWatchdog(long nPoint, float fValue, long nEnabled)
//-------------------------------------------------------------------------------------------------
// Set the watchdog values for a point.
//-------------------------------------------------------------------------------------------------
{
  BYTE arrbyData[8]; // buffer for the data to be read

  // Build the data area
  O22FILL_ARRAY_FROM_FLOAT(arrbyData, 0, fValue);
  O22FILL_ARRAY_FROM_LONG (arrbyData, 4, nEnabled);

  // Write the data
  return WriteBlock(SIOMM_POINT_CONFIG_WRITE_WDOG_VALUE_BASE + (SIOMM_POINT_CONFIG_BOUNDARY * nPoint), 
                    8, (BYTE*)arrbyData);
}



LONG O22SnapIoMemMap::CalcSetAnaPtOffset(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Calculate and set an analog point's offset
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_CALC_SET_OFFSET_BASE + 
                   (SIOMM_APOINT_READ_CALC_SET_BOUNDARY * nPoint), 
                   pfValue);
}


LONG O22SnapIoMemMap::CalcSetAnaPtGain(long nPoint, float *pfValue)
//-------------------------------------------------------------------------------------------------
// Calculate and set an analog point's gain
//-------------------------------------------------------------------------------------------------
{
  return ReadFloat(SIOMM_APOINT_READ_CALC_SET_GAIN_BASE + 
                   (SIOMM_APOINT_READ_CALC_SET_BOUNDARY * nPoint), 
                   pfValue);
}


LONG O22SnapIoMemMap::GetStatusPUC(long *pnPUCFlag)
//-------------------------------------------------------------------------------------------------
// Get the Powerup Clear flag
//-------------------------------------------------------------------------------------------------
{
  return ReadQuad(SIOMM_STATUS_READ_PUC_FLAG, (DWORD*)pnPUCFlag);
}


LONG O22SnapIoMemMap::GetStatusLastError(long *pnErrorCode)
//-------------------------------------------------------------------------------------------------
// Get the last error
//-------------------------------------------------------------------------------------------------
{
  return ReadQuad(SIOMM_STATUS_READ_LAST_ERROR, (DWORD*)pnErrorCode);
}


LONG O22SnapIoMemMap::GetStatusBootpAlways(long *pnBootpAlways)
//-------------------------------------------------------------------------------------------------
// Get the "bootp always" flag
//-------------------------------------------------------------------------------------------------
{
  return ReadQuad(SIOMM_STATUS_READ_BOOTP_FLAG, (DWORD*)pnBootpAlways);
}


LONG O22SnapIoMemMap::GetStatusDegrees(long *pnDegrees)
//-------------------------------------------------------------------------------------------------
// Get the temperature unit.
//-------------------------------------------------------------------------------------------------
{
  return ReadQuad(SIOMM_STATUS_READ_DEGREES_FLAG, (DWORD*)pnDegrees);
}


LONG O22SnapIoMemMap::GetStatusWatchdogTime(long *pnTimeMS)
//-------------------------------------------------------------------------------------------------
// Get the watchdog time
//-------------------------------------------------------------------------------------------------
{
  return ReadQuad(SIOMM_STATUS_READ_WATCHDOG_TIME, (DWORD*)pnTimeMS);
}

LONG O22SnapIoMemMap::GetStatusVersionEx(SIOMM_StatusVersion *pVersionData)
//-------------------------------------------------------------------------------------------------
// Get version information
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[32]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_STATUS_READ_BASE, 32, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pVersionData->nMapVer        = O22MAKELONG2(arrbyData, 0 );
    pVersionData->nLoaderVersion = O22MAKELONG2(arrbyData, 24);
    pVersionData->nKernelVersion = O22MAKELONG2(arrbyData, 28);
  }

  return nResult;
}


LONG O22SnapIoMemMap::GetStatusHardwareEx(SIOMM_StatusHardware *pHardwareData)
//-------------------------------------------------------------------------------------------------
// Get hardware information
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[44]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_STATUS_READ_BASE, 44, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pHardwareData->nIoUnitType   = O22MAKELONG2(arrbyData, 32);
    pHardwareData->byHwdVerMonth = O22MAKEBYTE2(arrbyData, 36);
    pHardwareData->byHwdVerDay   = O22MAKEBYTE2(arrbyData, 37);
    pHardwareData->wHwdVerYear   = O22MAKEWORD2(arrbyData, 38);
    pHardwareData->nRamSize      = O22MAKELONG2(arrbyData, 40);
  }

  return nResult;
}

LONG O22SnapIoMemMap::GetStatusHardwareEx2(SIOMM_StatusHardware2 *pHardwareData)
//-------------------------------------------------------------------------------------------------
// Get hardware information
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[SIOMM_STATUS_READ_PART_NUMBER_SIZE]; // buffer for the data to be read
  SIOMM_StatusHardware HardwareData1;

  // Use the original function first
  nResult = GetStatusHardwareEx(&HardwareData1);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // Read the part number
    nResult = ReadBlock(SIOMM_STATUS_READ_PART_NUMBER, 
                        SIOMM_STATUS_READ_PART_NUMBER_SIZE, (BYTE*)arrbyData);

    // Check for error
    if (SIOMM_OK == nResult)
    {
      // Copy the part number string
      memcpy(pHardwareData->byPartNumber, arrbyData, SIOMM_STATUS_READ_PART_NUMBER_SIZE);

      // If everything is okay, go ahead and fill the structure
      pHardwareData->nIoUnitType   = HardwareData1.nIoUnitType;                                  
      pHardwareData->byHwdVerMonth = HardwareData1.byHwdVerMonth;
      pHardwareData->byHwdVerDay   = HardwareData1.byHwdVerDay;
      pHardwareData->wHwdVerYear   = HardwareData1.wHwdVerYear;
      pHardwareData->nRamSize      = HardwareData1.nRamSize;
    }                                   
  }

  return nResult;
}


LONG O22SnapIoMemMap::GetStatusNetworkEx(SIOMM_StatusNetwork *pNetworkData)
//-------------------------------------------------------------------------------------------------
// Get network information
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[64]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_STATUS_READ_BASE, 64, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pNetworkData->wMACAddress0  = O22MAKEWORD2(arrbyData, 46);
    pNetworkData->wMACAddress1  = O22MAKEWORD2(arrbyData, 48);
    pNetworkData->wMACAddress2  = O22MAKEWORD2(arrbyData, 50);
    pNetworkData->nTCPIPAddress = O22MAKELONG2(arrbyData, 52);
    pNetworkData->nSubnetMask   = O22MAKELONG2(arrbyData, 56);
    pNetworkData->nDefGateway   = O22MAKELONG2(arrbyData, 60);
  }

  return nResult;
}


LONG O22SnapIoMemMap::GetStatusNetworkEx2(SIOMM_StatusNetwork2 *pNetworkData)
//-------------------------------------------------------------------------------------------------
// Get network information
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[124]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_STATUS_READ_BASE, 124, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    pNetworkData->wMACAddress0      = O22MAKEWORD2(arrbyData, 46);
    pNetworkData->wMACAddress1      = O22MAKEWORD2(arrbyData, 48);
    pNetworkData->wMACAddress2      = O22MAKEWORD2(arrbyData, 50);
    pNetworkData->nTCPIPAddress     = O22MAKELONG2(arrbyData, 52);
    pNetworkData->nSubnetMask       = O22MAKELONG2(arrbyData, 56);
    pNetworkData->nDefGateway       = O22MAKELONG2(arrbyData, 60);
    pNetworkData->nTcpIpMinRtoMS    = O22MAKELONG2(arrbyData, 88);
    pNetworkData->nInitialRtoMS     = O22MAKELONG2(arrbyData, 100);
    pNetworkData->nTcpRetries       = O22MAKELONG2(arrbyData, 104);
    pNetworkData->nTcpIdleTimeout   = O22MAKELONG2(arrbyData, 108);
    pNetworkData->nEnetLateCol      = O22MAKELONG2(arrbyData, 112);
    pNetworkData->nEnetExcessiveCol = O22MAKELONG2(arrbyData, 116);
    pNetworkData->nEnetOtherErrors  = O22MAKELONG2(arrbyData, 120);
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetStatusOperation(long nOpCode)
//-------------------------------------------------------------------------------------------------
// Issue an operation code
//-------------------------------------------------------------------------------------------------
{
  return WriteQuad(SIOMM_STATUS_WRITE_OPERATION, nOpCode);
}


LONG O22SnapIoMemMap::SetStatusBootpRequest(long nFlag)
//-------------------------------------------------------------------------------------------------
// Set the "bootp always" flag
//-------------------------------------------------------------------------------------------------
{
  return WriteQuad(SIOMM_STATUS_WRITE_BOOTP, nFlag);
}


LONG O22SnapIoMemMap::SetStatusDegrees(long nDegFlag)
//-------------------------------------------------------------------------------------------------
// Set the temperature unit
//-------------------------------------------------------------------------------------------------
{
  return WriteQuad(SIOMM_STATUS_WRITE_TEMP_DEGREES, nDegFlag);
}


LONG O22SnapIoMemMap::SetStatusWatchdogTime(long nTimeMS)
//-------------------------------------------------------------------------------------------------
// Set the watchdog time
//-------------------------------------------------------------------------------------------------
{
  return WriteQuad(SIOMM_STATUS_WRITE_WATCHDOG_TIME, nTimeMS);
}


LONG O22SnapIoMemMap::GetDateTime(char * pchDateTime)
//-------------------------------------------------------------------------------------------------
// Get the brain's date and time
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;        // for checking the return values of functions
  BYTE arrbyData[SIOMM_DATE_AND_TIME_SIZE]; // buffer for the data to be read

  nResult = ReadBlock(SIOMM_DATE_AND_TIME_BASE, SIOMM_DATE_AND_TIME_SIZE, (BYTE*)arrbyData);

  // Check the result
  if (SIOMM_OK == nResult)
  {
    // Copy the date and time string.
    strncpy(pchDateTime, (char*)arrbyData, SIOMM_DATE_AND_TIME_SIZE);
  }

  return nResult;
}

LONG O22SnapIoMemMap::SetDateTime(char * pchDateTime)
//-------------------------------------------------------------------------------------------------
// Set the brain's date and time
//-------------------------------------------------------------------------------------------------
{
  return WriteBlock(SIOMM_DATE_AND_TIME_BASE, SIOMM_DATE_AND_TIME_SIZE, (BYTE*)pchDateTime);
}


LONG O22SnapIoMemMap::GetAnaBank(DWORD dwDestOffset, SIOMM_AnaBank * pBankData)
//-------------------------------------------------------------------------------------------------
// Get an analog bank section from a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;        // for checking the return values of functions
  BYTE arrbyData[SIOMM_ABANK_MAX_BYTES]; // buffer for the data to be read
  DWORD dwQuadlet;     // A temp for getting the read value

  // Read the data
  nResult = ReadBlock(dwDestOffset, SIOMM_ABANK_MAX_BYTES, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // Unpack the data packet
    for (int i = 0 ; i < SIOMM_ABANK_MAX_ELEMENTS ; i++)
    {
      dwQuadlet = O22MAKELONG2(arrbyData, i * sizeof(long));

      // Copy the data
      memcpy(&(pBankData->fValue[i]), &dwQuadlet, sizeof(float));
    }
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetAnaBank(DWORD dwDestOffset, SIOMM_AnaBank BankData)
//-------------------------------------------------------------------------------------------------
// Set an analog bank section to a location in the SNAP I/O memory map.
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;        // for checking the return values of functions
  BYTE arrbyData[SIOMM_ABANK_MAX_BYTES]; // buffer for the data to be read
  DWORD dwTemp;     // A temp for getting the read value

  // Pack the data packet
  for (int i = 0 ; i < SIOMM_ABANK_MAX_ELEMENTS ; i++)
  {
    // Copy the float into a DWORD for easy manipulation
    memcpy(&dwTemp, &(BankData.fValue[i]), sizeof(long));

    O22FILL_ARRAY_FROM_LONG(arrbyData, i * sizeof(float), dwTemp);
  }

  // Read the data
  nResult = WriteBlock(dwDestOffset, SIOMM_ABANK_MAX_BYTES, (BYTE*)&arrbyData);

  return nResult;
}



LONG O22SnapIoMemMap::GetAnaBankValuesEx(SIOMM_AnaBank * pBankData)
//-------------------------------------------------------------------------------------------------
// Get the analog point values in the analog bank
//-------------------------------------------------------------------------------------------------
{
  return GetAnaBank(SIOMM_ABANK_READ_POINT_VALUES, pBankData); 
}


LONG O22SnapIoMemMap::GetAnaBankCountsEx(SIOMM_AnaBank * pBankData)
//-------------------------------------------------------------------------------------------------
// Get the analog point raw counts in the analog bank
//-------------------------------------------------------------------------------------------------
{
  return GetAnaBank(SIOMM_ABANK_READ_POINT_COUNTS, pBankData); 
}


LONG O22SnapIoMemMap::GetAnaBankMinValuesEx(SIOMM_AnaBank * pBankData)
//-------------------------------------------------------------------------------------------------
// Get the analog point minimum values in the analog bank
//-------------------------------------------------------------------------------------------------
{
  return GetAnaBank(SIOMM_ABANK_READ_POINT_MIN_VALUES, pBankData); 
}

LONG O22SnapIoMemMap::GetAnaBankMaxValuesEx(SIOMM_AnaBank * pBankData)
//-------------------------------------------------------------------------------------------------
// Get the analog point maximum values in the analog bank
//-------------------------------------------------------------------------------------------------
{
  return GetAnaBank(SIOMM_ABANK_READ_POINT_MAX_VALUES, pBankData); 
}


LONG O22SnapIoMemMap::SetAnaBankValuesEx(SIOMM_AnaBank BankData)
//-------------------------------------------------------------------------------------------------
// Set the analog point values in the analog bank
//-------------------------------------------------------------------------------------------------
{
  return SetAnaBank(SIOMM_ABANK_WRITE_POINT_VALUES, BankData); 
}


LONG O22SnapIoMemMap::SetAnaBankCountsEx(SIOMM_AnaBank BankData)
//-------------------------------------------------------------------------------------------------
// Set the analog point raw counts in the analog bank
//-------------------------------------------------------------------------------------------------
{
  return SetAnaBank(SIOMM_ABANK_WRITE_POINT_COUNTS, BankData); 
}



LONG O22SnapIoMemMap::SetStreamConfiguration(long nOnFlag, long nIntervalMS, long nPort,
                                             long nIoMirroringEnabled, long nStartAddress, 
                                             long nDataSize)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  BYTE arrbyData[24]; // buffer for the data to be read

  // Build the data area

  O22FILL_ARRAY_FROM_LONG(arrbyData, 0,  nIoMirroringEnabled);
  O22FILL_ARRAY_FROM_LONG(arrbyData, 4,  nStartAddress);
  O22FILL_ARRAY_FROM_LONG(arrbyData, 8,  nDataSize);
  O22FILL_ARRAY_FROM_LONG(arrbyData, 12, nOnFlag);
  O22FILL_ARRAY_FROM_LONG(arrbyData, 16, nIntervalMS);
  O22FILL_ARRAY_FROM_LONG(arrbyData, 20, nPort);

  // Write the data
  return WriteBlock(SIOMM_STREAM_CONFIG_BASE, 24, (BYTE*)arrbyData);
}


LONG O22SnapIoMemMap::SetStreamTarget(long nTarget, char * pchIpAddressArg)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  long nIp1, nIp2, nIp3, nIp4;
  long nTemp;

  // Parse the IP address
  sscanf(pchIpAddressArg, "%d.%d.%d.%d", &nIp1, &nIp2, &nIp3, &nIp4);

  // Make an integer out of the IP address parts
  nTemp = O22MAKELONG(nIp1, nIp2, nIp3, nIp4);

  return WriteQuad(SIOMM_STREAM_TARGET_BASE + (SIOMM_STREAM_TARGET_BOUNDARY * (nTarget-1)), 
                   nTemp);
}

LONG O22SnapIoMemMap::GetStreamConfiguration(long * pnOnFlag, long * pnIntervalMS, long * pnPort,
                                             long * pnIoMirroringEnabled, long * pnStartAddress, 
                                             long * pnDataSize)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;       // for checking the return values of functions
  BYTE arrbyData[24]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_STREAM_CONFIG_BASE, 24, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // If everything is okay, go ahead and fill the structure
    *pnIoMirroringEnabled = O22MAKELONG2(arrbyData, 0);
    *pnStartAddress       = O22MAKELONG2(arrbyData, 4);
    *pnDataSize           = O22MAKELONG2(arrbyData, 8);
    *pnOnFlag             = O22MAKELONG2(arrbyData, 12);
    *pnIntervalMS         = O22MAKELONG2(arrbyData, 16);
    *pnPort               = O22MAKELONG2(arrbyData, 20);
  }

  return nResult;
}


LONG O22SnapIoMemMap::GetStreamTarget(long nTarget, long * pnIpAddressArg)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;       // for checking the return values of functions

  // Read the data
  nResult = ReadQuad(SIOMM_STREAM_TARGET_BASE + (SIOMM_STREAM_TARGET_BOUNDARY * (nTarget-1)), 
                     (DWORD*)pnIpAddressArg);

  return nResult;
}


LONG O22SnapIoMemMap::GetStreamReadAreaEx(SIOMM_StreamStandardBlock * pStreamData)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[SIOMM_STREAM_READ_AREA_SIZE]; // buffer for the data to be read

  // Read the data
  nResult = ReadBlock(SIOMM_STREAM_READ_AREA_BASE, SIOMM_STREAM_READ_AREA_SIZE, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // Flip-flop all the bytes
    int nTemp;
    for (int i = 0 ; i < SIOMM_STREAM_READ_AREA_SIZE ; i+=sizeof(long))
    {   
      nTemp = O22MAKELONG2(arrbyData, i);

      memcpy((void*)(arrbyData + i), &nTemp, sizeof(long));
    }

    // Update the stream block
    memcpy(&(((char*)pStreamData)[4]), arrbyData, SIOMM_STREAM_READ_AREA_SIZE);
  }

  return nResult;
}

LONG O22SnapIoMemMap::GetSerialModuleConfigurationEx(long nSerialPort, SIOMM_SerialModuleConfigArea * pConfigData)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;      // for checking the return values of functions
  BYTE arrbyData[SIOMM_SERIAL_MODULE_CONFIG_BOUNDARY]; // buffer for the data to be read
  DWORD dwEomData;

  // Read the data
  nResult = ReadBlock(SIOMM_SERIAL_MODULE_CONFIG_BASE + 
                      (SIOMM_SERIAL_MODULE_CONFIG_BOUNDARY * nSerialPort), 
                      SIOMM_SERIAL_MODULE_CONFIG_BOUNDARY, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    nResult = ReadQuad(SIOMM_SERIAL_MODULE_EOM_BASE + 
                       (SIOMM_SERIAL_MODULE_EOM_BOUNDARY * nSerialPort), 
                       &dwEomData);

    // Check for error
    if (SIOMM_OK == nResult)
    {
      pConfigData->nIpPort       = O22MAKELONG2(arrbyData, 0);
      pConfigData->nBaudRate     = O22MAKELONG2(arrbyData, 4);    
      pConfigData->byParity      = arrbyData[8];     
      pConfigData->byDataBits    = arrbyData[9];   
      pConfigData->byStopBits    = arrbyData[10];
      // Byte 11 is reserved
      pConfigData->byTestMessage = arrbyData[12];
      pConfigData->byEOM1        = O22BYTE0(dwEomData);
      pConfigData->byEOM2        = O22BYTE1(dwEomData);
      pConfigData->byEOM3        = O22BYTE2(dwEomData);
      pConfigData->byEOM4        = O22BYTE3(dwEomData);
    }
  }

  return nResult;
}

LONG O22SnapIoMemMap::SetSerialModuleConfigurationEx(long nSerialPort, SIOMM_SerialModuleConfigArea ConfigData)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  BYTE arrbyData[SIOMM_SERIAL_MODULE_CONFIG_BOUNDARY]; // buffer for the data to be read
  DWORD dwEomTemp; 
  LONG  nResult;

  O22FILL_ARRAY_FROM_LONG(arrbyData, 0, ConfigData.nBaudRate);
  arrbyData[4] = O22BYTE3(ConfigData.byParity);
  arrbyData[5] = O22BYTE3(ConfigData.byDataBits);
  arrbyData[6] = O22BYTE3(ConfigData.byStopBits);
  // bytes 7 is reserved
  arrbyData[8] = O22BYTE3(ConfigData.byTestMessage);

  dwEomTemp    = O22MAKELONG(ConfigData.byEOM1, ConfigData.byEOM2, ConfigData.byEOM3, ConfigData.byEOM4);

  // Write the first section
  nResult = WriteBlock(SIOMM_SERIAL_MODULE_CONFIG_BASE + sizeof(long) + 
                       (SIOMM_SERIAL_MODULE_CONFIG_BOUNDARY * nSerialPort),
                       9, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK != nResult)
    return nResult;

  // Write the second section (End-of-message characters)
  return WriteQuad(SIOMM_SERIAL_MODULE_EOM_BASE + (SIOMM_SERIAL_MODULE_EOM_BOUNDARY * nSerialPort), 
                   dwEomTemp);

}


LONG O22SnapIoMemMap::GetScratchPadBitArea(long *pnBits63to32, long *pnBits31to0)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  return GetBitmask64(SIOMM_SCRATCHPAD_BITS_BASE, pnBits63to32, pnBits31to0); 
}


LONG O22SnapIoMemMap::SetScratchPadBitArea(long nBits63to32, long nBits31to0)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  return SetBitmask64(SIOMM_SCRATCHPAD_BITS_BASE, nBits63to32, nBits31to0); 
}


LONG O22SnapIoMemMap::SetScratchPadBitAreaMask(long nOnMask63to32,  long nOnMaskPts31to0, 
                                               long nOffMask63to32, long nOffMaskPts31to0)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;       // for checking the return values of functions
  BYTE arrbyData[16]; // buffer for the data to be read

  // Unpack the bytes
  O22FILL_ARRAY_FROM_LONG(arrbyData, 0,  nOnMask63to32);
  O22FILL_ARRAY_FROM_LONG(arrbyData, 4,  nOnMaskPts31to0);
  O22FILL_ARRAY_FROM_LONG(arrbyData, 8,  nOffMask63to32);
  O22FILL_ARRAY_FROM_LONG(arrbyData, 12, nOffMaskPts31to0);

  // Write the data
  nResult = WriteBlock(SIOMM_SCRATCHPAD_BITS_ON_MASK_BASE, 16, (BYTE*)arrbyData);

  return nResult;
}


LONG O22SnapIoMemMap::GetScratchPadIntegerArea (long nStartIndex, long nLength, long * pnData)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;                                       // for checking the return values of functions
  BYTE arrbyData[SIOMM_SCRATCHPAD_INTEGER_MAX_BYTES]; // buffer for the data to be read

  // Check the length
  if (nLength > SIOMM_SCRATCHPAD_INTEGER_MAX_ELEMENTS) // 256 
    nLength = SIOMM_SCRATCHPAD_INTEGER_MAX_ELEMENTS;

  // Read the data
  nResult = ReadBlock(SIOMM_SCRATCHPAD_INTEGER_BASE + (nStartIndex * sizeof(long)), 
                      nLength * sizeof(long), (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // Unpack the data packet
    for (long i = 0 ; i < nLength ; i++)
    {
      pnData[i] = O22MAKELONG2(arrbyData, i * sizeof(long));
    }
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetScratchPadIntegerArea(long nStartIndex, long nLength, long * pnData)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  BYTE arrbyData[SIOMM_SCRATCHPAD_INTEGER_MAX_BYTES]; // buffer for the data to be read

  // Check the length
  if (nLength > SIOMM_SCRATCHPAD_INTEGER_MAX_ELEMENTS)
    nLength = SIOMM_SCRATCHPAD_INTEGER_MAX_ELEMENTS;

  // Pack the data packet
  for (int i = 0 ; i < nLength ; i++)
  {
    O22FILL_ARRAY_FROM_LONG(arrbyData, i * sizeof(long), pnData[i]);
  }

  // Read the data
  return WriteBlock(SIOMM_SCRATCHPAD_INTEGER_BASE + (nStartIndex * sizeof(long)), 
                    nLength * sizeof(long), (BYTE*)&arrbyData);
}


LONG O22SnapIoMemMap::GetScratchPadFloatArea (long nStartIndex, long nLength, float * pfData)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;                                       // for checking the return values of functions
  BYTE arrbyData[SIOMM_SCRATCHPAD_FLOAT_MAX_BYTES]; // buffer for the data to be read

  // Check the length
  if (nLength > SIOMM_SCRATCHPAD_FLOAT_MAX_ELEMENTS) // 256 
    nLength = SIOMM_SCRATCHPAD_FLOAT_MAX_ELEMENTS;

  // Read the data
  nResult = ReadBlock(SIOMM_SCRATCHPAD_FLOAT_BASE + (nStartIndex * sizeof(float)), 
                      nLength * sizeof(float), (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // Unpack the data packet
    for (long i = 0 ; i < nLength ; i++)
    {
      pfData[i] = O22MAKEFLOAT2(arrbyData, i * sizeof(float));
    }
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetScratchPadFloatArea(long nStartIndex, long nLength, float * pfData)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  BYTE arrbyData[SIOMM_SCRATCHPAD_FLOAT_MAX_BYTES]; // buffer for the data to be read

  // Check the length
  if (nLength > SIOMM_SCRATCHPAD_FLOAT_MAX_ELEMENTS)
    nLength = SIOMM_SCRATCHPAD_FLOAT_MAX_ELEMENTS;

  // Pack the data packet
  for (int i = 0 ; i < nLength ; i++)
  {
    O22FILL_ARRAY_FROM_FLOAT(arrbyData, i * sizeof(float), pfData[i]);
  }

  // Read the data
  return WriteBlock(SIOMM_SCRATCHPAD_FLOAT_BASE + (nStartIndex * sizeof(float)), 
                    nLength * sizeof(float), (BYTE*)&arrbyData);
}


LONG O22SnapIoMemMap::GetScratchPadStringArea(long nStartIndex, long nLength, 
                                              SIOMM_ScratchPadString * pStringData)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  LONG nResult;                                       // for checking the return values of functions
  BYTE arrbyData[SIOMM_SCRATCHPAD_STRING_MAX_BYTES];  // buffer for the data to be read

  // Check the length
  if (nLength > SIOMM_SCRATCHPAD_STRING_MAX_ELEMENTS) // 8
    nLength = SIOMM_SCRATCHPAD_STRING_MAX_ELEMENTS;

  // Read the data
  nResult = ReadBlock(SIOMM_SCRATCHPAD_STRING_BASE + (nStartIndex * SIOMM_SCRATCHPAD_STRING_BOUNDARY), 
                      nLength * SIOMM_SCRATCHPAD_STRING_BOUNDARY, (BYTE*)arrbyData);

  // Check for error
  if (SIOMM_OK == nResult)
  {
    // Unpack the data packet
    for (long i = 0 ; i < nLength ; i++)
    {
      pStringData[i].wLength  = O22MAKEWORD2(arrbyData, i * SIOMM_SCRATCHPAD_STRING_BOUNDARY);
      memcpy(pStringData[i].byString, 
             arrbyData + (i * SIOMM_SCRATCHPAD_STRING_BOUNDARY) + SIOMM_SCRATCHPAD_STRING_LENGTH_SIZE,
             SIOMM_SCRATCHPAD_STRING_DATA_SIZE);
    }
  }

  return nResult;
}


LONG O22SnapIoMemMap::SetScratchPadStringArea(long nStartIndex, long nLength, 
                                              SIOMM_ScratchPadString * pStringData)
//-------------------------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------------------------
{
  BYTE arrbyData[SIOMM_SCRATCHPAD_STRING_MAX_BYTES]; // buffer 

  // Check the length
  if (nLength > SIOMM_SCRATCHPAD_STRING_MAX_ELEMENTS)
    nLength = SIOMM_SCRATCHPAD_STRING_MAX_ELEMENTS;

  // Pack the data packet
  for (int i = 0 ; i < nLength ; i++)
  {
    O22FILL_ARRAY_FROM_SHORT(arrbyData, i * SIOMM_SCRATCHPAD_STRING_BOUNDARY, pStringData[i].wLength);
    memcpy(arrbyData + (i * SIOMM_SCRATCHPAD_STRING_BOUNDARY) + SIOMM_SCRATCHPAD_STRING_LENGTH_SIZE,
           pStringData[i].byString, 
           SIOMM_SCRATCHPAD_STRING_DATA_SIZE);
  }

  // Read the data
  return WriteBlock(SIOMM_SCRATCHPAD_STRING_BASE + (nStartIndex * SIOMM_SCRATCHPAD_STRING_BOUNDARY), 
                    nLength * SIOMM_SCRATCHPAD_STRING_BOUNDARY, (BYTE*)&arrbyData);
}
