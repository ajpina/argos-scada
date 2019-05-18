//-----------------------------------------------------------------------------
//
// O22SIOMM.h
// Copyright (c) 1999 - 2002 by Opto 22
//
// Header for the O22SnapIoMemMap C++ class.  
// 
// The O22SnapIoMemMap C++ class is used to communicate from a computer to an
// Opto 22 SNAP Ethernet I/O unit.
//
// The basic procedure for using this class is:
//
//   1. Create an instance of the O22SnapIoMemMap class
//   2. Call OpenEnet() or OpenEnet2() to start connecting to an I/O unit
//   3. Call IsOpenDone() to complete the connection  process
//   4. Call SetCommOptions() to set the desired timeout value
//   5. Call any of the memory map functions, such as ConfigurePt(),
//      GetAnaPtValue(), SetDigBankPointStates(), and ReadBlock(), to 
//      communicate with the I/O unit.  Repeat as neccessary.
//   6. Call Close() when done.
//
// While this code was developed on Microsoft Windows 32-bit operating 
// systems, it is intended to be as generic as possible.  For Windows specific
// code, search for "_WIN32" and "_WIN32_WCE".  For Linux specific code, search
// for "_LINUX".
//-----------------------------------------------------------------------------

#ifndef __O22SIOMM_H_
#define __O22SIOMM_H_


#ifndef __O22SIOUT_H
#include "O22SIOUT.h"
#endif


#ifndef __O22STRCT_H
#include "O22STRCT.h"
#endif


// Transaction code used by the I/O unit
#define SIOMM_TCODE_WRITE_QUAD_REQUEST   0
#define SIOMM_TCODE_WRITE_BLOCK_REQUEST  1
#define SIOMM_TCODE_WRITE_RESPONSE       2
#define SIOMM_TCODE_READ_QUAD_REQUEST    4
#define SIOMM_TCODE_READ_BLOCK_REQUEST   5
#define SIOMM_TCODE_READ_QUAD_RESPONSE   6
#define SIOMM_TCODE_READ_BLOCK_RESPONSE  7

// Sizes of the read/write requests and responses
#define SIOMM_SIZE_WRITE_QUAD_REQUEST    16   
#define SIOMM_SIZE_WRITE_BLOCK_REQUEST   16   
#define SIOMM_SIZE_WRITE_RESPONSE        12
#define SIOMM_SIZE_READ_QUAD_REQUEST     12
#define SIOMM_SIZE_READ_QUAD_RESPONSE    16
#define SIOMM_SIZE_READ_BLOCK_REQUEST    16
#define SIOMM_SIZE_READ_BLOCK_RESPONSE   16

// Response codes from the I/O unit
#define SIOMM_RESPONSE_CODE_ACK          0
#define SIOMM_RESPONSE_CODE_NAK          7

// Memory Map values

// Status read area of the memory map
#define SIOMM_STATUS_READ_BASE                 0xF0300000
#define SIOMM_STATUS_READ_PUC_FLAG             0xF0300004
#define SIOMM_STATUS_READ_LAST_ERROR           0xF030000C
#define SIOMM_STATUS_READ_BOOTP_FLAG           0xF0300048
#define SIOMM_STATUS_READ_DEGREES_FLAG         0xF030004C
#define SIOMM_STATUS_READ_WATCHDOG_TIME        0xF0300054
#define SIOMM_STATUS_READ_PART_NUMBER          0xF0300080
#define SIOMM_STATUS_READ_PART_NUMBER_SIZE     0x00000020

// Date and Time
#define SIOMM_DATE_AND_TIME_BASE               0xF0350000
#define SIOMM_DATE_AND_TIME_SIZE               0x00000017


// Serial Module
#define SIOMM_SERIAL_MODULE_CONFIG_BASE        0xF03A8000
#define SIOMM_SERIAL_MODULE_CONFIG_BOUNDARY    0x00000010
#define SIOMM_SERIAL_MODULE_EOM_BASE           0xF03A8200
#define SIOMM_SERIAL_MODULE_EOM_BOUNDARY       0x00000010

