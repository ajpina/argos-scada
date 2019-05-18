//-----------------------------------------------------------------------------
//
// eiocon.cpp
// Opto 22 SNAP Ethernet I/O Driver Toolkit
//
// This example demonstrates use of the OptoSnapIoMemMap C++ class for 
// communicating to an Opto 22 SNAP Ethernet I/O unit.  It creates a small
// shell environment that lets a user issues commands to read and write to
// a SNAP Ethernet I/O unit.  The Brain object, an instance of the 
// OptoSnapIoMemMap C++ class, is used for connecting and communicating to 
// the I/O unit.
//-----------------------------------------------------------------------------

#include "stdio.h"
#include "ctype.h"
#include "../../../../Source/C++/O22SIOMM.h"

#define TRUE  1
#define FALSE 0

#ifdef _LINUX
#define stricmp strcasecmp
#endif

#define EIOCL_ERROR     0
#define EIOCL_MAIN_RESULT_SUCCESS  0
#define EIOCL_MAIN_RESULT_ERROR    1

void PrintHelp()
{  
  printf("  Opto 22 SNAP Ethernet I/O Command Line Tool B1.0c\n" 
         "\n" //   0     1       2    3            4        5   6    7        8
         "  usage: eiocl ip-addr port timeout(sec) addr/pt r/w type [length] [data]\n"
         "         eiocl ip-addr port timeout(sec) -f filename\n"
         "\n"
         "  Required Read/Write parameters for the data types:\n"
         "\n"
         "     Type        Description            Read Mode     Write Mode\n"
         "    ==========================================================================\n"
         "     d,x,f    Decimal, Hex, Float                     value\n"
         "       b      Binary                    length        length  data\n"
         "       s      String                    length        \"string\"\n"
         "      ip      IP Address                              IP-Address\n"
         "      dp      Digital Point                           state\n"
         "      ap      Analog  Point                           value\n"
         "\n"
         "  Examples:\n"
         "    eiocl 10.20.30.40 2001 10 F0B00040 w f  12.34\n"
         "    eiocl 10.20.30.40 2001 10 F03B0000 w b  3 4A 69 6D\n"
         "    eiocl 10.20.30.40 2001 10 F03B0000 w s  \"Opto\"\n"
         "    eiocl 10.20.30.40 2001 10 F03B0000 w ip 123.45.67.89\n"
         "    eiocl 10.20.30.40 2001 10 12       w dp 1\n"
         "    eiocl 10.20.30.40 2001 10 F0500008 r d\n"
         "    eiocl 10.20.30.40 2001 10 F03B0000 r b  3\n"
         "    eiocl 10.20.30.40 2001 10 -f test.txt\n");
}

void PrintError(int nResult)
{
  if (nResult == SIOMM_ERROR)
    printf("General error");
  else if (nResult == SIOMM_TIME_OUT) 
    printf("Timeout");
  else if (nResult == SIOMM_ERROR_NO_SOCKETS) 
    printf("No sockets");
  else if (nResult == SIOMM_ERROR_CREATING_SOCKET) 
    printf("Could not create socket to SNAP I/O");
  else if (nResult == SIOMM_ERROR_CONNECTING_SOCKET) 
    printf("Could not connect socket to SNAP I/O");
  else if (nResult == SIOMM_ERROR_RESPONSE_BAD) 
    printf("Bad response");
  else if (nResult == SIOMM_ERROR_NOT_CONNECTED_YET) 
    printf("Not connected yet");
  else if (nResult == SIOMM_ERROR_OUT_OF_MEMORY)  
    printf("Out of memory");
  else if (nResult == SIOMM_ERROR_NOT_CONNECTED)
    printf("Not connected to SNAP I/O");
  else if (nResult == SIOMM_BRAIN_ERROR_UNDEFINED_CMD) 
    printf("Undefined Command");
  else if (nResult == SIOMM_BRAIN_ERROR_INVALID_PT_TYPE) 
    printf("Invalid point type");
  else if (nResult == SIOMM_BRAIN_ERROR_INVALID_FLOAT) 
    printf("Invalid float");
  else if (nResult == SIOMM_BRAIN_ERROR_PUC_EXPECTED) 
    printf("Powerup Clear expected");
  else if (nResult == SIOMM_BRAIN_ERROR_INVALID_ADDRESS) 
    printf("Invalid memory address");
  else if (nResult == SIOMM_BRAIN_ERROR_INVALID_CMD_LENGTH) 
    printf("Invalid command length");
  else if (nResult == SIOMM_BRAIN_ERROR_RESERVED) 
    printf("Reserved");
  else if (nResult == SIOMM_BRAIN_ERROR_BUSY) 
    printf("Busy");
  else if (nResult == SIOMM_BRAIN_ERROR_CANT_ERASE_FLASH) 
    printf("Cannot erase flash");
  else if (nResult == SIOMM_BRAIN_ERROR_CANT_PROG_FLASH) 
    printf("Cannot program flash");
  else if (nResult == SIOMM_BRAIN_ERROR_IMAGE_TOO_SMALL) 
    printf("Downloaded imaged too small");
  else if (nResult == SIOMM_BRAIN_ERROR_IMAGE_CRC_MISMATCH) 
    printf("Image CRC mismatch");
  else if (nResult == SIOMM_BRAIN_ERROR_IMAGE_LEN_MISMATCH) 
    printf("Image length mismatch");
  else
    printf("Unknown error");

  printf("\n\n");
}


