
Opto 22 SNAP Ethernet I/O Driver Toolkit README.TXT

Date:    August 26, 2002
Version: R2.0a


==========================================================================

INTRODUCTION

This file describes the Opto 22 SNAP Ethernet I/O Driver Toolkit, a toolkit
provided by Opto 22 for communicating with SNAP Ethernet I/O and 
Ultimate I/O units .  

The toolkit consists of ActiveX components and C++ classes that make it 
easy to communicate with the SNAP Ethernet brains by hiding the details of
Ethernet communications and the IEEE 1394-based memory map protocol. 

Examples are included of using the toolkit in Visual C++, Visual Basic, 
Access, Word, Internet Explorer with VBScript, Borland Delphi, 
and Linux. 

See "SNAP Ethernet Brain Programming Guide" (Form #1227) and "SNAP 
Ultimate I/O Alternate Programming Methods Guide" (Form #1312) for more
information.

==========================================================================

DOCUMENTATION

The toolkit is documented in "SNAP Ethernet Brain Programming 
Guide" (Form #1227) and "SNAP Ultimate I/O Alternate Programming 
Methods Guide" (Form #1312), which can be found on the Opto 22 Web site at 

http://www.opto22.com/Datasheets/1227_SNAP_Ethernet_Brain_Programming_Guide.pdf
http://www.opto22.com/Datasheets/1312_SNAP_Ultimate_IO_Alternate_Programming_Methods_Guide.pdf


==========================================================================

RELEASE HISTORY

Version: R2.0a  August 26, 2002

  NEW FEATURES:

    + Added the following commands to the OptoSnapIoMemMapX ActiveX component
      and O22SnapIoMemMap C++ class:

      SetStreamConfiguration
      SetStreamTarget
      GetStreamConfiguration
      GetStreamTarget
      GetStreamReadAreaEx
      GetStreamReadArea

      WriteBlock            *
      ReadBlock             *
      GetLongAtBlockIndex   *
      GetFloatAtBlockIndex  *
      SetLongAtBlockIndex   *
      SetFloatAtBlockIndex  *

      OpenEnet2

      GetStatusWatchdogTime
      GetStatusHardware2     *
      GetStatusHardwareEx2   +
      GetStatusNetwork2      *
      GetStatusNetworkEx2    +

      GetDateTime
      SetDateTime

      GetSerialModuleConfigurationEx
      SetSerialModuleConfigurationEx

      SetAnaPtTpoPeriod
      GetAnaPtTpoPeriod

      SetPtConfiguration2    *
      GetPtConfiguration2    *
      SetPtConfigurationEx2  +
      GetPtConfigurationEx2  +

      GetScratchPadBitArea
      GetScratchPadIntegerArea
      GetScratchPadFloatArea
      GetScratchPadStringArea
      SetScratchPadBitArea
      SetScratchPadBitAreaMask
      SetScratchPadIntegerArea
      SetScratchPadFloatArea
      SetScratchPadStringArea
      SetScratchPadStringAreaBin   *

      * means this method was added only to the ActiveX component
      + means this method was added only to the C++ class

      
    
    + Added the OptoSnapIoStreamX ActiveX component and the 
      O22SnapIoStream C++ class for receiving and handling stream packets
      from multiple SNAP brains.

    + Added UDP support. The new OptoEnet2() method has an argument for specifying
      TCP or UDP.

    + Added the following examples:

      + Microsoft Access 2000:     Demo Center
      + Borland Delphi 5:          Digital Bank
      + Visual Basic 6:            Scratch Pad, Streaming
      + Visual C++ 6.0 and Linux:  eiocl


  BUG FIXES:

    + The functions SetPtConfigurationEx() and SetPtConfiguration()
      were overwriting new fields in the point configuration area.

    + Timeouts of less than a second in the functions OpenEnet() and
      SetCommOptions() did not work correctly.



Version: R1.0b  August 11, 1999

  BUG FIXES:

    + All Demo Center examples had the wrong I/O configuration.



Version: R1.0a  June 18, 1999

  First official release.


==========================================================================

USE

The toolkit includes all source code, which may be modified, reused, 
and borrowed at your own risk.  Opto 22 does not provide support for modifying
the code in this toolkit.


==========================================================================

DISTRIBUTION

The SNAP Ethernet I/O Driver Toolkit may be downloaded from the 
Opto 22 Web site at http://www.opto22.com/products/driver_tool.asp.
  
There are two installations available:

1) For Windows, download OptoEnetDTK.exe. It will copy all files and register 
   the ActiveX component. 

   If you have any troubles using the toolkit, download and install 
   OptoENETUtilities.exe, which will update various system files.

2) For Linux, download optoenetdtk.tar. It contains just the files 
   needed for a Linux system.


==========================================================================

HOW TO GET HELP

If you have any questions about this product, contact Opto 22 Product 
Support Monday through Friday, 8 a.m. to 5 p.m., Pacific Time.

Phone:  800/TEK-OPTO (835-6786)
        909/695-3080

Fax:    800/832-OPTO (6786)
        909/695-3017

Internet:
  Web site: www.opto22.com
  BBS site: bbs.opto22.com
  e-mail:   support@opto.com

Bulletin Board (BBS): 
  (24 hours a day, 7 days a week)
  
  909/695-1367 
  http://bbs.opto22.com

When accessing the BBS, use the following modem settings:
  - No parity, 8 data bits, 1 stop bit
  - Baud rates up to 28,800
  - Z-modem protocol for uploads and downloads

When calling for technical support, be prepared to provide the following
information about your system to the Product Support engineer:

  - Version of this product
  - PC configuration
  - A complete description of your hardware system, including:
    - jumper configuration
    - accessories installed (such as daughter cards)
    - type of power supply
    - types of I/O units installed
    - third-party devices installed (e.g., barcode readers)
  - Controller firmware version
  - Any specific error messages seen

