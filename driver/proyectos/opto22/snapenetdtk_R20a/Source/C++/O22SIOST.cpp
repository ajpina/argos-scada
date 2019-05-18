//-----------------------------------------------------------------------------
//
// O22SIOST.cpp
// Copyright (c) 2000-2002 by Opto 22
//
// Source for the O22SnapIoStream C++ class.  
// 
// The O22SnapIoStream C++ class is used to listen to UDP stream packets
// from an Opto 22 SNAP Ethernet I/O unit.
//
// See the header file for more information on using this class.
//
// While this code was developed on Microsoft Windows 32-bit operating 
// systems, it is intended to be as generic as possible.  For Windows specific
// code, search for "_WIN32" and "_WIN32_WCE".  For Linux specific code, search
// for "_LINUX".
//-----------------------------------------------------------------------------

#ifndef __O22SIOST_H_
#include "O22SIOST.h"
#endif


#ifdef _WIN32
#define WINSOCK_VERSION_REQUIRED_MAJ 2
#define WINSOCK_VERSION_REQUIRED_MIN 0
#endif


#ifdef _WIN32
void StreamThread(void * pParam);
#endif
#ifdef _LINUX
void * StreamThread(void * pParam);
#endif



O22SnapIoStream::O22SnapIoStream()
//-------------------------------------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------------------------------------
{
  // Set defaults
  nStreamListCount = 0;
  pStreamList = NULL;
  m_pbyLastStreamBlock = NULL;
  m_StreamSocket = INVALID_SOCKET;

  m_nStreamType = 0;   // The type set in OpenStreaming()
  m_nStreamLength = 0; // The length set in OpenStreaming()

  m_bListenToStreaming = FALSE; 

  memset(&m_LastStreamBlock, 0, sizeof(SIOMM_StreamCustomBlock));    // Structure containing the last stream


#ifdef _WIN32
  m_hStreamThread = NULL;

  InitializeCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
  memset(&m_hStreamThread, 0, sizeof(pthread_t)); // Handle to the stream listening thread

  pthread_mutex_init(&m_StreamCriticalSection, NULL);
#endif

}
 

O22SnapIoStream::~O22SnapIoStream()
//-------------------------------------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------------------------------------
{

#ifdef _WIN32
  WSACleanup();
#endif

#ifdef _WIN32
  DeleteCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
  pthread_mutex_destroy(&m_StreamCriticalSection);
#endif

}


LONG O22SnapIoStream::OpenStreaming(long nType, long nLength, long nPort)
//-------------------------------------------------------------------------------------------------
// Prepares to start listening for stream packets from SNAP Ethernet I/O units.
//
// nType can be either of the following values:
//   #define SIOMM_STREAM_TYPE_STANDARD           1
//   #define SIOMM_STREAM_TYPE_CUSTOM             2
//
// Standard means that default streaming is being sent from the I/O units.
// Custom means that a custom memory map area and length is being streamed from the I/O unit.
// This class expects for all I/O units to be streaming the same type and length.
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
  CloseStreaming();

  // Set the listen flag 
  m_bListenToStreaming = TRUE;

  // Check the stream type and make sure we know about it
  if (SIOMM_STREAM_TYPE_STANDARD == nType)
  {
    m_nStreamType   = nType; 
    m_nStreamLength = sizeof(SIOMM_StreamStandardBlock) - 4;
  }
  else if (SIOMM_STREAM_TYPE_CUSTOM == nType)
  {
    m_nStreamType   = nType; 
    m_nStreamLength = nLength + 8; // plus 4 for the header and 4 for the mem-map address
  }
  else
  {
    return SIOMM_ERROR_STREAM_TYPE_BAD;
  }
 
  // Allocate the response buffer
  m_pbyLastStreamBlock = new BYTE[m_nStreamLength];
  if (m_pbyLastStreamBlock == NULL)
    return SIOMM_ERROR_OUT_OF_MEMORY; // Couldn't allocate memory!

  // Create the UDP socket
  m_StreamSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (m_StreamSocket == INVALID_SOCKET)
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
  if (SOCKET_ERROR == ioctlsocket(m_StreamSocket, FIONBIO, &nNonBlocking))