int ProcessCommand( char * pchCommand, O22SnapIoMemMap * pBrain, char * pchPreErrorMsg, 
                    BOOL bCalledFromCommandLine)
{
  long            nResult;
  int             nAddress;
  int             nPoint;
  int             nDataLength;
  BYTE *          pbyData;
  BOOL            bReadFlag;
  char *          pchMemAddress;
  char *          pchReadWriteType;
  char *          pchDataType;
  char *          pchDataLength;
  char *          pchDataByte;
  char *          pchString;

  nPoint = 0;

  pchMemAddress = strtok(pchCommand, " \n");
  if (!pchMemAddress)
  {
    printf( "%sNo memory address given.\n\n", pchPreErrorMsg);
    return EIOCL_ERROR;
  }

  pchReadWriteType = strtok(NULL, " \n");
  if (!pchReadWriteType)
  {
    printf("%sNo read/write type given.\n\n", pchPreErrorMsg);
    return EIOCL_ERROR;
  }

  pchDataType = strtok(NULL, " \n");
  if (!pchDataType)
  {
    printf("%sNo data type given.\n\n", pchPreErrorMsg);
    return EIOCL_ERROR;
  }


  // Check and process read/write flag
  if (!stricmp(pchReadWriteType, "R"))
  {
    bReadFlag = TRUE;
  }
  else if (!stricmp(pchReadWriteType, "W"))
  {
    bReadFlag = FALSE;
  }
  else
  {
    printf("%sUnknown read/write flag. Should be R or W.\n\n", pchPreErrorMsg);
    return EIOCL_ERROR;
  }

  // Get the memory address or point number
  if ((0 == stricmp(pchDataType, "dp")) || (0 == stricmp(pchDataType, "ap")))
  {
    // Get the address. Not much to check
    sscanf(pchMemAddress, "%d", &nPoint);
  }
  else
  {
    // Get the address. Not much to check
    sscanf(pchMemAddress, "%x", &nAddress);
  }


  // Check and process data types
  if (!stricmp(pchDataType, "B"))
  {
    // Get the next token, which should be the data length
    pchDataLength = strtok(NULL, " \n");
    if (!pchDataLength)
    {
      printf("%sNo data length given.\n\n", pchPreErrorMsg);
      return EIOCL_ERROR;
    }

    // Get and check the data length
    sscanf(pchDataLength, "%d", &nDataLength);
    if ((nDataLength < 1) || (nDataLength > 1000)) // 1000 is arbitrary
    {
      printf("%sInvalid data length. Should be between 1 and 1000.\n", pchPreErrorMsg);
      return EIOCL_ERROR;
    }

    if (bReadFlag)
    {
      pbyData = new BYTE[nDataLength];
    }
    else
    {

      // Copy the data into the byte array
      pbyData = new BYTE[nDataLength];
      for (int i = 0 ; i < nDataLength ; i++)
      {
        int nDataByte;

        // Get the next token
        pchDataByte = strtok(NULL, " \n");
        if (!pchDataByte)
        {
          printf("%sInvalid amount of binary data. Expecting %d items.\n\n", pchPreErrorMsg);
          return EIOCL_ERROR;
        }

        // Check the data
        if (0 == sscanf(pchDataByte, "%x", &nDataByte))
        {
          printf("%sInvalid binary data. '%s' is not a hexadecimal number.\n\n", pchPreErrorMsg);

          delete [] pbyData;
          return EIOCL_ERROR;
        }

        // Set this byte of data
        pbyData[i] = nDataByte;
      }
    }
  }
  else if (!stricmp(pchDataType, "D"))
  {
    nDataLength = 4;
    pbyData     = new BYTE[nDataLength];

    if (bReadFlag)
    {
      // nothing to do yet!
    }
    else
    {
      char * pchData;
      int nData;

      pchData = strtok(NULL, " \n");

      if (pchData)
      {
        sscanf(pchData, "%d", &nData);

        pbyData[0] = O22BYTE0(nData);
        pbyData[1] = O22BYTE1(nData);
        pbyData[2] = O22BYTE2(nData);
        pbyData[3] = O22BYTE3(nData);
      }
      else
      {
        printf("%sNo data given.\n\n", pchPreErrorMsg);
        delete [] pbyData;
        return EIOCL_ERROR;
      }
    }
  }
  else if (!stricmp(pchDataType, "DP"))
  {
    nDataLength = 4;
    pbyData     = new BYTE[nDataLength];

    // Check the point number
    if ((nPoint < 0) || (nPoint > 63))
    {
      printf("%sPoint number must be between 0 and 63.\n\n", pchPreErrorMsg);
      delete [] pbyData;
      return EIOCL_ERROR;
    }

    if (bReadFlag)
    {
      // Figure the memory map address from the digital point given
      nAddress = SIOMM_DPOINT_READ_STATE + (SIOMM_DPOINT_READ_BOUNDARY * nPoint);
    }
    else
    {
      char * pchData;
      int    nData = 0;

      pchData = strtok(NULL, " \n");

      if (pchData)
      {
        sscanf(pchData, "%d", &nData);

        // Figure the memory map address from the digital point given
        if (nData)
          nAddress = SIOMM_DPOINT_WRITE_TURN_ON_BASE  + (SIOMM_DPOINT_WRITE_BOUNDARY * nPoint);
        else
          nAddress = SIOMM_DPOINT_WRITE_TURN_OFF_BASE + (SIOMM_DPOINT_WRITE_BOUNDARY * nPoint);

        pbyData[0] = O22BYTE0(1);
        pbyData[1] = O22BYTE1(1);
        pbyData[2] = O22BYTE2(1);
        pbyData[3] = O22BYTE3(1);
      }
      else
      {
        printf("%sNo data given.\n\n", pchPreErrorMsg);
        delete [] pbyData;
        return EIOCL_ERROR;
      }
    }
  }
  else if (!stricmp(pchDataType, "AP"))
  {
    nDataLength = 4;
    pbyData     = new BYTE[nDataLength];

    if ((nPoint < 0) || (nPoint > 63))
    {
      printf("%sPoint number must be between 0 and 63.\n\n", pchPreErrorMsg);
      delete [] pbyData;
      return EIOCL_ERROR;
    }

    if (bReadFlag)
    {
      // Figure the memory map address from the digital point given
      nAddress = SIOMM_APOINT_READ_VALUE_BASE + (SIOMM_APOINT_READ_BOUNDARY * nPoint);
    }
    else
    {
      char * pchData;
      float  fData = 0.0;
      int    nTemp;

      nAddress = SIOMM_APOINT_WRITE_VALUE_BASE + (SIOMM_APOINT_WRITE_BOUNDARY * nPoint);

      pchData = strtok(NULL, " \n");

      if (pchData)
      {
        sscanf(pchData, "%f", &fData);

        memcpy(&nTemp, &fData, 4);

        pbyData[0] = O22BYTE0(nTemp);
        pbyData[1] = O22BYTE1(nTemp);
        pbyData[2] = O22BYTE2(nTemp);
        pbyData[3] = O22BYTE3(nTemp);
      }
      else
      {
        printf("%sNo data given.\n\n", pchPreErrorMsg);
        delete [] pbyData;
        return EIOCL_ERROR;
      }
    }
  }
  else if (!stricmp(pchDataType, "X"))
  {
    nDataLength = 4;
    pbyData     = new BYTE[nDataLength];

    if (bReadFlag)
    {
      // nothing to do yet!
    }
    else
    {
      char * pchData;
      int nData;

      pchData = strtok(NULL, " \n");

      if (pchData)
      {
        sscanf(pchData, "%x", &nData);

        pbyData[0] = O22BYTE0(nData);
        pbyData[1] = O22BYTE1(nData);
        pbyData[2] = O22BYTE2(nData);
        pbyData[3] = O22BYTE3(nData);
      }
      else
      {
        printf("%sNo data given.\n\n", pchPreErrorMsg);
        delete [] pbyData;
        return EIOCL_ERROR;
      }
    }
  }
  else if (!stricmp(pchDataType, "IP"))
  {
    nDataLength = 4;
    pbyData     = new BYTE[nDataLength];

    if (bReadFlag)
    {
      // nothing to do yet!
    }
    else
    {
      char * pchIpAddress;
      int nIpAddress;

      pchIpAddress = strtok(NULL, " \n");

      if (pchIpAddress)
      {
        nIpAddress = inet_addr(pchIpAddress);

        if (INADDR_NONE == nIpAddress)
        {
          printf("%sInvalid IP address. Should be in #.#.#.# form.\n\n", pchPreErrorMsg);
          delete [] pbyData;
          return EIOCL_ERROR;
        }

        // nIpAddress is already in the correct byte order so just copy
        // it into the data byte array.
        pbyData[0] = O22BYTE3(nIpAddress);
        pbyData[1] = O22BYTE2(nIpAddress);
        pbyData[2] = O22BYTE1(nIpAddress);
        pbyData[3] = O22BYTE0(nIpAddress);
      }
      else
      {
        printf("%sNo IP address given.\n\n", pchPreErrorMsg);
        return EIOCL_ERROR;
      }
    }
  }
  else if (!stricmp(pchDataType, "F"))
  {
    float fData;
    int   nTemp;

    nDataLength = 4;
    pbyData     = new BYTE[nDataLength];

    if (bReadFlag)
    {
      // nothing to do yet!
    }
    else
    {
      char * pchData;

      pchData = strtok(NULL, " \n");

      if (pchData)
      {
        sscanf(pchData, "%f", &fData);

        memcpy(&nTemp, &fData, 4);

        pbyData[0] = O22BYTE0(nTemp);
        pbyData[1] = O22BYTE1(nTemp);
        pbyData[2] = O22BYTE2(nTemp);
        pbyData[3] = O22BYTE3(nTemp);
      }
      else
      {
        printf("%sNo data given.\n\n", pchPreErrorMsg);
        delete [] pbyData;
        return EIOCL_ERROR;
      }
    }
  }
  else if (0 == stricmp(pchDataType, "S"))
  {
    if (bReadFlag)
    {
      pchDataLength = strtok(NULL, " \n");
      if (!pchDataLength)
      {
        printf("%sNo string length given.\n\n", pchPreErrorMsg);
        return EIOCL_ERROR;
      }

      // Get and check the data length
      sscanf(pchDataLength, "%d", &nDataLength);
      if ((nDataLength < 1) || (nDataLength > 1000)) // 1000 is arbitrary
      {
        printf("%sInvalid string length. Should be between 1 and 1000.\n\n", pchPreErrorMsg);
        return EIOCL_ERROR;
      }

      pbyData = new BYTE[nDataLength+1];
    }
    else
    {
      // Read 'til the next "
      pchString = strtok(NULL, "\n");

      if (!pchString)
      {
        printf("%sNo string given.\n\n", pchPreErrorMsg);
        return EIOCL_ERROR;
      }

      char * pchStringTemp;
      pchStringTemp = new char[strlen(pchString) + 1]; // make a new string(might be too long; oh well)
      pchStringTemp[0] = 0;

      // The strings may in a different depending if it is from a file or the command line.
      // Commands from the command line have any \" turned into just "
      if (bCalledFromCommandLine)
      {
        strcpy(pchStringTemp, pchString);
      }
      else
      {
        // Convert any \" into "
        for (unsigned int i = 0 ; i < strlen(pchString) ; i++)
        {
          if ((pchString[i] == '\\') && (pchString[i+1]))
          {
            strncat(pchStringTemp, "\"", 1);
            i++;
          }
          else
          {
            strncat(pchStringTemp, pchString+i, 1);
          }
        }

        // Trim the right hand side of whitespace
        while (isspace(pchStringTemp[strlen(pchStringTemp)-1]))
        {
          pchStringTemp[strlen(pchStringTemp)-1] = 0;
        }

      }
      
      // Remove the quotes around the string
      pchStringTemp++;
      pchStringTemp[strlen(pchStringTemp)-1] = 0;

      nDataLength = strlen(pchStringTemp) + 1;

      // Copy the data into the byte array
      pbyData = new BYTE[nDataLength];

      strcpy((char*)pbyData, pchStringTemp);

      // Reset the temp pointer and clean up
      pchStringTemp--;
      delete [] pchStringTemp;
    }
  }
  else
  {
    printf("%sUnknown data type. Should be B, D, X, F, S, or IP.\n\n", pchPreErrorMsg);
    return EIOCL_ERROR;
  }

  if (bReadFlag)
  {
    // NULL out the data byte array
    memset(pbyData, 0, nDataLength);

    nResult = pBrain->ReadBlock(nAddress, nDataLength, pbyData);

    // Check the result
    if (nResult == SIOMM_OK)
    {
      if (!stricmp(pchDataType, "B"))
      {
        printf("  Binary data at %s is ", pchMemAddress);
        for (int i = 0 ; i < nDataLength ; i++)
        {
          printf("%02X ", pbyData[i]);
        }
        printf("\n\n");
      }
      else if (!stricmp(pchDataType, "D"))
      {
        int   nValue;

        nValue = O22MAKELONG(pbyData[0], pbyData[1], pbyData[2], pbyData[3]);

        printf("  Integer value at %s is %d\n\n", pchMemAddress, nValue);
      }
      else if (!stricmp(pchDataType, "X"))
      {
        int nValue;

        nValue = O22MAKELONG(pbyData[0], pbyData[1], pbyData[2], pbyData[3]);

        printf("  Integer value at %s is 0x%08X\n\n", pchMemAddress, nValue);
      }
      else if (!stricmp(pchDataType, "DP"))
      {
        int   nValue;

        nValue = O22MAKELONG(pbyData[0], pbyData[1], pbyData[2], pbyData[3]);

        printf("  Digital point #%d is %s\n\n", nPoint, nValue ? "ON" : "OFF");
      }
      else if (!stricmp(pchDataType, "AP"))
      {
        float fValue;
        int   nTemp;

        nTemp = O22MAKELONG(pbyData[0], pbyData[1], pbyData[2], pbyData[3]);

        memcpy(&fValue, &nTemp, 4);

        printf("  Analog point #%d is %f\n\n", nPoint, fValue);
      }
      else if (!stricmp(pchDataType, "IP"))
      {
        printf("  IP address at %s is %d.%d.%d.%d\n\n", pchMemAddress, 
               pbyData[0], pbyData[1], pbyData[2], pbyData[3]);
      }
      else if (!stricmp(pchDataType, "F"))
      {
        float fValue;
        int   nTemp;

        nTemp = O22MAKELONG(pbyData[0], pbyData[1], pbyData[2], pbyData[3]);

        memcpy(&fValue, &nTemp, 4);

        printf("  Float value at %s is %f\n\n", pchMemAddress, fValue);
      }
      else if (!stricmp(pchDataType, "S"))
      {
        pbyData[nDataLength] = 0;
        printf("  String value at %s is \"%s\"\n\n", pchMemAddress, pbyData);
      }
    }

  }
  else
  {
    nResult = pBrain->WriteBlock(nAddress, nDataLength, pbyData);
  }

  delete [] pbyData;

  if (nResult != SIOMM_OK)
  {
    printf("%s", pchPreErrorMsg);
    PrintError(nResult);
  }

  return nResult;
}


