/***************************************************************************
 *   Copyright (C) 2006 by TuxPLC					                                 *
 *   Author Stephane JEANNE s.jeanne@tuxplc.net                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "config.h"     // Agregado 22-01-09 soporte Arquitectura de 64 bits
#include <stdint.h>     // Agregado 22-01-09 soporte Arquitectura de 64 bits

#ifndef _CIP_TYPES_H
#define _CIP_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif


#ifndef WIN32
	typedef unsigned char BYTE;
	typedef unsigned short WORD;
#else
     #include <windef.h>
#endif


#ifdef _x86_64_          // Agregado 22-01-09 soporte Arquitectura de 64 bits
    typedef unsigned int LONGWORD;  // 4 byte en Arquitectura de 64 bits
#else /* !_x86_64_ */
    typedef unsigned long LONGWORD; // 4 byte en Arquitectura de 32 bits
#endif


typedef char CIP_SINT;	//128..127	8 bits sign�
typedef short CIP_INT;	//32768..32767	16 bits sign�
#ifdef _x86_64_          // Agregado 22-01-09 soporte Arquitectura de 64 bits
    typedef int CIP_DINT;	//2147483648..2147483647	32 bits sign� en Arquitectura de 64 bits
    typedef long CIP_LINT;	//2^0..2^63 64 bits sign� en Arquitectura de 64 bits
#else /* !_x86_64_ */
    typedef long CIP_DINT;	//2147483648..2147483647	32 bits sign� en Arquitectura de 32 bits
    typedef long long int CIP_LINT;	//2^0..2^63 64 bits sign� en Arquitectura de 32 bits
#endif
typedef BYTE CIP_USINT;	//0..255	8 bits non sign�
typedef WORD CIP_UINT;	//0..65535	16 bits non sign�
typedef LONGWORD CIP_UDINT;//0..4294967295	32 bits non sign�

typedef enum _EPath_Format{FPadded,FPacked} EPath_Format;
typedef enum _EPath_LogicalAdress{_8Bits,_16Bits,_32Bits} EPath_LogicalAdress;
typedef BYTE Path_Segment;
typedef Path_Segment EPath[0];
typedef EPath *PEPath;

#ifdef __cplusplus
}
#endif

#endif /* _CIP_TYPES_H */
