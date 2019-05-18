//-------------------------------------------------------------------------------------------------
//
// O22SIOST.h
// Copyright (c) 2000-2002 by Opto 22
//
// Header for the O22SnapIoStream C++ class.  
// 
// The O22SnapIoStream C++ class is used to listen to UDP stream packets
// from multiple Opto 22 SNAP Ethernet I/O units streaming to the same port on
// the computer running this code. 
//
// The basic procedure for using this class is:
//
//   1. Create an instance of the O22SnapIoStream class
//   2. Call SetCallbackFuntions() to initialize several callback functions
//   3. Call OpenStreaming() to initialize the stream type, length, and port
//   4. Call StartStreamListening() for each I/O unit.  A second thread will 
//      be created to listen for incoming UDP packets.
//   5. The Stream Event callback function will be called every time a stream
//      packet from a registered I/O unit is received. Every time this callback
//      is called, the function GetLastStreamStandardBlockEx() or 
//      GetLastStreamCustomBlockEx() should be called, depending on the type set
//      in step #3.
//   6. StopStreamListening() may be called at any time to stop listening
//      for a specified I/O unit.
//   7. StartStreamListening() may be called at any time to add I/O units.
//   8. When done, call CloseStreaming()
//
// This class will eat all UDP packets sent to the port specified in OpenStreaming().
// No other program on the same computer can listen to UDP packets on the same port
// while this code is running.
//
// While this code was developed on Microsoft Windows 32-bit operating 
// systems, it is intended to be as generic as possible.  For Windows specific
// code, search for "_WIN32" and "_WIN32_WCE".  For Linux specific code, search
// for "_LINUX".
//
//-------------------------------------------------------------------------------------------------

#ifndef __O22SIOST_H_
#define __O22SIOST_H_


#ifndef __O22SIOUT_H
#include "O22SIOUT.h"
#endif


#ifndef __O22STRCT_H
#include "O22STRCT.h"
#endif


// These type #defines are used in OpenStreaming()
#define SIOMM_STREAM_TYPE_STANDARD           1
#define SIOMM_STREAM_TYPE_CUSTOM             2
  
// These callback functions definitions are used in StartStreamListening()
typedef LONG (* STREAM_CALLBACK_PROC)(void * pUserParam);
typedef LONG (* STREAM_EVENT_CALLBACK_PROC)(LONG nTCPIPAddress, void * pUserParam, LONG nResult);


// The O22StreamItem is used to create a linked list of I/O units that 
// will be listened to.
struct O22StreamItem 
{
  DWORD           nIpAddress;           // IP address of brain
  DWORD           nTimeout;             // timeout duration
  DWORD           nLastPacketTickCount; // for tracking timeouts
  BOOL            bTimeoutSent;         // flag for if a timeout error was sent already
  O22StreamItem * pNext;                // next in the list
};


class O22SnapIoStream {

  public:
  // Public data

    // Public Construction/Destruction
    O22SnapIoStream();
    ~O22SnapIoStream();

  // Public Members

  // Stream functions

    LONG OpenStreaming(long nType, long nLength, long nPort);
    //---------------------------------------------------------------------------------------------
    //  Usage  : Opens and initializes the computer's port to start listening for stream packets.
    //
    //  Input  : nType -   SIOMM_STREAM_TYPE_STANDARD for standard streams
    //                     SIOMM_STREAM_TYPE_CUSTOM for custom streams.
    //                     The type is set in the I/O unit's stream configuration.
    //                     This must match it and all I/O unit's streaming to the same port
    //                       should be of the same type.
    //           nLength - nLength sets the length of custom stream packets. 
    //                     Ignored for standard streams. 
    //           nPort -   The port being streamed to.
    //  Output : none
    //  Returns: SIOMM_OK if everything is OK, an error otherwise.
    //---------------------------------------------------------------------------------------------


    LONG CloseStreaming();
    //---------------------------------------------------------------------------------------------
    //  Usage  : Stops all listening and closes the port.
    //  Input  : none
    //  Output : none
    //  Returns: SIOMM_OK if everything is OK, an error otherwise.
    //---------------------------------------------------------------------------------------------


    LONG SetCallbackFunctions(STREAM_CALLBACK_PROC pStartThreadCallbackFunc, 
                             void * pStartThreadParam,
                             STREAM_EVENT_CALLBACK_PROC pStreamEventCallbackFunc, 
                             void * pStreamEventParam,
                             STREAM_CALLBACK_PROC pStopThreadCallbackFunc,  
                             void * pStopThreadParam);
    //---------------------------------------------------------------------------------------------
    //  Usage  : Sets the three callback functions used by the stream listening thread.
    //  Input  : pStartThreadCallbackFunc - This callback function is called whenever the stream
    //                                      listening thread is started, which is when the list
    //                                      of stream items goes from zero items to one item.
    //                                      Can be NULL if callback function not needed
    //           pStartThreadParam -        This is passed to the pStartThreadCallbackFunc
    //                                      callback function. It is useful for passing a value  
    //                                      or a pointer to the callback function.
    //           pStreamEventCallbackFunc - This callback function if called whenever a stream
    //                                      packet from an I/O unit is received.
    //                                      Basically, it notifies the user's program that a 
    //                                      packet of interest has been received.
    //           pStreamEventParam -        This is passed to the pStartThreadCallbackFunc
    //                                      callback function. It is useful for passing a value 
    //                                      or a pointer to the callback function.
    //           pStopThreadCallbackFunc -  This callback function is called whenever the stream
    //                                      listening thread is stopped, which is when the list
    //                                      of stream items goes from one item to zero items.
    //                                      Can be NULL if callback function not needed
    //           pStopThreadParam -         This is passed to the pStartThreadCallbackFunc
    //                                      callback function. It is useful for passing a value 
    //                                      or a pointer to the callback function.
    //  Output : none
    //  Returns: SIOMM_OK
    //---------------------------------------------------------------------------------------------
    
