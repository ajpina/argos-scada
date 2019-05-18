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
#include "../../../../Source/C++/O22SIOMM.h"
#include "../../../../Source/C++/O22SIOST.h"

#define TRUE  1
#define FALSE 0

void PrintHelp()
{
  printf("\n" \
         "  Opto 22 SNAP Ethernet I/O Driver Toolkit Console Test Application\n"
         "\n" \
         "  Commands:\n" \
         "    rdb                  - read the digital bank\n" \
         "    rab                  - read the analog bank\n" \
         "    rdp <point>          - read the given digital point\n" \
         "    rap <point>          - read the given analog point\n" \
         "    wdp <point> <value>  - write the value to the given digital point\n" \
         "    wap <point> <value>  - write the value to the given analog point\n" \
         "    ls                   - listen to the stream area\n" \
         "    help                 - print help\n" \
         "    quit                 - exit this program\n" \
         "\n"
        );
}

LONG StreamCallback(long nIPAddress, void * pParam, long nStatus)
{
  long nResult;

  // Check the status of this event
  if (nStatus == SIOMM_OK)
  {
    SIOMM_StreamStandardBlock StreamBlock;

    // Get the latest stream packet
    nResult = ((O22SnapIoStream*)pParam)->GetLastStreamStandardBlockEx(&StreamBlock);

    // Print out the results
    if ((nResult == SIOMM_OK) && (StreamBlock.nTCPIPAddress == nIPAddress))
    {
      printf("\nSTREAMING DATA - press the Enter key to stop listening to stream data.\n");
      printf("\n  DIGITAL BANK VALUES\n");
      printf("    Point States:     0x%08X%08X\n", 
             StreamBlock.nStatePts63to32, StreamBlock.nStatePts31to0);
      printf("    On-Latch States:  0x%08X%08X\n", 
             StreamBlock.nOnLatchStatePts63to32, StreamBlock.nOnLatchStatePts31to0);
      printf("    Off-Latch States: 0x%08X%08X\n", 
             StreamBlock.nOffLatchStatePts63to32, StreamBlock.nOffLatchStatePts31to0);

      printf("\n  ANALOG VALUES\n  ");
      for (int i = 0 ; i < 64 ; i++)
      {
        printf("  %2d: %12.5f", i, StreamBlock.fAnalogValue[i]);
        if ((i+1)%4 == 0)
          printf("\n  ");
      }
    }
    else
    {
      printf("IP addresses don't match: %X and %X", StreamBlock.nTCPIPAddress, nIPAddress);
    }
  }
  else if (nStatus == SIOMM_TIME_OUT)
  {
    // Print out an error
    printf("\nStream timeout - press the Enter key to stop listening to stream data.\n"
           "Make sure the brain is set to stream.\n");
  }

  return SIOMM_OK;
}


