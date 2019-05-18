/**
* @file	as-tags.cpp
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

#include "as-tags.h"
#include <cstring>

using namespace std;
namespace argos{

/**
* @brief	Constructor por Defecto de Tags
*/
tags::tags(){
        nombre[0] = '\0';
        valor_campo.L = 0;
        tipo = TERROR_;
        siguiente = NULL;
        ptr_siguiente = NULL;
}

/**
* @brief	Constructor de Tags
* @param	el_nombre		Nombre de Tag
* @param	el_valor_campo	Valor del Tag
* @param	el_tipo			Tipo de Tag
*/
tags::tags(char *el_nombre, Tvalor el_valor_campo, Tdato el_tipo) {
        asignar_propiedades(el_nombre,el_valor_campo,el_tipo);
        siguiente = NULL;
        ptr_siguiente = NULL;
}

/**
* @brief	Destructor de Tags
*/
tags::~tags() {
}


char * tags::obtener_nombre(){
	return &nombre[0];
}

/**
* @brief	Sobrecarga del operador NEW
* @param	n_bytes		Espacio Solicitado
* @param	direccion	Direccion en Memoria Compartida
* @return	direccion	Direccion en Memoria Compartida
*/
void * tags::operator new(size_t n_bytes, void *direccion){
        return direccion;
}

/**
* @brief	Comparacion de Tags
* @param	t		Tag
* @return	true	Tag con igual Nombre.
*/
bool tags::es_igual(tags * t){
        return (strcmp(nombre,t->nombre) == 0);
}

/**
* @brief	Asignacion de atributos al Tag
* @param	el_nombre		Nombre de Tag
* @param	el_valor_campo	Valor del Tag
* @param	el_tipo			Tipo de Tag
* @return	true	Asignacion exitosa
*/
bool tags::asignar_propiedades(char *el_nombre, Tvalor el_valor_campo, Tdato el_tipo){
        strcpy(&nombre[0], el_nombre);
        tipo  = el_tipo;
        switch(tipo){
            case CARAC_:
				valor_campo.C = el_valor_campo.C;
        	break;
            case CORTO_:
				valor_campo.S = el_valor_campo.S;
        	break;
        	case ENTERO_:
				valor_campo.I = el_valor_campo.I;
        	break;
        	case LARGO_:
				valor_campo.L = el_valor_campo.L;
        	break;
        	case REAL_:
				valor_campo.F = el_valor_campo.F;
        	break;
        	case DREAL_:
				valor_campo.D = el_valor_campo.D;
        	break;
        	case BIT_:
				valor_campo.B = el_valor_campo.B;
        	break;
        	case TERROR_:
        	default:
				valor_campo.L = _ERR_DATO_;
        }
        return true;
}

/**
* @brief	Asignacion de atributos al Tag
* @param	el_nombre			Nombre de Tag
* @param	el_valor_campo		Valor del Tag
* @param	el_tipo				Tipo de Tag
* @param	el_siguiente		Siguiente Tag
* @param	el_ptr_siguiente	Siguiente Tag Memoria Compartida
* @return	true				Asignacion exitosa
*/
bool tags::asignar_propiedades(const char *el_nombre, const Tvalor el_valor_campo,
				const Tdato el_tipo, tags *el_siguiente, void *el_ptr_siguiente){
        strcpy(&nombre[0], el_nombre);
        tipo	= el_tipo;
        switch(tipo){
        	case CARAC_:
				valor_campo.C = el_valor_campo.C;
        	break;
            case CORTO_:
				valor_campo.S = el_valor_campo.S;
        	break;
        	case ENTERO_:
				valor_campo.I = el_valor_campo.I;
        	break;
        	case LARGO_:
				valor_campo.L = el_valor_campo.L;
        	break;
        	case REAL_:
				valor_campo.F = el_valor_campo.F;
        	break;
        	case DREAL_:
				valor_campo.D = el_valor_campo.D;
        	break;
        	case BIT_:
				valor_campo.B = el_valor_campo.B;
        	break;
        	case TERROR_:
        	default:
				valor_campo.L = _ERR_DATO_;
        }
        siguiente = el_siguiente;
        ptr_siguiente = el_ptr_siguiente;
        return true;
}

/**
* @brief	Asignacion de nombre al Tag
* @param	el_nombre			Nombre de Tag
* @return	true				Asignacion exitosa
*/
bool tags::asignar_nombre( char * el_nombre ){
	strcpy( &nombre[0], el_nombre );
	valor_campo.L = _ERR_DATO_;
	tipo = TERROR_;
	siguiente = NULL;
	ptr_siguiente = NULL;
	return true;
}

/**
* @brief	Proximo Tag de la Lista
* @return	tags	Puntero al Tag siguiente
*/
tags * tags::ptr_proximo(){
        if(ptr_siguiente)
                return (tags *)ptr_siguiente;
        else
                return NULL;
}

/**
* @brief	Tipo de dato del tag
* @return	Tdato	Tdato del tag
*/
Tdato tags::tipo_dato(){
	return tipo;
}
}
