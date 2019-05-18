/**
* @file	as-shmhistorico.h
* @class shm_historico
* @brief		<b>Tipo Abstracto de Datos para el Almacenamiento de
				un buffer circular en Memoria Compartida para el almacenamiento
				de tags y alarmas</b><br>
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

#ifndef SHMHISTORICO_H
#define SHMHISTORICO_H

#include <string>

#include "as-historico.h"


namespace argos{

class shm_historico
{
	private:
		int fd_t;												//!< Identificador de SHM
		int fd_a;												//!< Identificador de SHM
		unsigned int tamano_bloque_t;		//!< Tamano del segmento de memoria
		unsigned int tamano_bloque_a;		//!< Tamano del segmento de memoria
		std::string nombre_tendencia;				//!< Nombre del buffer de tags
		std::string nombre_alarmero;					//!< Nombre del buffer de alarmas

		typedef struct _Ttab_tendencia{
			unsigned int	cantidad;
			tendencia			*tend;
		}Ttab_tendencia;
		Ttab_tendencia	*ptr_tab_tendencia;

		typedef struct _Ttab_alarmero{
			unsigned int	buf_len;
			alarmero			*alarm;
		}Ttab_alarmero;
		Ttab_alarmero		*ptr_tab_alarmero;

	public:
		unsigned long * mi_segmento_inicio_t;
		unsigned long * mi_segmento_inicio_a;


		shm_historico(std::string t = "tendencia", std::string a = "alarmero");
		~shm_historico();


/*--------------------------------------------------------
	Las mismas funciones del constructor, para ser usado
	por un Objeto global
----------------------------------------------------------*/
	bool inicializar_shm_historico(std::string ,std::string );
	bool inicializar_shm_tendencia(std::string );
	bool inicializar_shm_alarmero(std::string );

/*--------------------------------------------------------
	Utiliza una direccion de segmento unica para
	que todas las instancias del objeto compartan
	la misma memoria compartida
----------------------------------------------------------*/
	void asignar_almacenamiento_segmento_inicio(void * ,void * );

/*--------------------------------------------------------
	Actualiza el apuntador a la Tabla Hash
----------------------------------------------------------*/
	void asignar_ref_historico(void * ,void * );

/*--------------------------------------------------------
	Tamano del segmento de memoria compartida
----------------------------------------------------------*/
	unsigned int sizeof_bloque_tendencia();
	unsigned int sizeof_bloque_alarmero();

/*--------------------------------------------------------
	Crear la Memoria Compartida para la Tabla Hash
	de Tags
----------------------------------------------------------*/
	bool crear_shm_tendencia(unsigned int );
	bool crear_shm_alarmero(unsigned int );

/*--------------------------------------------------------
	Obtener acceso a una Memoria Compartida previamente
	creada
----------------------------------------------------------*/
	bool obtener_shm_tendencia();
	bool obtener_shm_alarmero();

/*--------------------------------------------------------
	Cierra el segmento de Memoria Compartida
----------------------------------------------------------*/
	bool cerrar_shm_tendencia();
	bool cerrar_shm_alarmero();

/*--------------------------------------------------------
	Imprime por pantalla el contenido de la
	Tabla Hash
----------------------------------------------------------*/
	void imprimir_shm_tendencia();
	void imprimir_shm_alarmero();


	tendencia * obtener_tendencia(unsigned int);
	alarmero * obtener_alarmero(unsigned int);




};

}

#endif // SHMHISTORICO_H