int main( int argc, char *argv[])
{
  O22SnapIoMemMap Brain;
  long            nResult;
  int             nPort;
  int             nTimeout;

  printf("\n");

  // Make sure there's at least five command-line parameters.  
  if (argc < 6)
  {  
    PrintHelp();
    return EIOCL_MAIN_RESULT_ERROR;
  }

  // Check the IP address
  if (inet_addr(argv[1]) == INADDR_NONE)
  {
    printf("  Invalid IP address. Should be in #.#.#.# form.\n"
           "  Example: 10.20.30.40\n");
    return EIOCL_MAIN_RESULT_ERROR;
  }

  // Get the port number and check it
  sscanf(argv[2], "%d", &nPort);
  if ((nPort < 1) || (nPort > 65535))
  {
    printf("  Invalid IP port. Should be between 1 and 65535.\n"
           "  Example: 2001\n");
    return EIOCL_MAIN_RESULT_ERROR;
  }


  // Get the timeout value and check it
  sscanf(argv[3], "%d", &nTimeout);
  if ((nTimeout < 1) || (nTimeout > 86400))
  {
    printf("  Invalid timeout value. Should be between 1 and 86400 seconds.\n"
           "  Example: 10\n");
    return EIOCL_MAIN_RESULT_ERROR;
  }


  // Set the send/recv timeout value
  Brain.SetCommOptions(nTimeout * 1000, 0);

  // Open a connection. Use the same timeout for the open.
  nResult = Brain.OpenEnet(argv[1], nPort, nTimeout * 1000, 1);

  // Print a status message
//  printf("\n  Attemping to connect to Ethernet I/O unit...");

  // Check the result of the open function
  if (SIOMM_OK == nResult)
  {
    // Keep calling IsOpenDone() until we connect or error
    nResult = Brain.IsOpenDone();
    while (SIOMM_ERROR_NOT_CONNECTED_YET == nResult)
    {
      // Try again
      nResult = Brain.IsOpenDone();
    }

  }

  // Check for error on OpenEnet() and IsOpenDone()
  if (SIOMM_OK != nResult)
  {   
    Brain.Close();

    printf("  ERROR: ");
    PrintError(nResult);
    return EIOCL_MAIN_RESULT_ERROR;
  }

  // Check if a file is being specified or not
  if (0 == stricmp(argv[4], "-f"))
  {
    FILE * pCommandFile;

    // Try to open the given file
    pCommandFile = fopen(argv[5], "rt");

    // Check the file
    if (pCommandFile)
    {
      char   pchLineCommandTemp[1000]; // for reading in each line from the file
      char * pchLineCommand;           // for parsing through each line
      BOOL   bContinue = TRUE;
      int    nLine = 1;

      // Get the first line
      pchLineCommand = fgets(pchLineCommandTemp, 1000, pCommandFile);

      while (pchLineCommand && bContinue)
      {
        // Trim the left hand side of whitespace
        while (isspace(pchLineCommand[0]))
        {
          pchLineCommand++;
        }

        // Filter out empty lines and comments
        if ((strlen(pchLineCommand) > 1) &&
            (!((pchLineCommand[0] == '/') && (pchLineCommand[1] == '/'))))
        {         
          char pchPreErrorMsg[50];
          sprintf(pchPreErrorMsg, "  ERROR on line #%d: ", nLine);

          // Pass this line on.
          nResult = ProcessCommand(pchLineCommand, &Brain, pchPreErrorMsg, FALSE);

          if (nResult != SIOMM_OK)
          {
            bContinue = FALSE; // this will get us out of the loop
          }

          // Sleep for a moment so the brain won't be flooded
#ifdef _WIN32
          Sleep(100000); // this delay is arbitrary
#endif
#ifdef _LINUX
          usleep(100); // this delay is arbitrary
#endif
        }

        nLine++;

        // Get the next line
        pchLineCommand = fgets(pchLineCommand, 1000, pCommandFile);
      }

      fclose(pCommandFile);

    }
    else
    {
      // Unable to open the file.
      printf("  Unable to open file \"%s\".\n", argv[5]);
      nResult = EIOCL_ERROR;
    }
  }
  else
  {

    char * pchCommand;
    pchCommand = new char[1000]; // what should this be?
    pchCommand[0] = 0;

    for (int i = 4 ; i < argc ; i++)
    {
      if ((7 == i) && (0 == stricmp(argv[6], "S")) && (0 == stricmp(argv[5], "W")))
      {
        strcat(pchCommand, "\"");
        strcat(pchCommand, argv[i]);
        strcat(pchCommand, "\"");
      }
      else
      {
        strcat(pchCommand, argv[i]);
        strcat(pchCommand, " ");
      }
    }

    strcat(pchCommand, "\n");

    nResult = ProcessCommand(pchCommand, &Brain, "  ERROR: ", TRUE);
  }

  // Close the connection to the brain
  Brain.Close();

  if (nResult == SIOMM_OK)
    return EIOCL_MAIN_RESULT_SUCCESS;
  else
    return EIOCL_MAIN_RESULT_ERROR;
}