#endif
#ifdef _LINUX
  if (-1 == fcntl(m_StreamSocket,F_SETFL,O_NONBLOCK))
#endif
  {
    CloseStreaming();

    return SIOMM_ERROR_CREATING_SOCKET;
  }

  // Setup the socket address structure
  sockaddr_in StreamSocketAddress;
  StreamSocketAddress.sin_addr.s_addr = INADDR_ANY;
  StreamSocketAddress.sin_family      = AF_INET;
  StreamSocketAddress.sin_port        = htons((WORD)nPort);

  // attempt connection
  nResult = bind(m_StreamSocket, (sockaddr*) &StreamSocketAddress, sizeof(StreamSocketAddress));

#ifdef _WIN32
  if (SOCKET_ERROR == nResult)
  {
    nResult = WSAGetLastError();  // for checking the specific error for debugging purposes
  }
#endif


  // Create the listener thread
#ifdef _WIN32
  m_hStreamThread = _beginthread(StreamThread, 0, this /*param*/);

  // Check for error
  if (-1 == m_hStreamThread)
  {
    m_hStreamThread = 0;
    return SIOMM_ERROR;
  }
#endif
#ifdef _LINUX
  if (0 != pthread_create(&m_hStreamThread, NULL, StreamThread, this))
  {
    m_hStreamThread = 0;
    return SIOMM_ERROR;
  }
#endif

  return SIOMM_OK;
}


LONG O22SnapIoStream::CloseStreaming()
//-------------------------------------------------------------------------------------------------
// Stops all listening and cleans everything up.
//-------------------------------------------------------------------------------------------------
{
  // Close up everything
  if (m_StreamSocket != INVALID_SOCKET)
  { 
#ifdef _WIN32
    closesocket(m_StreamSocket);
#endif
#ifdef _LINUX
    close(m_StreamSocket);
#endif
  }

#ifdef _WIN32
  EnterCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
  pthread_mutex_lock(&m_StreamCriticalSection);
#endif

  O22StreamItem * pTempStreamItem;
  O22StreamItem * pTempNextStreamItem;

  pTempNextStreamItem = pStreamList;

  while (pTempNextStreamItem != NULL)
  {

//    in_addr in;
//    in.S_un.S_addr = pTempNextStreamItem->nIpAddress;
//    StopStreamListening(inet_ntoa(in));

// I don't think this while loop is neccessary. The client will have
// to stop all of its listening before attemping to close.
// Or, this section needs more work. The VB client dies when 
// CloseStreaming() is called with open listeners.

    pTempStreamItem = pTempNextStreamItem;
    pTempNextStreamItem = pTempStreamItem->pNext;

    delete pTempStreamItem;

    nStreamListCount--;
  }

  pStreamList = NULL;

#ifdef _WIN32
  LeaveCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
  pthread_mutex_unlock(&m_StreamCriticalSection);
#endif

  // Clean up and reset the stream data block.
  if (m_pbyLastStreamBlock)
  {
    delete [] m_pbyLastStreamBlock;
    m_pbyLastStreamBlock = NULL;
  }

  // Stop listening for packets by setting this flag. It will cause the listening thread to stop.
  m_bListenToStreaming = FALSE;

  return SIOMM_OK;
}


#ifdef _WIN32
void StreamThread(void * pParam)
#endif
#ifdef _LINUX
void * StreamThread(void * pParam)
#endif
//-------------------------------------------------------------------------------------------------
// This is THE second thread that does the listening.
//-------------------------------------------------------------------------------------------------
{ 
  O22SnapIoStream * pThis = (O22SnapIoStream*) pParam;

  // Call the initilization function
  if (pThis->m_pStartThreadCallbackFunc)
    pThis->m_pStartThreadCallbackFunc(pThis->m_pStartThreadParam);

  // Enter a loop. We'll break out when done.
  while (1)
  {
    long nResult;

    // Call StreamHandler(), the main worker function of this thread
    nResult = pThis->StreamHandler();

    // Check for error. Timeout errors are okay in this instance.
    if (!((SIOMM_OK == nResult) || (SIOMM_TIME_OUT == nResult)))
    {     
      break;
    }

    // Check flag to see if we should keep looping
    if (!pThis->m_bListenToStreaming)
    {
      // Stop looping
      nResult = SIOMM_OK;
      break;
    }

    // Check the timeouts of all stream items
    pThis->CheckStreamTimeouts();

#ifdef _WIN32
    Sleep(1);
#endif
#ifdef _LINUX
    usleep(1000);
#endif
  }

  // Call the shutdown callback function
  if (pThis->m_pStopThreadCallbackFunc)
    pThis->m_pStopThreadCallbackFunc(pThis->m_pStopThreadParam);

  // End this thread
#ifdef _WIN32
  _endthread();
  return;
#endif
#ifdef _LINUX
  pthread_exit(NULL);
  return NULL;
#endif

}


