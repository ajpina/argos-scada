//-----------------------------------------------------------------------------
//
// O22STRCT.h
// Copyright (c) 1999-2002 by Opto 22
//
// 
// This is a support file for "O22SIOMM.H" and its class, O22SnapIoMemMap.
// It is seperated from "O22SIOMM.H" because this file is also used in
// "OptoSnapIoMemMapX.idl" for the definition of the OptoSnapIoMemMapX ActiveX
// component.  It should not be modified without consideration to the ActiveX
// requirements of an OLE Automation interface.
//
// Most of the structures in this file map directly or closely to areas 
// of the I/O unit's memory map.
//-----------------------------------------------------------------------------


#ifndef __O22STRCT_H
#define __O22STRCT_H

  /////////////////////////////////////////////////////////////////////
  // Digital Bank read-only area
  //
  typedef struct SIOMM_DigBankReadArea
  { 
    long nStatePts63to32;
    long nStatePts31to0;
    long nOnLatchStatePts63to32;
    long nOnLatchStatePts31to0;
    long nOffLatchStatePts63to32;
    long nOffLatchStatePts31to0;
    long nActiveCountersPts63to32;
    long nActiveCountersPts31to0;
  } O22_SIOMM_DigBankReadArea; 

  /////////////////////////////////////////////////////////////////////
  // Digital Point read-only area.
  //
  typedef struct SIOMM_DigPointReadArea
  {
    long  nState;          // bool
    long  nOnLatch;        // bool
    long  nOffLatch;       // bool
    long  nCounterState;   // bool
    long  nCounts;         // unsigned int
  } O22_SIOMM_DigPointReadArea;


  /////////////////////////////////////////////////////////////////////
  // Analog Point read-only area.
  //
  typedef struct SIOMM_AnaPointReadArea
  {
    float fValue;
    float fCounts;
    float fMinValue;
    float fMaxValue;
  } O22_SIOMM_AnaPointReadArea;

  /////////////////////////////////////////////////////////////////////
  // Analog Bank area
  //
  typedef struct SIOMM_AnaBank
  {
    float fValue[64];
  } O22_SIOMM_AnaBank;

  /////////////////////////////////////////////////////////////////////
  // Point Configuration read/write area.
  //
  typedef struct SIOMM_PointConfigArea2
  {
    long  nModuleType;          // Read only.  Not used in SetPointConfiguration()
    long  nPointType;           // Read/Write
    long  nFeature;             // Read/Write.  Only used for digital points
    float fOffset;              // Read/Write.  Only used for analog points
    float fGain;                // Read/Write.  Only used for analog points
    float fHiScale;             // Read/Write.  Only used for analog points
    float fLoScale;             // Read/Write.  Only used for analog points
    float fFilterWeight;        // Read/Write.  Only used for analog points
    float fWatchdogValue;       // Read/Write
    long  nWatchdogEnabled;     // Read/Write
    unsigned char byName[16];   // Read/Write
  } O22_SIOMM_PointConfigArea2;


  typedef struct SIOMM_PointConfigArea
  {
    long  nModuleType;       // Read only.  Not used in SetPointConfiguration()
    long  nPointType;        // Read/Write
    long  nFeature;          // Read/Write.  Only used for digital points
    float fOffset;           // Read/Write.  Only used for analog points
    float fGain;             // Read/Write.  Only used for analog points
    float fHiScale;          // Read/Write.  Only used for analog points
    float fLoScale;          // Read/Write.  Only used for analog points
    float fWatchdogValue;    // Read/Write
    long  nWatchdogEnabled;  // Read/Write
  } O22_SIOMM_PointConfigArea;

  
  typedef struct SIOMM_StatusVersion
  {
    long          nMapVer;            //  Memory map version
    long          nLoaderVersion;     //  Loader version (1.2.3.4 format)
    long          nKernelVersion;     //  Kernel version (1.2.3.4 format)
  } O22_SIOMM_StatusVersion;


  typedef struct SIOMM_StatusHardware2
  {
    long          nIoUnitType;        //  I/O unit type
    unsigned char byHwdVerMonth;      //  hardware version (month)
    unsigned char byHwdVerDay;        //  hardware version (day)
    short         wHwdVerYear;        //  hardware version (4 digit year)
    long          nRamSize;           //  bytes of installed RAM
    unsigned char byPartNumber[32];   //  part number as a string
  } O22_SIOMM_StatusHardware2;

  typedef struct SIOMM_StatusHardware
  {
    long          nIoUnitType;        //  I/O unit type
    unsigned char byHwdVerMonth;      //  hardware version (month)
    unsigned char byHwdVerDay;        //  hardware version (day)
    short         wHwdVerYear;        //  hardware version (4 digit year)
    long          nRamSize;           //  bytes of installed RAM
  } O22_SIOMM_StatusHardware;


  typedef struct SIOMM_StatusNetwork2
  {
    short         wMACAddress0;       //  MAC address 
    short         wMACAddress1;       //  MAC address 
    short         wMACAddress2;       //  MAC address 
    long          nTCPIPAddress;      //  IP address
    long          nSubnetMask;        //  subnet mask
    long          nDefGateway;        //  default gateway
    long          nTcpIpMinRtoMS;     //  TCP/IP minimum Response Timeout (RTO) in milliseconds
    long          nInitialRtoMS;      //  initial RTO
    long          nTcpRetries;        //  number of TCP retries
    long          nTcpIdleTimeout;    //  TCP idle session timeout
    long          nEnetLateCol;       //  Ethernet late collisions
    long          nEnetExcessiveCol;  //  Ethernet excessive collisions
    long          nEnetOtherErrors;   //  Other Ethernet errors
  } O22_SIOMM_StatusNetwork2;

  typedef struct SIOMM_StatusNetwork
  {
    short         wMACAddress0;       //  MAC address 
    short         wMACAddress1;       //  MAC address 
    short         wMACAddress2;       //  MAC address 
    long          nTCPIPAddress;      //  IP address
    long          nSubnetMask;        //  subnet mask
    long          nDefGateway;        //  default gateway
  } O22_SIOMM_StatusNetwork;

  typedef struct SIOMM_SerialModuleConfigArea
  {
    long          nIpPort;            //  READ ONLY
    long          nBaudRate;          //  baud rate
    unsigned char byParity;           //  parity
    unsigned char byDataBits;         //  data bits (7 or 8)
    unsigned char byStopBits;         //  stop bits (1 or 2)
    unsigned char byTestMessage;      //  bool for sending a powerup test message
    unsigned char byEOM1;             //  first  end-of-message character
    unsigned char byEOM2;             //  second end-of-message character
    unsigned char byEOM3;             //  third  end-of-message character
    unsigned char byEOM4;             //  fourth end-of-message character
  } O22_SIOMM_SerialModuleConfigArea;


  typedef struct SIOMM_StreamCustomBlock
  // Be careful when making changes to this structure!
  {
    // The first three variables map directly to a custom UDP stream packet.
    long           nHeader;        // See below for info 
    long           nMemMapAddress; // Memory address of custom stream area.
    unsigned char  byData[2034];   // Max data size of 2034 came from JimFred 
                                   // on 02/07/2000

    long           nTCPIPAddress;  // The source IP address
  } O22_SIOMM_StreamCustomBlock;

  typedef struct SIOMM_StreamStandardBlock
  {
    long           nHeader; // Bits  0-15 DataLength
                            // Bits 16-17 Tag              
                            // Bits 18-23 Channel          
                            // Bits 24-27 TransactionCode  
                            // Bits 28-31 Synchronization Code  

    // The following items map directly to the standard stream data
    float          fAnalogValue[64];
    long           nDigPointFeature[64];
    long           nStatePts63to32;
    long           nStatePts31to0;
    long           nOnLatchStatePts63to32;
    long           nOnLatchStatePts31to0;
    long           nOffLatchStatePts63to32;
    long           nOffLatchStatePts31to0;
    long           nActiveCountersPts63to32;
    long           nActiveCountersPts31to0;
    unsigned char  byReserved[56]; // reserved for future use

    long           nTCPIPAddress; // The source IP address

    // Be careful when making changes to this structure!

/*
    short          wMonth;
    short          wDay;
    short          wYear;
    short          wHour;
    short          wMinute;
    short          wSecond;
    short          wMillisecond;
*/
  } O22_SIOMM_StreamStandardBlock;


  /////////////////////////////////////////////////////////////////////
  // Scratch Pad area
  //

  typedef struct SIOMM_ScratchPadIntegerBlock
  {
    long nValue[256];
  } O22_SIOMM_ScratchPadIntegerBlock;

  typedef struct SIOMM_ScratchPadFloatBlock
  {
    float fValue[256];
  } O22_SIOMM_ScratchPadFloatBlock;

  typedef struct SIOMM_ScratchPadString
  {
    short         wLength;
    unsigned char byString[128];
  } O22_SIOMM_ScratchPadString;

// The following structures are for the ActiveX component
#ifdef __midl
  typedef struct SIOMM_ScratchPadStringX
  {
    short         wLength;
    unsigned char byString[128];
    BSTR          bstrString;
  } O22_SIOMM_ScratchPadStringX;

  typedef struct SIOMM_ScratchPadStringBlock
  {
    O22_SIOMM_ScratchPadStringX String[8];
  } O22_SIOMM_ScratchPadStringBlock;
#endif


#endif // __O22STRCT_H
