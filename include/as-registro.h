/**
* @file	as-registro.h
* @class registro
* @brief		<b>Tipo Abstracto de Datos para el Manejo de Registros en los Dispositivos</b><br>
* @author	    Alejandro Pina <ajpina@gmail.com><br>
* @link			http://www.cintal.com.ve/tecnologia/argos<br>
* @package      argos
* @access       public
* @version      1.0  -  01/06/09
*/

/*
Copyright (C) 2006 Alejandro Pina <ajpina@gmail.com>

Este programa es software libre. Puede redistribuirlo y/o modificarlo bajo
los terminos de la Licencia Publica General de GNU segun es publicada por
la Free Software Foundation, bien de la version 3 de dicha Licencia o bien
(segun su eleccion) de cualquier version posterior.

Este programa se distribuye con la esperanza de que sea util, pero
SIN NINGUNA GARANTIA, incluso sin la garantia MERCANTIL implicita o sin
garantizar la CONVENIENCIA PARA UN PROPOSITO PARTICULAR. Vease la Licencia
Publica General de GNU para mas detalles.

Deberia haber recibido una copia de la Licencia Publica General junto con
este programa. Si no ha sido asi, escriba a la Free Software Foundation, Inc.,
en 675 Mass Ave, Cambridge, MA 02139, EEUU.

Alejandro Pina mediante este documento renuncia a cualquier interes de derechos de
copyright con respecto al programa 'argos'.

01 de Noviembre de 2008

Por favor reporte cualquier fallo a la siguiente direccion:

	http://www.cintal.com.ve/tecnologia/argos

*/

#ifndef REGISTRO_H_
#define REGISTRO_H_

#include "as-util.h"

#include <cstring>
#include <iostream>

namespace argos{

class registro{
public:
	char direccion[_LONG_NOMB_REG_];		// Direccion en el Dispositivo
	Tdato tipo;								// Tipo de Dato
	Tvalor valor;							// Dato

/*--------------------------------------------------------
	Comparacion de Registros por Direcciones.
----------------------------------------------------------*/
	bool operator < (const registro & b) const{
		return (strcmp(direccion,b.direccion) < 0);
	}

/*--------------------------------------------------------
	Comparacion de Registros por Direcciones.
----------------------------------------------------------*/
	bool operator == (const registro & b) const{
		return (strcmp(direccion,b.direccion) == 0);
	}

/*--------------------------------------------------------
	Copia de Registros.
----------------------------------------------------------*/
	registro & operator = (const registro & a){
		strcpy(direccion,a.direccion);
		tipo = a.tipo;
		valor = a.valor;
		return *this;
	}

/*--------------------------------------------------------
	Operador de Salida para el Registro.
----------------------------------------------------------*/
	friend std::ostream& operator << (std::ostream & os, const registro & r){
		switch(r.tipo){
			case CARAC_:
				os << r.direccion << "\tTipo: CARACTER\tValor: " << static_cast<palabra_t> (r.valor.C) << "\n";
			break;
			case CORTO_:
				os << r.direccion << "\tTipo: CORTO\tValor: " << r.valor.S << "\n";
			break;
			case ENTERO_:
				os << r.direccion << "\tTipo: ENTERO\tValor: " << r.valor.I << "\n";
			break;
			case LARGO_:
				os << r.direccion << "\tTipo: LARGO\tValor: " << r.valor.L << "\n";
			break;
			case REAL_:
				os << r.direccion << "\tTipo: REAL\tValor: " << r.valor.F << "\n";
			break;
			case DREAL_:
				os << r.direccion << "\tTipo: DREAL\tValor: " << r.valor.D << "\n";
			break;
			case BIT_:
				os << r.direccion << "\tTipo: BIT\tValor: " << r.valor.B << "\n";
			break;
			case TERROR_:
			default:
				os << r.direccion << "\tTipo: DESCONOCIDO\tValor: " << r.valor.L << "\n";
		}
		return os;
	}


};

}

#endif