LONG O22SnapIoStream::CheckStreamTimeouts()
//-------------------------------------------------------------------------------------------------
// Checks if any stream items have timed out
//-------------------------------------------------------------------------------------------------
{

  O22StreamItem * pTempStreamItem;
  pTempStreamItem = pStreamList;

  DWORD nTickCount;

#ifdef _WIN32
  EnterCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
  pthread_mutex_lock(&m_StreamCriticalSection);
#endif

  // Get the tick count
#ifdef _WIN32
  nTickCount = GetTickCount(); // GetTickCount returns the number of milliseconds that have
                               // elapsed since the system was started.
#endif
#ifdef _LINUX
  tms DummyTime;
  nTickCount = times(&DummyTime); // times() returns the number of milliseconds that have
                                  // elapsed since the system was started.
#endif

  long nTemp = 0; // for counting through the list of stream items.

  // Loop through the list of stream items
  while (pTempStreamItem != NULL)
  {
    // Increment the count
    nTemp++;

    // Double check we haven't gone off the end of the list
    if (nTemp > nStreamListCount)
    {
      break;
    }

    // No need to check if a timeout has already been sent.
    if (!pTempStreamItem->bTimeoutSent) 
    {
      // Do the timeout math!
      if ((nTickCount - pTempStreamItem->nLastPacketTickCount) > pTempStreamItem->nTimeout)
      {
        // Timeout!
        m_pStreamEventCallbackFunc(pTempStreamItem->nIpAddress, m_pStreamEventParam, SIOMM_TIME_OUT);

        // Set this flag so we don't check for another timeout with this stream item.
        // This will be reset by the next packet we receive for this item.
        pTempStreamItem->bTimeoutSent = TRUE;
      }
    }

    // Get the next item in the list
    pTempStreamItem = pTempStreamItem->pNext;
  }

#ifdef _WIN32
  LeaveCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
  pthread_mutex_unlock(&m_StreamCriticalSection);
#endif

  return SIOMM_OK;
}


LONG O22SnapIoStream::SetCallbackFunctions(STREAM_CALLBACK_PROC pStartThreadCallbackFunc, 
                                           void * pStartThreadParam,
                                           STREAM_EVENT_CALLBACK_PROC pStreamEventCallbackFunc, 
                                           void * pStreamEventParam,
                                           STREAM_CALLBACK_PROC pStopThreadCallbackFunc, 
                                           void * pStopThreadParam)
//-------------------------------------------------------------------------------------------------
// The pStartThreadCallbackFunc and pStopThreadCallbackFunc callback functions are used
// to help users of this class. They were mainly added to help the ActiveX wrapper. 
// Their use is optional. Set them to NULL if not needed. They are only called when the
// listener thread has started or stopped, which is done in the OpenStreaming() and 
// CloseStreaming() functions. 
//
// The pStreamEventCallbackFunc callback is used to inform the user of this class that
// a stream packet has arrived or that a timeout has occured.
//
// The pStartThreadParam, pStreamEventParam, and pStopThreadParam parameters are passed
// along with each corresponding callback function. They are useful for passing information
// to the callback, such as a pointer.
//-------------------------------------------------------------------------------------------------
{
  // Set all the callback functions and parameters
  m_pStartThreadCallbackFunc = pStartThreadCallbackFunc;
  m_pStreamEventCallbackFunc = pStreamEventCallbackFunc;
  m_pStopThreadCallbackFunc  = pStopThreadCallbackFunc;
  m_pStartThreadParam        = pStartThreadParam;
  m_pStreamEventParam        = pStreamEventParam;
  m_pStopThreadParam         = pStopThreadParam;

  return SIOMM_OK;
};

