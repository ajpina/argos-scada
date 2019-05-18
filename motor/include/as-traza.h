/**
* @file	as-traza.h
* @class traza
* @brief		<b>Tipo Abstracto de Datos para el Itercambio de Tendencias
				Entre Servidores y Clientes (Usando Serializacion en XML)</b><br>
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

#ifndef TRAZA_H_
#define TRAZA_H_

#include <sys/time.h>

#include "as-util.h"

namespace argos{

class puntraza{
private:
	struct timeval	tiempo;
	Tvalor			valor;
	size_t			posicion;
};

class eventraza{
private:
	struct timeval	tiempo;
	Ealarma			estado;
	size_t			posicion;
};

class vartraza{
private:
	char			nombre[_LONG_NOMB_TAG_];	//!< Nombre del Tag
	size_t			n_puntos;					//!< Numero de Puntos
	puntraza		*puntos;					//!< Puntos de la Variable
	struct timeval	desde;
	struct timeval	hasta;
};

class alartraza{
private:
	char			nombre[_LONG_NOMB_ALARM_];	//!< Nombre de la Alarma
	size_t			n_eventos;					//!< Numero de Eventos
	eventraza		*eventos;					//!< Eventos de la Alarma
	struct timeval	desde;
	struct timeval	hasta;
};

class traza{
private:
	size_t			n_variables;				//!< Numero de Variables
	vartraza		*variables;					//!< Variables de la Traza
	size_t			n_alarmas;					//!< Numero de Alarmas
	alartraza		*alarmas;					//!< Alarmas de la Traza


public:
	traza();
	~traza();

/*--------------------------------------------------------
	Producir un Arreglo de Tags para el grupo Multicast
----------------------------------------------------------*/
	void enviar_traza();

};


}


#endif /*SSTRAZA_H_*/