// Status write area of the memory map
#define SIOMM_STATUS_WRITE_OPERATION           0xF0380000
#define SIOMM_STATUS_WRITE_BOOTP               0xF0380004
#define SIOMM_STATUS_WRITE_TEMP_DEGREES        0xF0380008
#define SIOMM_STATUS_WRITE_WATCHDOG_TIME       0xF0380010

// Streaming configuration area of the memory map
#define SIOMM_STREAM_CONFIG_BASE               0xF03FFFC4
#define SIOMM_STREAM_CONFIG_ON_FLAG            0xF03FFFD0
#define SIOMM_STREAM_CONFIG_INTERVAL           0xF03FFFD4
#define SIOMM_STREAM_CONFIG_PORT               0xF03FFFD8

#define SIOMM_STREAM_TARGET_BOUNDARY           0x00000004
#define SIOMM_STREAM_TARGET_BASE               0xF03FFFE0

// Digital bank read area of the memory map
#define SIOMM_DBANK_READ_AREA_BASE             0xF0400000
#define SIOMM_DBANK_READ_POINT_STATES          0xF0400000
#define SIOMM_DBANK_READ_ON_LATCH_STATES       0xF0400008
#define SIOMM_DBANK_READ_OFF_LATCH_STATES      0xF0400010
#define SIOMM_DBANK_READ_ACTIVE_COUNTERS       0xF0400018
#define SIOMM_DBANK_READ_COUNTER_DATA_BASE     0xF0400100
#define SIOMM_DBANK_READ_COUNTER_DATA_BOUNDARY 0x00000004
#define SIOMM_DBANK_MAX_FEATURE_ELEMENTS       0x00000040

// Digital bank write area of the memory map
#define SIOMM_DBANK_WRITE_AREA_BASE            0xF0500000
#define SIOMM_DBANK_WRITE_TURN_ON_MASK         0xF0500000
#define SIOMM_DBANK_WRITE_TURN_OFF_MASK        0xF0500008
#define SIOMM_DBANK_WRITE_ACT_COUNTERS_MASK    0xF0500010
#define SIOMM_DBANK_WRITE_DEACT_COUNTERS_MASK  0xF0500018

// Analog bank read area of the memory map
#define SIOMM_ABANK_READ_AREA_BASE             0xF0600000
#define SIOMM_ABANK_READ_POINT_VALUES          0xF0600000
#define SIOMM_ABANK_READ_POINT_COUNTS          0xF0600100
#define SIOMM_ABANK_READ_POINT_MIN_VALUES      0xF0600200
#define SIOMM_ABANK_READ_POINT_MAX_VALUES      0xF0600300

// Analog bank write area of the memory map
#define SIOMM_ABANK_WRITE_AREA_BASE            0xF0700000
#define SIOMM_ABANK_WRITE_POINT_VALUES         0xF0700000
#define SIOMM_ABANK_WRITE_POINT_COUNTS         0xF0700100

#define SIOMM_ABANK_MAX_BYTES                  0x00000100
#define SIOMM_ABANK_MAX_ELEMENTS               0x00000040


// Digital point read area of the memory map
#define SIOMM_DPOINT_READ_BOUNDARY             0x00000040
#define SIOMM_DPOINT_READ_AREA_BASE            0xF0800000
#define SIOMM_DPOINT_READ_STATE                0xF0800000
#define SIOMM_DPOINT_READ_ONLATCH_STATE        0xF0800004
#define SIOMM_DPOINT_READ_OFFLATCH_STATE       0xF0800008
#define SIOMM_DPOINT_READ_ACTIVE_COUNTER       0xF080000C
#define SIOMM_DPOINT_READ_COUNTER_DATA         0xF0800010

// Digital point write area of the memory map
#define SIOMM_DPOINT_WRITE_BOUNDARY            0x00000040
#define SIOMM_DPOINT_WRITE_TURN_ON_BASE        0xF0900000
#define SIOMM_DPOINT_WRITE_TURN_OFF_BASE       0xF0900004
#define SIOMM_DPOINT_WRITE_ACTIVATE_COUNTER    0xF0900008
#define SIOMM_DPOINT_WRITE_DEACTIVATE_COUNTER  0xF090000C