LONG O22SnapIoStream::StartStreamListening(char * pchIpAddressArg, long nTimeoutMS)
//-------------------------------------------------------------------------------------------------
// Start listening to a particular I/O unit. No connection is actually made. We just listen for 
// packets. We can listen to more than one at a time.
//
// The timeout value is used to generate an error event if a packet hasn't been received
// in the timeout period. 
//-------------------------------------------------------------------------------------------------
{
  // Create and add the new item
  O22StreamItem * pNewStreamItem = new O22StreamItem;
  if (pNewStreamItem)
  {
    // make sure the new object is all zeroes
    memset(pNewStreamItem, 0, sizeof(O22StreamItem));

    // Set the IP address and timeout
    pNewStreamItem->nTimeout = nTimeoutMS;
    pNewStreamItem->nIpAddress = inet_addr(pchIpAddressArg);

    // Get the current tick count
#ifdef _WIN32
    pNewStreamItem->nLastPacketTickCount = GetTickCount(); // GetTickCount returns the number of milliseconds that have
                                                           // elapsed since the system was started.
#endif
#ifdef _LINUX
    tms DummyTime;
    pNewStreamItem->nLastPacketTickCount = times(&DummyTime); // times() returns the number of milliseconds that have
                                                              // elapsed since the system was started.
#endif


    // A temp stream item for traversing the list
    O22StreamItem * pTempStreamItem;

#ifdef _WIN32
    EnterCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
    pthread_mutex_lock(&m_StreamCriticalSection);
#endif

    // Check if the list is empty
    if (pStreamList)
    {
      // The list is not empty, so add the new item only if doesn't already exist.
      pTempStreamItem = pStreamList;
      while (pTempStreamItem->pNext != NULL)
      {
        // Look for a match, in which case we won't need to add it.
        if (pTempStreamItem->nIpAddress == pNewStreamItem->nIpAddress)
        {
          delete pNewStreamItem;
          break;
        }

        // Get the next item
        pTempStreamItem = pTempStreamItem->pNext;
      }

      // If we made it this far, we're at the end of the list. 
      // Add the new item here.
      pTempStreamItem->pNext = pNewStreamItem;

      // Increment our count of stream items
      nStreamListCount++;
    }
    else
    {
      // The list is empty, so add the new item here
      pStreamList = pNewStreamItem;

      // Increment our count of stream items
      nStreamListCount++;

    }

#ifdef _WIN32
    LeaveCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
    pthread_mutex_unlock(&m_StreamCriticalSection);
#endif

  }
  else
  {
    return SIOMM_ERROR_OUT_OF_MEMORY;
  }
  
  // Set the listen flag. This may have been set already, but it guarantees
  // that we'll be listening now.
  m_bListenToStreaming = TRUE;

  // Everything must be okay.
  return SIOMM_OK;
}


