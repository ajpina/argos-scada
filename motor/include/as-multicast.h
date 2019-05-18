/**
* @file	as-multicast.h
* @class mcast
* @brief		<b>Tipo Abstracto de Datos para el Itercambio de Datos
				Entre Servidores y Clientes (Usando Multicast)</b><br>
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

#ifndef MULTICAST_H_
#define MULTICAST_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "as-tags.h"
#include "as-alarma.h"

#define _PUERTO_	16001
#define _GRUPO_		"224.0.1.1"
#define _TTL_		64
#define _LOOP_		1

namespace argos{

class mcast{
private:
	int puerto;							//!< Numero de Puerto
	char grupo[20];						//!< IP del Grupo Multicast
	struct sockaddr_in direccion;
	struct ip_mreq requisitos;
	int sock;							//!< Socket UDP
	bool servidor;						//!< Operar como Productor o Consumidor
	short ttl;							//!< Time To Live
	short loop;							//!< Replicar en localhost

public:
	mcast();
	mcast(bool serv, int p = _PUERTO_, char * g = _GRUPO_, short t = _TTL_, short l = _LOOP_);
	~mcast();

/*--------------------------------------------------------
	Reservar los recursos necesarios para iniciar la
	Comunicacion Multicast
----------------------------------------------------------*/
	void iniciar_mcast();

/*--------------------------------------------------------
	Asignacion de Atributos
----------------------------------------------------------*/
	void asignar_puerto(int);
	void asignar_grupo(const char *);
	void asignar_ttl(short);
	void asignar_loop(short);
	void hacer_servidor(bool);

/*--------------------------------------------------------
	Producir un Arreglo de Tags para el grupo Multicast
----------------------------------------------------------*/
	void enviar_tags(Ttags_portable * msg, unsigned int ntags, unsigned int ntags_x_msg, unsigned long frec);

/*--------------------------------------------------------
	Producir un Arreglo de Alarmas para el grupo Multicast
----------------------------------------------------------*/
	void enviar_alarmas(Talarmas_portable * msg, unsigned int ntags, unsigned int ntags_x_msg, unsigned long frec);

/*--------------------------------------------------------
	Consumir un Mensaje desde el grupo Multicast
----------------------------------------------------------*/
	int recibir_msg(void * msg, unsigned int tam);
};


}


#endif /*SSMULTICAST_H_*/