// Analog point read area of the memory map
#define SIOMM_APOINT_READ_BOUNDARY             0x00000040
#define SIOMM_APOINT_READ_AREA_BASE            0xF0A00000
#define SIOMM_APOINT_READ_VALUE_BASE           0xF0A00000
#define SIOMM_APOINT_READ_COUNTS_BASE          0xF0A00004
#define SIOMM_APOINT_READ_MIN_VALUE_BASE       0xF0A00008
#define SIOMM_APOINT_READ_MAX_VALUE_BASE       0xF0A0000C
#define SIOMM_APOINT_READ_TPO_PERIOD_BASE      0xF0B0000C

// Analog point write area of the memory map
#define SIOMM_APOINT_WRITE_BOUNDARY            0x00000040
#define SIOMM_APOINT_WRITE_VALUE_BASE          0xF0B00000
#define SIOMM_APOINT_WRITE_COUNTS_BASE         0xF0B00004
#define SIOMM_APOINT_WRITE_TPO_PERIOD_BASE     0xF0B0000C

// Point configuration area of the memory map
#define SIOMM_POINT_CONFIG_BOUNDARY               0x00000040
#define SIOMM_POINT_CONFIG_READ_MOD_TYPE_BASE     0xF0C00000
#define SIOMM_POINT_CONFIG_WRITE_TYPE_BASE        0xF0C00004
#define SIOMM_POINT_CONFIG_WRITE_FEATURE_BASE     0xF0C00008
#define SIOMM_POINT_CONFIG_WRITE_OFFSET_BASE      0xF0C0000C
#define SIOMM_POINT_CONFIG_WRITE_GAIN_BASE        0xF0C00010
#define SIOMM_POINT_CONFIG_WRITE_HISCALE_BASE     0xF0C00014
#define SIOMM_POINT_CONFIG_WRITE_LOSCALE_BASE     0xF0C00018
#define SIOMM_POINT_CONFIG_WRITE_WDOG_VALUE_BASE  0xF0C00024
#define SIOMM_POINT_CONFIG_WRITE_WDOG_ENABLE_BASE 0xF0C00028

#define SIOMM_POINT_CONFIG_NAME_SIZE              0x00000010


// Digital point read and clear area of the memory map
#define SIOMM_DPOINT_READ_CLEAR_BOUNDARY       0x00000004
#define SIOMM_DPOINT_READ_CLEAR_COUNTS_BASE    0xF0F00000
#define SIOMM_DPOINT_READ_CLEAR_ON_LATCH_BASE  0xF0F00100
#define SIOMM_DPOINT_READ_CLEAR_OFF_LATCH_BASE 0xF0F00200

// Analog point read and clear area of the memory map
#define SIOMM_APOINT_READ_CLEAR_BOUNDARY       0x00000004
#define SIOMM_APOINT_READ_CLEAR_MIN_VALUE_BASE 0xF0F80000
#define SIOMM_APOINT_READ_CLEAR_MAX_VALUE_BASE 0xF0F80100

// Analog point calc and set area of the memory map
#define SIOMM_APOINT_READ_CALC_SET_BOUNDARY    0x00000004
#define SIOMM_APOINT_READ_CALC_SET_OFFSET_BASE 0xF0E00000
#define SIOMM_APOINT_READ_CALC_SET_GAIN_BASE   0xF0E00100

// Streaming read area of the memory map
#define SIOMM_STREAM_READ_AREA_BASE            0xF1000000
#define SIOMM_STREAM_READ_AREA_SIZE            0x00000220