LONG O22SnapIoStream::StopStreamListening(char * pchIpAddressArg)
//-------------------------------------------------------------------------------------------------
// Stop listening for packets from a certain IP address
//-------------------------------------------------------------------------------------------------
{
  // ?? The method of using a flag to stop the thread might cause one more stream packet
  // to be received and processed!

  // Get the IP address in integer form
  unsigned long nIpAddressArg = inet_addr(pchIpAddressArg);

#ifdef _WIN32
  EnterCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
  pthread_mutex_lock(&m_StreamCriticalSection);
#endif

  // Check that the stream list has something in it.
  if (pStreamList)
  {
    // A few stream items for traversing the stream list
    O22StreamItem * pTempStreamItem;
    O22StreamItem * pTempNextStreamItem;

    // Get the head of the list
    pTempStreamItem = pStreamList;

    // Check the IP address of the head item
    if (pStreamList->nIpAddress == nIpAddressArg)
    {
      // Remove the head item
      pTempStreamItem = pStreamList;
      pStreamList = pStreamList->pNext;

      // Delete the head item and decrement the count
      delete pTempStreamItem;
      nStreamListCount--;
    }
    else
    {
      // The head item didn't match, so start looping through the list
      pTempStreamItem = pStreamList;
      pTempNextStreamItem = pTempStreamItem->pNext;
      while (pTempNextStreamItem != NULL)
      {
        // Check the IP address
        if (pTempNextStreamItem->nIpAddress == nIpAddressArg)
        {
          // Found a match!

          // Remove this item from the list
          pTempStreamItem->pNext = pTempNextStreamItem->pNext;

          // Delete this item and decrement the list count
          delete pTempNextStreamItem;
          nStreamListCount--;

          // Break out of the loop
          break;
        }
        else
        {
          // Get the next item in the list
          pTempStreamItem = pTempNextStreamItem;
          pTempNextStreamItem = pTempNextStreamItem->pNext;
        }
      }
    }
  }

#ifdef _WIN32
  LeaveCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
  pthread_mutex_unlock(&m_StreamCriticalSection);
#endif

  return SIOMM_OK;
}


LONG O22SnapIoStream::StreamHandler()
//-------------------------------------------------------------------------------------------------
// This is the main worker function. It is called repeatedly by the listening thread, StreamThread().
// It checks the socket for new packets and determines if the user is interested in them.
//-------------------------------------------------------------------------------------------------
{
  LONG         nResult;
  fd_set       fds;
  sockaddr_in  SourceAddress;
  BOOL         bCriticalSectionFlag = TRUE; // used to tell if we need to leave the critical 
                                            // at the end of the function section
#ifdef _WIN32
  int       nSourceLen = sizeof(SourceAddress);
#endif
#ifdef _LINUX
  socklen_t nSourceLen = sizeof(SourceAddress);
#endif

  // Timeout structure for sockets, so to zeros.
  timeval tvStreamTimeOut;      
  tvStreamTimeOut.tv_sec  = 0;
  tvStreamTimeOut.tv_usec = 0;


  // Check that we have a valid socket
  if (INVALID_SOCKET == m_StreamSocket)
  {
    return SIOMM_ERROR_NOT_CONNECTED;
  }

  FD_ZERO(&fds);
  FD_SET(m_StreamSocket, &fds);


  // Is the recv ready?
  // Note that Param#1 of select() is ignored by Windows.  What should it be in other systems?
  if (0 == select(m_StreamSocket + 1, &fds, NULL, NULL, &tvStreamTimeOut)) 
  {
    // we timed-out, which doesn't really matter. The timeout was set to zero.
    // Basically, it means no packets are ready.
  
    // Return a timeout error. The caller will know what to do. This will be a normal condition.
    return SIOMM_TIME_OUT;
  }

  // The response is ready, so recv it.
  nResult = recvfrom(m_StreamSocket, (char*)m_pbyLastStreamBlock, 
                     m_nStreamLength, 0 /*??*/, (sockaddr*)&SourceAddress, &nSourceLen);

  // Check for socket error
  if (SOCKET_ERROR == nResult)
  {
#ifdef _WIN32
    nResult = WSAGetLastError();  // for checking the specific error
#endif

    return SIOMM_ERROR;
  }

  // Check the length
  if (m_nStreamLength != nResult)
  {
    return SIOMM_ERROR_RESPONSE_BAD;
  }

#ifdef _WIN32
  EnterCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
  pthread_mutex_lock(&m_StreamCriticalSection);
#endif

  // Check if this packet is from a source that we care about
  O22StreamItem * pTempStreamItem;
  pTempStreamItem = pStreamList;


  while (pTempStreamItem != NULL)
  {

    if (pTempStreamItem->nIpAddress == SourceAddress.sin_addr.s_addr )
    {
      // We have a match, so process that packet and inform the client.

      if (SIOMM_STREAM_TYPE_STANDARD == m_nStreamType)
      {
        // Copy the header
        memcpy(&(m_LastStreamBlock.nHeader), m_pbyLastStreamBlock, 4);

        // Copy the data block
        memcpy(&(m_LastStreamBlock.byData), m_pbyLastStreamBlock + 4, sizeof(SIOMM_StreamStandardBlock) - 8);
      }
      else if (SIOMM_STREAM_TYPE_CUSTOM == m_nStreamType)
      {
        // Copy the data block
        memcpy(&m_LastStreamBlock, m_pbyLastStreamBlock, m_nStreamLength);
      }

      // Set the source IP address
//      m_LastStreamBlock.nTCPIPAddress = O22_SWAP_BYTES_LONG(SourceAddress.sin_addr.s_addr);
      m_LastStreamBlock.nTCPIPAddress = O22MAKELONG(O22BYTE3(SourceAddress.sin_addr.s_addr), 
                                                    O22BYTE2(SourceAddress.sin_addr.s_addr), 
                                                    O22BYTE1(SourceAddress.sin_addr.s_addr), 
                                                    O22BYTE0(SourceAddress.sin_addr.s_addr));


      // Get the tickcount
#ifdef _WIN32
      // GetTickCount returns the number of milliseconds that have
      // elapsed since the system was started.
      pTempStreamItem->nLastPacketTickCount = GetTickCount(); 
#endif
#ifdef _LINUX
      tms DummyTime;

      // times() returns the number of milliseconds that have elapsed since the system was started
      pTempStreamItem->nLastPacketTickCount = times(&DummyTime); 
#endif

      // Reset the timeout sent flag.
      pTempStreamItem->bTimeoutSent = FALSE;

#ifdef _WIN32
      LeaveCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
      pthread_mutex_unlock(&m_StreamCriticalSection);
#endif

      // Call the callback function
      m_pStreamEventCallbackFunc(m_LastStreamBlock.nTCPIPAddress, m_pStreamEventParam, SIOMM_OK);

      bCriticalSectionFlag = FALSE;

      break;
    }

    // No match yet, so keep traversing the list
    pTempStreamItem = pTempStreamItem->pNext;
  }

  // Make sure we're out of the critical section
  if (bCriticalSectionFlag)
  {
#ifdef _WIN32
    LeaveCriticalSection(&m_StreamCriticalSection);
#endif
#ifdef _LINUX
    pthread_mutex_unlock(&m_StreamCriticalSection);
#endif
  }


  // Everything must be good if we got here
  return SIOMM_OK;
}


