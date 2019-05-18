/**
* @file	as-shmalarma.h
* @class shm_thalarma
* @brief		<b>Tipo Abstracto de Datos para el Almacenamiento de
				una Tabla Hash de Alarmas en Memoria Compartida</b><br>
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


#ifndef SHMALARMA_H_
#define SHMALARMA_H_

#include <cstring>
#include "as-util.h"
#include "as-alarma.h"
#include "as-thalarma.h"
#include <pthread.h>

namespace argos{

class shm_thalarma{
private:
	int fd;							//!< Identificador de SHM
	unsigned int tamano_bloque;		//!< Tamano del segmento de memoria
	string nombre;					//!< Nombre de la memoria compartida
	thalarma * th;					//!< Referencia a la Tabla Hash

public:
	unsigned long * mi_segmento_inicio;	//!< Direccion de la Memoria Compartida

	shm_thalarma(string n = "thalarma");
	~shm_thalarma();

/*--------------------------------------------------------
	Las mismas funciones del constructor, para ser usado
	por un Objeto global
----------------------------------------------------------*/
	bool inicializar_shm_thalarma(std::string n);

/*--------------------------------------------------------
	Utiliza una direccion de segmento unica para
	que todas las instancias del objeto compartan
	la misma memoria compartida
----------------------------------------------------------*/
	void asignar_almacenamiento_segmento_inicio(void * mi_si);

/*--------------------------------------------------------
	Actualiza el apuntador a la Tabla Hash
----------------------------------------------------------*/
	void asignar_ref_tablahash(void * direccion);

/*--------------------------------------------------------
	Devuelve la referencia a la Tabla Hash
----------------------------------------------------------*/
	thalarma * obtener_tablahash(){	return th;	}

/*--------------------------------------------------------
	Tamano del segmento de memoria compartida
----------------------------------------------------------*/
	unsigned int sizeof_bloque();

/*--------------------------------------------------------
	Crear la Memoria Compartida para la Tabla Hash
	de Alarmas
----------------------------------------------------------*/
	bool crear_shm_tablahash(unsigned int c);

/*--------------------------------------------------------
	Obtener acceso a una Memoria Compartida previamente
	creada
----------------------------------------------------------*/
	bool obtener_shm_tablahash();

/*--------------------------------------------------------
	Cierra el segmento de Memoria Compartida
----------------------------------------------------------*/
	bool cerrar_shm_tablahash();

/*--------------------------------------------------------
	Agregar Alarmas a la Tabla Hash
----------------------------------------------------------*/
	bool insertar_alarma(alarma t);

/*--------------------------------------------------------
	Buscar Alarmas en la Tabla Hash
----------------------------------------------------------*/
	bool buscar_alarma(alarma * t);
	alarma * buscar_alarma( std::string );

/*--------------------------------------------------------
	Actualiza el Estado de la Alarma en la Tabla Hash
----------------------------------------------------------*/
	bool actualizar_alarma(alarma * t);

/*--------------------------------------------------------
	Elimina la Alarma
----------------------------------------------------------*/
	bool borrar_alarma(alarma t);

/*--------------------------------------------------------
	Imprime por pantalla el contenido de la
	Tabla Hash
----------------------------------------------------------*/
	void imprimir_shm_tablahash();

/*--------------------------------------------------------
	Modifica la Direccion de Referencia para la Tabla Hash
	de Alarmas
----------------------------------------------------------*/
	void actualizar_shm_tablahash(thalarma * nueva_th, void * ni);

/*--------------------------------------------------------
	Crea un Arreglo de Alarmas desde la Tabla Hash
----------------------------------------------------------*/
	void obtener_alarmas(alarma * arreglo, unsigned int * tamano_arreglo );
	void obtener_alarmas_portable(Talarmas_portable * arreglo, unsigned int * tamano_arreglo );


};


}

#endif /*SSSHMALARMA_H_*/
