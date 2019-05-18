//-----------------------------------------------------------------------------
//
// O22SIOUT.h
// Copyright (c) 1999 - 2002 by Opto 22
//
// Utility functions for the Ethernet I/O Driver Toolkit C++ classes.  
//
// While this code was developed on Microsoft Windows 32-bit operating 
// systems, it is intended to be as generic as possible.  For Windows specific
// code, search for "_WIN32" and "_WIN32_WCE".  For Linux specific code, search
// for "_LINUX".
//-----------------------------------------------------------------------------

#ifndef __O22SIOUT_H
#include "O22SIOUT.h"
#endif


float O22MAKEFLOAT2(BYTE * a, long o)  
//-------------------------------------------------------------------------------------------------
// This is similar to the O22MAKELONG2 #define, but it had to be put in a function.
//-------------------------------------------------------------------------------------------------
{
  float f;
  long  i = O22MAKELONG2(a, o); 
  
  memcpy(&f, &i, 4);
  
  return f;
}
