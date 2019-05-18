/**
* @file	as-tags.h
* @class tags
* @brief		<b>Tipo Abstracto de Datos para el Manejo
*				de Tags, usado como elemento fundamental
*				en la comunicacion entre el sistema supervisor
*				y los distintos dispositivos</b><br>
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

#ifndef TAGS_H_
#define TAGS_H_

#include <cstring>
#include <iostream>
#include <cstdio>

#include "as-util.h"



using namespace std;

namespace argos {

const size_t _SIZEOF_TAG_		=	64;		//!< Tamano en Bytes del Tag

class tags {
public:
    char		nombre[_LONG_NOMB_TAG_];	//!< Nombre del Tag
    Tvalor		valor_campo;				//!< Valor del Tag
    Tdato		tipo;						//!< Tipo de Dato del Tag
    tags		*siguiente;					//!< Proximo Tag de la Lista
    void		*ptr_siguiente;				//!< Proximo Tag de la Lista Memoria Compartida

    tags();
    tags(char *el_nombre, Tvalor el_valor_campo, Tdato el_tipo);
    ~tags();

    char * obtener_nombre();

    /*--------------------------------------------------------
    	Devuelve una direccion en el Segmento de
    	Memoria Compartida
    ----------------------------------------------------------*/
    void * operator new(size_t n_bytes, void *direccion);

    /*--------------------------------------------------------
    	No hace nada ya que la direccion entregada se
    	encuentra en el Segmento de Memoria Compartida
    ----------------------------------------------------------*/
    void operator delete(void *) {}

    /*--------------------------------------------------------
    	Funciones de Asignacion de Atributos a los
    	Tags
    ----------------------------------------------------------*/
    bool asignar_propiedades(char *el_nombre, Tvalor el_valor_campo, Tdato el_tipo);
    bool asignar_propiedades( const char *, const Tvalor , const Tdato, tags *, void * );
    bool asignar_nombre( char * );

    /*--------------------------------------------------------
    	Operadores de Comparacion de Tags por
    	el nombre
    ----------------------------------------------------------*/
    int operator==(const tags & der) const {
        return (strcmp(nombre, der.nombre) == 0);
    }

    int operator!=(const tags & der) const {
        return (strcmp(nombre, der.nombre) != 0);
    }

    /*--------------------------------------------------------
    	Operador de Salida para los Tags
    ----------------------------------------------------------*/
    friend ostream& operator << (ostream & os, const tags & ct) {
        char NumAStr[15];
        switch(ct.tipo) {
        case CARAC_:
            os << ct.nombre;
            mostrar_espaciado(os,ct.nombre,34);
            os << static_cast<palabra_t> (ct.valor_campo.C) ;
            sprintf(NumAStr,"%c",ct.valor_campo.C);
            mostrar_espaciado(os,NumAStr,17);
            os<< " Caracter";
            break;
        case CORTO_:
            os << ct.nombre;
            mostrar_espaciado(os,ct.nombre,34);
            os << ct.valor_campo.S;
            sprintf(NumAStr,"%d",ct.valor_campo.S);
            mostrar_espaciado(os,NumAStr,17);
            os << "  Corto";
            break;
        case ENTERO_:
            os << ct.nombre;
            mostrar_espaciado(os,ct.nombre,34);
            os << ct.valor_campo.I;
            sprintf(NumAStr,"%d",ct.valor_campo.I);
            mostrar_espaciado(os,NumAStr,17);
            os << "  Entero";
            break;
        case LARGO_:
            os << ct.nombre;
            mostrar_espaciado(os,ct.nombre,34);
            os << ct.valor_campo.L;
            sprintf(NumAStr,"%lld",ct.valor_campo.L);
            mostrar_espaciado(os,NumAStr,17);
            os<< "  Largo ";
            break;
        case REAL_:
            os << ct.nombre;
            mostrar_espaciado(os,ct.nombre,34);
            os << ct.valor_campo.F;
            sprintf(NumAStr,"%f",ct.valor_campo.F);
            mostrar_espaciado(os,NumAStr,17);
            os << "  Real ";
            break;
        case DREAL_:
            os << ct.nombre;
            mostrar_espaciado(os,ct.nombre,34);
            os << ct.valor_campo.D;
            sprintf(NumAStr,"%f",ct.valor_campo.D);
            mostrar_espaciado(os,NumAStr,17);
            os << "  DReal";
            break;
        case BIT_:
            os << ct.nombre;
            mostrar_espaciado(os,ct.nombre,34);
            os << ct.valor_campo.B;
            sprintf(NumAStr,"%d",ct.valor_campo.B);
            mostrar_espaciado(os,NumAStr,17);
            os << "  Bit";
            break;
        case TERROR_:
        default:
            os << ct.nombre << ct.valor_campo.L << "  Desconocido";
        }
        return os;
    }

    /*--------------------------------------------------------
    	Comparacion de Tags cuando se requiere hacer
    	busquedas
    ----------------------------------------------------------*/
    bool es_igual(tags * t);

    /*--------------------------------------------------------
    	Siguiente Tag en la Lista
    ----------------------------------------------------------*/
    tags * ptr_proximo();

    /*--------------------------------------------------------
    	Tipo dato del tag
    ----------------------------------------------------------*/
    Tdato tipo_dato();
};


/*--------------------------------------------------------
	Estructura de Datos Portable usada para trasnferir
	Tags entre Servidores y Clientes de Distintas
	Plataformas
----------------------------------------------------------*/
union Ttags_portable {
    char				dimension[_SIZEOF_TAG_];		//!< Tamano de la Estructura
    struct {
        char			nombre[_LONG_NOMB_TAG_];    	//!< Nombre del Tag
        Tvalor			valor_campo;					//!< Valor del Tag
        Tdato			tipo;							//!< Tipo de Tag
    };
};

/*--------------------------------------------------------
	Arreglo de Tags Portable usada para trasnferir
	Tags entre Servidores y Clientes de Distintas
	Plataformas
----------------------------------------------------------*/
struct paquete_tags {
    __uint64_t          tipo;							//!< Alarmas o Tags
    Ttags_portable      arreglo[_MAX_TAM_ARR_TAGS_];	//!< Arreglo de Alarmas Portables
};


}

#endif // SSTAGS_H_