    LONG StartStreamListening(char * pchIpAddressArg, long nTimeoutMS);
    //---------------------------------------------------------------------------------------------
    //  Usage  : Starts listening for packets from an I/O unit at the specified IP address. 
    //           More than one I/O unit may be listened to by multiple calls to this function.
    //  Input  : pchIpAddressArg - IP address of I/O unit in "X.X.X.X" form.
    //           nTimeoutMS - The timeout value is used to generate error events if the specified 
    //                        length of time has passed since a stream packet has been received 
    //                        from the specified I/O unit. Different I/O units can have 
    //                        different timeout values.
    //  Output : none
    //  Returns: SIOMM_OK if everything is OK, an error otherwise.
    //---------------------------------------------------------------------------------------------


    LONG StopStreamListening(char * pchIpAddressArg);
    //---------------------------------------------------------------------------------------------
    //  Usage  : Stops listening to the I/O unit at the specified address
    //  Input  : pchIpAddressArg - IP address of I/O unit in "X.X.X.X" form.
    //  Output : none
    //  Returns: SIOMM_OK if everything is OK, an error otherwise.
    //---------------------------------------------------------------------------------------------

    LONG GetLastStreamStandardBlockEx(SIOMM_StreamStandardBlock *pStreamData);
    //---------------------------------------------------------------------------------------------
    //  Usage  : Gets values for the last stream packet received. 
    //
    //           This function should be called immediately after receiving a packet event
    //           via the pStreamEventCallbackFunc callback function set in StartStreamListening().
    //           
    //           Should only be called if the standard type was set in the OpenStreaming() method.
    //  Input  : none
    //  Output : pStreamData - Structure to receive the data from the last stream packet received.
    //  Returns: SIOMM_OK if everything is OK, an error otherwise.
    //---------------------------------------------------------------------------------------------


    LONG GetLastStreamCustomBlockEx(SIOMM_StreamCustomBlock *pStreamData);
    //---------------------------------------------------------------------------------------------
    //  Usage  : Gets data for the last stream packet received. 
    //
    //           This function should be called immediately after receiving a packet event
    //           via the pStreamEventCallbackFunc callback function set in StartStreamListening().
    //           
    //           Should only be called if the custom type was set in the OpenStreaming() method.
    //  Input  : none
    //  Output : pStreamData - Structure to receive the data from the last stream packet received.
    //  Returns: SIOMM_OK if everything is OK, an error otherwise.
    //---------------------------------------------------------------------------------------------

    LONG StreamHandler();
    //---------------------------------------------------------------------------------------------
    // Usage  : This is the main worker function. It is called repeatedly by the listening thread, 
    //          StreamThread().  It checks the socket for new packets and determines if the user is
    //          interested in them.
    //          IMPORTANT: This function should NOT be called by the user program. It has public 
    //          access because it is called by StreamThead(), a worker thread.
    //---------------------------------------------------------------------------------------------

    LONG CheckStreamTimeouts();
    //---------------------------------------------------------------------------------------------
    // Usage  : Checks if any stream items have timed out. If one has timed out, the
    //          pStreamEventCallbackFunc callback function set in StartStreamListening() is called
    //          with the SIOMM_TIME_OUT value in the nResult parameter.
    //          IMPORTANT: This function should NOT be called by the user program. It has public 
    //          access because it is called by StreamThead(), a worker thread.
    //---------------------------------------------------------------------------------------------
    


    // The following members should be in the protected area. They're public so that
    // the StreamThread() function can get to them.

      BOOL m_bListenToStreaming; // A flag used in StreamThread() to know when to stop listening

      BYTE                       * m_pbyLastStreamBlock; // Byte array containing the last 
                                                         // block received

      SIOMM_StreamCustomBlock      m_LastStreamBlock;    // Structure containing the last stream
                                                         // block received. It can be converted to
                                                         // a SIOMM_StreamStandardBlock structure
                                                         // in GetLastStreamStandardBlockEx()

      // The following members are used to store the callback functions and user parameters
      // set in the SetCallbackFuntions() function.
      STREAM_CALLBACK_PROC         m_pStartThreadCallbackFunc; 
      STREAM_EVENT_CALLBACK_PROC   m_pStreamEventCallbackFunc;
      STREAM_CALLBACK_PROC         m_pStopThreadCallbackFunc;
      void                       * m_pStartThreadParam;
      void                       * m_pStreamEventParam;
      void                       * m_pStopThreadParam;


  protected:
    // Protected data

    // Member data for streaming
    SOCKET      m_StreamSocket;  // The handle to the UDP socket

    long        m_nStreamType;   // The type set in OpenStreaming()
    long        m_nStreamLength; // The length set in OpenStreaming()

#ifdef _WIN32
    DWORD            m_hStreamThread; // Handle to the stream listening thread
    CRITICAL_SECTION m_StreamCriticalSection;
#endif
#ifdef _LINUX
    pthread_t        m_hStreamThread; // Handle to the stream listening thread
    pthread_mutex_t  m_StreamCriticalSection;
#endif


    // This class keeps a linked list of I/O units to listen for. 
    // The O22StreamItem structure keeps information on each I/O unit and has a pNext
    // pointer to let use make the list structure. 
    O22StreamItem * pStreamList;      // The head of the list
    long            nStreamListCount; // The number of items in the list

    
    // Protected Members


  private:
    // Private data

    // Private Members
};


#endif // __O22SIOST_H_