// Scratch pad aread of the memory map
#define SIOMM_SCRATCHPAD_BITS_BASE             0xF0D80000
#define SIOMM_SCRATCHPAD_BITS_ON_MASK_BASE     0xF0D80400
#define SIOMM_SCRATCHPAD_BITS_OFF_MASK_BASE    0xF0D80408
#define SIOMM_SCRATCHPAD_INTEGER_BASE          0xF0D81000
#define SIOMM_SCRATCHPAD_INTEGER_MAX_BYTES     0x00000400
#define SIOMM_SCRATCHPAD_INTEGER_MAX_ELEMENTS  0x00000100
#define SIOMM_SCRATCHPAD_FLOAT_BASE            0xF0D82000
#define SIOMM_SCRATCHPAD_FLOAT_MAX_BYTES       0x00000400
#define SIOMM_SCRATCHPAD_FLOAT_MAX_ELEMENTS    0x00000100
#define SIOMM_SCRATCHPAD_STRING_BASE           0xF0D83000
#define SIOMM_SCRATCHPAD_STRING_LENGTH_BASE    0xF0D83000
#define SIOMM_SCRATCHPAD_STRING_LENGTH_SIZE    0x00000002
#define SIOMM_SCRATCHPAD_STRING_DATA_BASE      0xF0D83002
#define SIOMM_SCRATCHPAD_STRING_DATA_SIZE      0x00000080
#define SIOMM_SCRATCHPAD_STRING_BOUNDARY       0x00000082
#define SIOMM_SCRATCHPAD_STRING_MAX_BYTES      0x00000410
#define SIOMM_SCRATCHPAD_STRING_MAX_ELEMENTS   0x00000008


class O22SnapIoMemMap {

  public:
  // Public data

    // Public Construction/Destruction
    O22SnapIoMemMap();
    ~O22SnapIoMemMap();

  // Public Members

    // Connection functions
    LONG OpenEnet (char * pchIpAddressArg, long nPort, long nOpenTimeOutMS, long nAutoPUC);
    LONG OpenEnet2(char * pchIpAddressArg, long nPort, long nOpenTimeOutMS, long nAutoPUC, 
                   long nConnectionType);
    //---------------------------------------------------------------------------------------------
    //  Usage  : Starts the connection process to a SNAP Ethernet brain. 
    //           Use the IsOpenDone() method to check if the open connection is completed. 
    //           OpenEnet() creates a TCP connection while OpenEnet2() allows the connection
    //           type to be specified.
    //
    //  Input  : pchIpAddressArg - IP address of I/O unit in "X.X.X.X" form.
    //           nPort - The Ethernet port to connect to, such as 2001
    //           nOpenTimeOutMS - a timeout value for the open process. Used by IsOpenDone()
    //                            to determine if the open process has timed out.
    //           nAutoPUC - used to automatically read and clear the I/O unit's Powerup Clear flag.
    //           nConnectionType - SIOMM_TCP for TCP connection, SIOMM_UDP for UDP connection
    //  Output : none
    //  Returns: SIOMM_OK if everything is OK, an error otherwise.
    //---------------------------------------------------------------------------------------------


    LONG IsOpenDone();
    //---------------------------------------------------------------------------------------------
    //  Usage  : Called after OpenEnet() to determine if the connection process is completed yet.
    //           This should be called until it doesn't return SIOMM_ERROR_NOT_CONNECTED_YET.
    //           If SIOMM_OK is returned, the connection process is complete and there is a 
    //           connection to the I/O unit.
    //
    //           If the nAutoPUC flag in OpenEnet() or OpenEnet2() was set to TRUE, then this 
    //           function will attempt to read and clear the I/O unit's Powerup Clear (PUC) flag 
    //           after the connection has been made.
    //
    //  Input  : none
    //  Output : none
    //  Returns: SIOMM_ERROR_NOT_CONNECTED_YET if the connection process isn't completed yet.
    //           SIOMM_TIME_OUT if the connection process timed out
    //           SIOMM_OK if the connection process is completed
    //           Or possibly any other error
    //---------------------------------------------------------------------------------------------

    LONG Close();
    //---------------------------------------------------------------------------------------------
    //  Usage  : Close the connection to the I/O unit
    //  Input  : none
    //  Output : none
    //  Returns: SIOMM_OK if everything is OK, an error otherwise.
    //---------------------------------------------------------------------------------------------