LONG O22SnapIoStream::GetLastStreamStandardBlockEx(SIOMM_StreamStandardBlock * pStreamData)
//-------------------------------------------------------------------------------------------------
// Gets the last stream block that was received. If this function isn't called after receiving
// a packet notification event (STREAM_EVENT_CALLBACK_PROC callback), the data will lost when
// the next packet is received.
//-------------------------------------------------------------------------------------------------
{
  int nTemp;

  // Flip-flop all the bytes so they're in the right order for the PC
  for (int i = 4 ; i < sizeof(SIOMM_StreamStandardBlock) ; i+=4)
  {   
    nTemp = O22MAKELONG(m_LastStreamBlock.byData[i-4], m_LastStreamBlock.byData[i-3],
                        m_LastStreamBlock.byData[i-2], m_LastStreamBlock.byData[i-1]);
    memcpy((void*)(((char*)pStreamData) + i), &nTemp, 4);
  }

  // Set the header
  pStreamData->nHeader = m_LastStreamBlock.nHeader;

  // Set the source IP address
  pStreamData->nTCPIPAddress =  m_LastStreamBlock.nTCPIPAddress;

  return SIOMM_OK;
}

LONG O22SnapIoStream::GetLastStreamCustomBlockEx(SIOMM_StreamCustomBlock * pStreamData)
//-------------------------------------------------------------------------------------------------
// Gets the last stream block that was received. If this function isn't called after receiving
// a packet notification event (STREAM_EVENT_CALLBACK_PROC callback), the data will lost when
// the next packet is received.
//-------------------------------------------------------------------------------------------------
{
  // Copy the data.
  memcpy(pStreamData, &m_LastStreamBlock, sizeof(m_LastStreamBlock));

  return SIOMM_OK;
}


