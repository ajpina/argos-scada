/**
* @file	as-alarma.h
* @class alarma
* @brief		<b>Tipo Abstracto de Datos para el Manejo de Alarmas</b><br>
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

#ifndef ALARMA_H_
#define ALARMA_H_

#include <iostream>
#include <cstdio>

#include "as-util.h"



using namespace std;

namespace argos{

const size_t _SIZEOF_ALARMA_		=	64;			//!< Tamano del Paquete de Alarmas

class alarma{
public:
	int			id_grupo;						//!< Identificador de Grupo de Alarmas
	char		grupo[_LONG_NOMB_GALARM_];		//!< Nombre del Grupo de Alarmas
	int			id_subgrupo;					//!< Identificador del Subgrupo de Alarmas
	char		subgrupo[_LONG_NOMB_SGALARM_];	//!< Nombre del Subgrupo de Alarmas
	char		nombre[_LONG_NOMB_ALARM_];		//!< Nombre de las Alarmas
	Ealarma		estado;							//!< Estado de la Alarma
	Tvalor		valor_comparacion;				//!< Valor para Comparar la Alarma
	Tcomp		tipo_comparacion;				//!< Tipo de Comparacion a aplicar
	Tdato		tipo_origen;					//!< Tipo de Dato del Registro
	alarma		*siguiente;						//!< Proximo Alarma de la Lista
	void 		*ptr_siguiente;					//!< Proxima Alarma de la Lista en Memoria Compartida


	alarma();
	alarma(int, char *, int, char *, char *, Ealarma, Tvalor, Tcomp, Tdato);
	~alarma();


	Ealarma obtener_estado();
	char * obtener_nombre();

/*--------------------------------------------------------
	Devuelve una direccion en el Segmento de
	Memoria Compartida
----------------------------------------------------------*/
	void * operator new(size_t, void *);

/*--------------------------------------------------------
	No hace nada ya que la direccion entregada se
	encuentra en el Segmento de Memoria Compartida
----------------------------------------------------------*/
	void operator delete(void *){}


/*--------------------------------------------------------
	Funciones de Asignacion de Atributos a las
	Alarmas
----------------------------------------------------------*/
	bool asignar_propiedades(int, const char *, int, const char *, const char *, Ealarma,
					Tvalor, Tcomp, Tdato);
	bool asignar_propiedades(const char *, Ealarma, Tdato);
	alarma & operator = (const alarma & );
	bool actualizar_estado(Ealarma);


/*--------------------------------------------------------
	Operadores de Comparacion de Alarmas por
	el nombre
----------------------------------------------------------*/
	int operator < (const alarma & der) const{
		return strcmp(nombre,der.nombre) < 0;
	}

	int operator > (const alarma & der) const{
		return strcmp(nombre,der.nombre) > 0;
	}

	int operator==(const alarma & der) const{
		return (strcmp(nombre, der.nombre) == 0);
	}

	int operator!=(const alarma & der) const{
		return (strcmp(nombre, der.nombre) != 0);
	}

/*--------------------------------------------------------
	Operador de Salida para las Alarmas
----------------------------------------------------------*/
	friend ostream& operator << (ostream & os, const alarma & ct){
		os << ct.nombre;
		mostrar_espaciado(os,ct.nombre,29);
		switch(ct.estado){
				case BAJO_:
					os << "  BAJO";
				break;
				case BBAJO_:
					os << "  BBAJO";
				break;
				case ALTO_:
					os << "  ALTO";
				break;
				case AALTO_:
					os << "  AALTO";
				break;
				case ACTIVA_:
					os << "  ACTIVA";
				break;
				case DESACT_:
					os << "  DESACT";
				break;
				case RECON_:
					os << "  RECON";
				break;
				case EERROR_:
				default:
					os << "  DESCONOCIDO";
			}

		return os;
	}


/*--------------------------------------------------------
	Comparacion de Alarmas cuando se requiere hacer
	busquedas
----------------------------------------------------------*/
	bool es_igual(alarma * );

/*--------------------------------------------------------
	Siguiente Alarma en la Lista
----------------------------------------------------------*/
	alarma * ptr_proximo();
};


/*--------------------------------------------------------
	Estructura de Datos Portable usada para trasnferir
	Alarmas entre Servidores y Clientes de Distintas
	Plataformas
----------------------------------------------------------*/
union Talarmas_portable{
    char                    dimension[_SIZEOF_ALARMA_];	//!< Tamano de la Estructura
    struct{
        char                nombre[_LONG_NOMB_ALARM_];	//!< Nombre de la Alarma
        Ealarma             estado;						//!< Estado de la Alarma
        Tdato			    tipo_origen;				//!< Tipo de Dato del Registro
    };
};

/*--------------------------------------------------------
	Arreglo de Alarmas Portable usada para trasnferir
	Alarmas entre Servidores y Clientes de Distintas
	Plataformas
----------------------------------------------------------*/
struct paquete_alarmas{
	__uint64_t          tipo;							//!< Alarmas o Tags
	Talarmas_portable   arreglo[_MAX_TAM_ARR_ALARM_];	//!< Arreglo de Alarmas Portables
};

}

#endif /*SSALARMA_H_*/