    LONG SetCommOptions(LONG nTimeOutMS, LONG nReserved);
    //---------------------------------------------------------------------------------------------
    //  Usage  : Set communication options, such as the timeout period
    //  Input  : nTimeOutMS - The timeout period for normal communications. The connection process
    //           using OpenEnet() has a seperate timeout period
    //           nReserved   - not used at this time. Set to 0.
    //  Output : none
    //  Returns: SIOMM_OK if everything is OK, an error otherwise.
    //---------------------------------------------------------------------------------------------


    // The following functions are for building and unpacking read/write requests for the 
    // 1394-based protocol.
    LONG BuildWriteBlockRequest(BYTE  * pbyWriteBlockRequest,
                                BYTE    byTransactionLabel,
                                DWORD   dwDestinationOffset,
                                WORD    wDataLength,
                                BYTE  * pbyBlockData);

    LONG BuildWriteQuadletRequest(BYTE * pbyWriteQuadletRequest,
                                  BYTE  byTransactionLabel, 
                                  WORD  wSourceId,
                                  DWORD dwDestinationOffset,
                                  DWORD dwQuadletData);

    LONG UnpackWriteResponse(BYTE * pbyWriteQuadletResponse,
                             BYTE * pbyTransactionLabel, 
                             BYTE * pbyResponseCode);

    LONG BuildReadQuadletRequest(BYTE * pbyReadQuadletRequest,
                                 BYTE   byTransactionLabel, 
                                 DWORD  dwDestinationOffset);

    LONG UnpackReadQuadletResponse(BYTE  * pbyReadQuadletResponse,
                                   BYTE  * pbyTransactionLabel, 
                                   BYTE  * pbyResponseCode,
                                   DWORD * pdwQuadletData);

    LONG BuildReadBlockRequest(BYTE * pbyReadBlockRequest,
                               BYTE   byTransactionLabel, 
                               DWORD  dwDestinationOffset,
                               WORD   wDataLength);

    LONG UnpackReadBlockResponse(BYTE  * pbyReadBlockResponse,
                                 BYTE  * pbyTransactionLabel, 
                                 BYTE  * pbyResponseCode,
                                 WORD  * pwDataLength,
                                 BYTE  * pbyBlockData);

    // Functions for reading and writing quadlets (DWORDs) at an I/O unit memory map address.
    LONG ReadQuad(DWORD dwDestOffset, DWORD * pdwQuadlet);
    LONG WriteQuad(DWORD dwDestOffset, DWORD dwQuadlet);

    // Functions for reading and writing floats  at an I/O unit memory map address.
    LONG ReadFloat(DWORD dwDestOffset, float * pfValue);
    LONG WriteFloat(DWORD dwDestOffset, float fValue);

    // Functions for reading and writing blocks of bytes at an I/O unit memory map address.
    LONG ReadBlock(DWORD dwDestOffset, WORD wDataLength, BYTE * pbyData);
    LONG WriteBlock(DWORD dwDestOffset, WORD wDataLength, BYTE * pbyData);

    // Status read
    LONG GetStatusPUC(long *pnPUCFlag);
    LONG GetStatusLastError(long *pnErrorCode);
    LONG GetStatusBootpAlways(long *pnBootpAlways);
    LONG GetStatusDegrees(long *pnDegrees);
    LONG GetStatusWatchdogTime(long *pnTimeMS);
    LONG GetStatusVersionEx(SIOMM_StatusVersion *pVersionData);
    LONG GetStatusHardwareEx(SIOMM_StatusHardware *pHardwareData);
    LONG GetStatusHardwareEx2(SIOMM_StatusHardware2 *pHardwareData);
    LONG GetStatusNetworkEx(SIOMM_StatusNetwork *pNetworkData);
    LONG GetStatusNetworkEx2(SIOMM_StatusNetwork2 *pNetworkData);

    // Status write
    LONG SetStatusOperation(long nOpCode);
    LONG SetStatusBootpRequest(long nFlag);
    LONG SetStatusDegrees(long nDegFlag);
    LONG SetStatusWatchdogTime(long nTimeMS);

    // Date and time configuration
    LONG GetDateTime(char * pchDateTime);
    LONG SetDateTime(char * pchDateTime);