int main( int argc, char *argv[])
{
  O22SnapIoMemMap        Brain;
  long                   nResult;
  long                   bIncompleteCommand;
  long                   nPoint;
  long                   nValue;
  float                  fValue;
  char                   pchCommand[100];
  char                   pchCommandType[100];
  SIOMM_DigPointReadArea DigPtData;
  SIOMM_AnaPointReadArea AnaPtData;
  SIOMM_DigBankReadArea  DigBankData;
  SIOMM_AnaBank          AnaBankData;
 
  // Make sure there's at least one command-line parameter.  
  if (argc < 2)
  {
    printf("\n" \
           "Opto 22 SNAP Ethernet I/O Driver Toolkit Console Test Application\n\n" \
           "usage: eiocon ip-address\n" \
           "Example: dbankcon.exe 10.192.54.0\n\n");
    return 0;
  }

  // Assume that the command-line parameter is a valid IP address.
  // Use port 2001 and a timeout of 10 seconds.
  nResult = Brain.OpenEnet2(argv[1], 2001, 10000, 1, SIOMM_UDP);

  // Print a status message
  printf("\nAttemping to connect to Ethernet I/O unit...");

  // Check the result of the open function
  if (SIOMM_OK == nResult)
  {
    // Keep calling IsOpenDone() until we connect or error
    nResult = Brain.IsOpenDone();
    while (SIOMM_ERROR_NOT_CONNECTED_YET == nResult)
    {
      // Print a little update
      printf(".");

      // Try again
      nResult = Brain.IsOpenDone();
    }

    // Check the final result from IsOpenDone() 
    if (SIOMM_OK == nResult)
    {
      // The open is done and good
      printf("done.\n");
    }
  }

  // Check for error on OpenEnet() and IsOpenDone()
  if (SIOMM_OK != nResult)
  {   
    printf("Error while connecting to Ethernet I/O brain.\n");
    return 0;
  }


  // Enter a loop for command processing
  while (1)
  {
    // printf the eiocon command prompt
    printf("\neiocon> ");

    // Set the result to OK
    nResult = SIOMM_OK;

    bIncompleteCommand = FALSE;

    // Get an input line from the console
    fgets(pchCommand, 100, stdin);

    // Get the command type
    if (1 == sscanf(pchCommand, "%s", pchCommandType))
    {
      if ((strcmp(pchCommandType, "help") == 0) ||
          (strcmp(pchCommandType, "h")    == 0) ||
          (strcmp(pchCommandType, "?")    == 0))
      {
        PrintHelp();
      }
      else if ((strcmp(pchCommandType, "quit") == 0) ||
               (strcmp(pchCommandType, "exit") == 0))
      {
        break;
      }
      else if (strcmp(pchCommandType, "rdp") == 0)
      {
        if (2 == sscanf(pchCommand, "%s %d", pchCommandType, &nPoint))
        {
          // Read the digital point state
          nResult = Brain.GetDigPtReadAreaEx(nPoint, &DigPtData);

          // Check the result
          if (SIOMM_OK == nResult)
          {
            // If everything is okay, then print the state of the digital point.
            printf("\n  DIGITAL POINT %d\n", nPoint);
            printf("    State:         %s\n", DigPtData.nState ? "ON" : "OFF");
            printf("    On-Latch:      %s\n", DigPtData.nOnLatch ? "ON" : "OFF");
            printf("    Off-Latch:     %s\n", DigPtData.nOffLatch ? "ON" : "OFF");
            printf("    Counter State: %s\n", DigPtData.nCounterState ? "ACTIVE" : "DEACTIVE");
            printf("    Counters:      %d\n", DigPtData.nCounts);
          }
        }
        else
        {
          bIncompleteCommand = TRUE;
        }
      }
      else if (strcmp(pchCommandType, "rap") == 0)
      {
        if (2 == sscanf(pchCommand, "%s %d", pchCommandType, &nPoint))
        {
          // Read the analog point value
          nResult = Brain.GetAnaPtReadAreaEx(nPoint, &AnaPtData);

          // Check the result
          if (SIOMM_OK == nResult)
          {
            // If everything is okay, then print the value of the analog point.
            printf("\n  ANALOG POINT %d\n", nPoint);
            printf("    Value:      %f\n", AnaPtData.fValue);
            printf("    Raw Counts: %f\n", AnaPtData.fCounts);
            printf("    Min Value:  %f\n", AnaPtData.fMinValue);
            printf("    Max Value:  %f\n", AnaPtData.fMaxValue);
          }
        }
        else
        {
          bIncompleteCommand = TRUE;
        }
      }
      else if (strcmp(pchCommandType, "wdp") == 0)
      {
        if (3 == sscanf(pchCommand, "%s %d %d", pchCommandType, &nPoint, &nValue))
        {
          // Write to the digital point's state
          nResult = Brain.SetDigPtState(nPoint, nValue);

          // Check the result
          if (SIOMM_OK == nResult)
          {
            // If everything is okay, then print the state of the digital point.
            printf("\n  Digital point %d has been set to %s\n", nPoint, nValue ? "ON" : "OFF");
          }
        }
        else
        {
          bIncompleteCommand = TRUE;
        }
      }
      else if (strcmp(pchCommandType, "wap") == 0)
      {
        if (3 == sscanf(pchCommand, "%s %d %f", pchCommandType, &nPoint, &fValue))
        {
          // Write to the analog point's value
          nResult = Brain.SetAnaPtValue(nPoint, fValue);

          // Check the result
          if (SIOMM_OK == nResult)
          {
            // If everything is okay, then print the value of the analog point
            printf("\n  Analog point %d has been set to %f\n", nPoint, fValue);
          }
        }
        else
        {
          bIncompleteCommand = TRUE;
        }
      }
      else if (strcmp(pchCommandType, "rdb") == 0)
      {
        // Read the digital bank area
        nResult = Brain.GetDigBankReadAreaEx(&DigBankData);

        // Check the result
        if (SIOMM_OK == nResult)
        {
          // If everything is okay, then print the state of the digital bank read area.
          printf("\n  DIGITAL BANK READ AREA\n");
          printf("    Point States:     0x%08X%08X\n", DigBankData.nStatePts63to32, DigBankData.nStatePts31to0);
          printf("    On-Latch States:  0x%08X%08X\n", DigBankData.nOnLatchStatePts63to32, DigBankData.nOnLatchStatePts31to0);
          printf("    Off-Latch States: 0x%08X%08X\n", DigBankData.nOffLatchStatePts63to32, DigBankData.nOffLatchStatePts31to0);
        }
      }     
      else if (strcmp(pchCommandType, "rab") == 0)
      {
        // Read the analog bank area
        nResult = Brain.GetAnaBankValuesEx(&AnaBankData);

        // Check the result
        if (SIOMM_OK == nResult)
        {
          printf("\n  ANALOG BANK READ AREA\n");

          // If everything is okay, then print the state of the analog bank read area.
          for (int i = 0 ; i < 64 ; i++)
          {
            printf("  %2d: %12.5f", i, AnaBankData.fValue[i]);
            if ((i+1)%4 == 0)
              printf("\n");
          }
        }
      }     
      else if (strcmp(pchCommandType, "ls") == 0)
      {
        O22SnapIoStream StreamHandler;
        
        // Open streaming and start listening
        StreamHandler.SetCallbackFunctions(NULL, 0, StreamCallback, &StreamHandler, NULL, 0);
        StreamHandler.OpenStreaming(SIOMM_STREAM_TYPE_STANDARD, 0, 5001);
        StreamHandler.StartStreamListening(argv[1], 1000);

        // Wait for the Enter key to be hit. The stream thread will cause screen
        // updates until we get the Enter key.
        getchar();

        // Clean up
        StreamHandler.StopStreamListening(argv[1]);
        StreamHandler.CloseStreaming();
      }
      else
      {
        printf("  Type \"help\" for help.\n");
      }

      if (SIOMM_OK != nResult)
      {
        // Print error message and exit.
        printf("Unable to complete command.  Exiting...\n");
        Brain.Close();
        return 0;
      }

      if (bIncompleteCommand == TRUE)
      {
         printf("  Incomplete command.  Type \"help\" for help.\n");
      }

    }
  } // while (1)


  // Close the connection to the brain
  Brain.Close();

  return 0;
}

