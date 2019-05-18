/*
  header:      enetbootp.h

  desc:        enetheader for the bootp project

  author:      Bryce A. Nakatani (2000-01-10)
  revised:

  notes:

  DISCLAIMER:  This source code is supplied as shareware, use at your own risk without any gaurantees or support.

*/

#ifndef _ENETBOOTP_HDR_
#define _ENETBOOTP_HDR_

// default IP and SubNet Mask
//#define DEFAULT_IP_ADDRESS       0x16161616
//#define DEFAULT_SUBNET_MASK      0xFFFFFF00
//#define DEFAULT_GATEWAY          0x00000000
//#define DEFAULT_DNS              0x00000000
//#define BROADCAST_IP_ADDRESS     "255.255.255.255"

#define BYTE   char
#define WORD   short
#define DWORD  long


//#define OPTO_MAC_ADDRESS         0x00A03D00   // only the upper 3 bytes are used

// byte defines for opto hardware
#define OPTO_MAC_BYTE0             0x00
#define OPTO_MAC_BYTE1             0xa0
#define OPTO_MAC_BYTE2             0x3d

// defines for radix values
//#define RADIX_DECIMAL            0x0A
//#define RADIX_HEX                0x10

// Opto BootP "Magic Number"
#define MAGIC_COOKIE             0x63825363

// defines for the BootP ports
#define BOOTP_PORT_SERVER        0x43
//#define BOOTP_PORT_SERVER        0x1000
#define BOOTP_PORT_CLIENT        0x44

// defines for various buffer sizes
#define BOOTP_PACKET_SIZE        0x12C
#define BOOTP_BUF_SIZE           0x1f4
#define BOOTP_VENDOR_DATA_SIZE   0x40
#define VENDOR_TAG_DESC_LEN      0x26

// defines for BootP value
#define BOOTP_VALUE_REQUEST      0x01  // a request packet
#define BOOTP_VALUE_REPLY        0x02  // a reply packet
#define BOOTP_VALUE_HOPS         0x00  // number of hops
#define BOOTP_VALUE_HLEN         0x06  // for ethernet
#define BOOTP_VALUE_HTYPE_ENET   0x01  // ethernet

// defines for offsets into the BOOTP packet
#define BOOTP_OFFSET_OP          0x00
#define BOOTP_OFFSET_HTYPE       0x01
#define BOOTP_OFFSET_HOPS        0x03
#define BOOTP_OFFSET_HLEN        0x02
#define BOOTP_OFFSET_TID         0x04
#define BOOTP_OFFSET_YOUR_IP     0x10
#define BOOTP_OFFSET_GATEWAY     0x18
#define BOOTP_OFFSET_HWD_ADDR    0x1C
#define BOOTP_OFFSET_VENDOR_DATA 0xEC

// tags for the vendor data area of the BootP packet
#define VENDOR_TAG_PAD           0x00
#define VENDOR_TAG_SUBNET        0x01
#define VENDOR_TAG_DNS           0x06
#define VENDOR_TAG_END           0xFF
#define VENDOR_TAG_OPTOADDRESS   0x80
#define VENDOR_TAG_LOADER        0x81
#define VENDOR_TAG_KERNEL        0x82
#define VENDOR_TAG_DESC          0x83

#define BOOTP_PORT_SERVER        0x43
#define BOOTP_PORT_CLIENT        0x44


#endif