    // Serial Module Configuration
    LONG GetSerialModuleConfigurationEx(long nSerialPort, SIOMM_SerialModuleConfigArea * pConfigData);
    LONG SetSerialModuleConfigurationEx(long nSerialPort, SIOMM_SerialModuleConfigArea ConfigData);
    

    // Point configuration
    LONG ConfigurePoint(long nPoint, long nPointType);
    LONG GetModuleType(long nPoint, long * pnModuleType);
    LONG GetPtConfigurationEx2(long nPoint, SIOMM_PointConfigArea2 * pData);
    LONG GetPtConfigurationEx(long nPoint, SIOMM_PointConfigArea * pData);
    LONG SetPtConfigurationEx2(long nPoint, SIOMM_PointConfigArea2 Data);
    LONG SetPtConfigurationEx(long nPoint, SIOMM_PointConfigArea Data);
    LONG SetDigPtConfiguration(long nPoint, long nPointType, long nFeature);
    LONG SetAnaPtConfiguration(long nPoint, long nPointType, float fOffset, float fGain, 
                               float fHiScale, float fLoScale);
    LONG SetPtWatchdog(long nPoint, float fValue, long nEnabled);

    // Digital point read
    LONG GetDigPtState(long nPoint, long *pnState);
    LONG GetDigPtOnLatch(long nPoint, long *pnState);
    LONG GetDigPtOffLatch(long nPoint, long *pnState);
    LONG GetDigPtCounterState(long nPoint, long *pnState);
    LONG GetDigPtCounts(long nPoint, long *pnValue);
    LONG GetDigPtReadAreaEx(long nPoint, SIOMM_DigPointReadArea * pData);

    // Digital point write
    LONG SetDigPtState(long nPoint, long nState);
    LONG SetDigPtCounterState(long nPoint, long nState);

    // Digital point read and clear
    LONG ReadClearDigPtCounts(long nPoint, long * pnState);
    LONG ReadClearDigPtOnLatch(long nPoint, long * pnState);
    LONG ReadClearDigPtOffLatch(long nPoint, long * pnState);

    // Digital bank read
    LONG GetDigBankPointStates(long *pnPts63to32, long *pnPts31to0);
    LONG GetDigBankOnLatchStates(long *pnPts63to32, long *pnPts31to0);
    LONG GetDigBankOffLatchStates(long *pnPts63to32, long *pnPts31to0);
    LONG GetDigBankActCounterStates(long *pnPts63to32, long *pnPts31to0);
    LONG GetDigBankReadAreaEx(SIOMM_DigBankReadArea * pData);

    // Digital bank write
    LONG SetDigBankPointStates(long nPts63to32, long nPts31to0, long nMask63to32, long nMask31to0);
    LONG SetDigBankOnMask(long nPts63to32, long nPts31to0);
    LONG SetDigBankOffMask(long nPts63to32, long nPts31to0);
    LONG SetDigBankActCounterMask(long nPts63to32, long nPts31to0);
    LONG SetDigBankDeactCounterMask(long nPts63to32, long nPts31to0);

    // Analog point read
    LONG GetAnaPtValue(long nPoint, float *pfValue);
    LONG GetAnaPtCounts(long nPoint, float *pfValue);
    LONG GetAnaPtMinValue(long nPoint, float *pfValue);
    LONG GetAnaPtMaxValue(long nPoint, float *pfValue);
    LONG GetAnaPtTpoPeriod(long nPoint, float *pfValue);
    LONG GetAnaPtReadAreaEx(long nPoint, SIOMM_AnaPointReadArea * pData);

    // Analog point write
    LONG SetAnaPtValue(long nPoint, float fValue);
    LONG SetAnaPtCounts(long nPoint, float fValue);
    LONG SetAnaPtTpoPeriod(long nPoint, float fValue);

    // Analog point read and clear
    LONG ReadClearAnaPtMinValue(long nPoint, float *pfValue);
    LONG ReadClearAnaPtMaxValue(long nPoint, float *pfValue);

    // Analog bank read
    LONG GetAnaBankValuesEx(SIOMM_AnaBank * pBankData);
    LONG GetAnaBankCountsEx(SIOMM_AnaBank * pBankData);
    LONG GetAnaBankMinValuesEx(SIOMM_AnaBank * pBankData);
    LONG GetAnaBankMaxValuesEx(SIOMM_AnaBank * pBankData);

    // Analog bank write
    LONG SetAnaBankValuesEx(SIOMM_AnaBank BankData);
    LONG SetAnaBankCountsEx(SIOMM_AnaBank BankData);

    // Analog point calc and set
    LONG CalcSetAnaPtOffset(long nPoint, float *pfValue);
    LONG CalcSetAnaPtGain(long nPoint, float *pfValue);


    // Stream functions
    LONG GetStreamConfiguration(long * pnOnFlag, long * pnIntervalMS, long * pnPort,
                                long * pnIoMirroringEnabled, long * pnStartAddress, 
                                long * pnDataSize);
    LONG SetStreamConfiguration(long nOnFlag, long nIntervalMS, long nPort,
                                long nIoMirroringEnabled, long nStartAddress, 
                                long nDataSize);
    LONG GetStreamTarget(long nTarget, long * pnIpAddressArg);
    LONG SetStreamTarget(long nTarget, char * pchIpAddressArg);
    LONG GetStreamReadAreaEx (SIOMM_StreamStandardBlock *pStreamData);  

    // Scratch Pad area
    LONG GetScratchPadBitArea(long *pnBits63to32, long *pnBits31to0);
    LONG SetScratchPadBitArea(long   nBits63to32, long   nBits31to0);
    LONG SetScratchPadBitAreaMask(long nOnMask63to32,  long nOnMaskPts31to0, 
                                  long nOffMask63to32, long nOffMaskPts31to0);

    

    LONG GetScratchPadIntegerArea(long nStartIndex, long nLength, long * pnData);
    LONG SetScratchPadIntegerArea(long nStartIndex, long nLength, long * pnData);
    
    LONG GetScratchPadFloatArea  (long nStartIndex, long nLength, float * pfData);
    LONG SetScratchPadFloatArea  (long nStartIndex, long nLength, float * pfData);

    LONG GetScratchPadStringArea (long nStartIndex, long nLength, SIOMM_ScratchPadString * pStringData);
    LONG SetScratchPadStringArea (long nStartIndex, long nLength, SIOMM_ScratchPadString * pStringData);


  protected:
    // Protected data
    SOCKET m_Socket;
    sockaddr_in m_SocketAddress; // for connecting our socket
    long        m_nConnectionType; // TCP or UDP

    timeval m_tvTimeOut;      // Timeout structure for sockets
    LONG    m_nTimeOutMS;     // For holding the user's timeout
    DWORD   m_nOpenTimeOutMS; // For holding the open timeout
    DWORD   m_nOpenTime;      // For testing the open timeout
    LONG    m_nRetries;       // For holding the user's retries.
    
    LONG    m_nAutoPUCFlag;   // For holding the AutoPUC flag sent in OpenEnet()

    BYTE    m_byTransactionLabel; // The current transaction label

    // Protected Members

    // Open/Close sockets functions
    LONG OpenSockets(char * pchIpAddressArg, long nPort, long nOpenTimeOutMS);
    LONG CloseSockets();

    // Generic functions for getting/setting 64-bit bitmasks
    LONG GetBitmask64(DWORD dwDestOffset, long *pnPts63to32, long *pnPts31to0);
    LONG SetBitmask64(DWORD dwDestOffset, long nPts63to32, long nPts31to0);

    // Generic functions for getting/setting analog banks
    LONG GetAnaBank(DWORD dwDestOffset, SIOMM_AnaBank * pBankData);
    LONG SetAnaBank(DWORD dwDestOffset, SIOMM_AnaBank BankData);

    // Gets the next transaction label
    inline void UpdateTransactionLabel() {m_byTransactionLabel++; \
                                          if (m_byTransactionLabel>=64) m_byTransactionLabel=0;}

  private:
    // Private data

    // Private Members
};



#endif // __O22SIOMM_H_

