/**
* @file	as-alarma.cpp
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

#include <cstring>
#include <climits>

#include "as-alarma.h"

using namespace std;
namespace argos{

/**
* @brief	Constructor por Defecto de Alarmas
*/
alarma::alarma(){
	id_grupo 			=	-1;
	grupo[0]			=	'\0';
	id_subgrupo			=	-1;
	subgrupo[0]			=	'\0';
	nombre[0]			=	'\0';
	estado				=	EERROR_;
	valor_comparacion.L	=	LONG_MIN;
	tipo_comparacion	=	AERROR_;
	tipo_origen			=	TERROR_;
	siguiente 			=	NULL;
	ptr_siguiente 		=	NULL;
}


/**
* @brief	Constructor de Alarmas
* @param	idg		Identificador de Grupo
* @param	gru		Nombre de Grupo
* @param	idsg	Identificador de Subgrupo
* @param	subgru	Nombre de Subgrupo
* @param	nomb	Nombre de Alarma
* @param	est		Estado
* @param	vc		Valor de Comparacion
* @param	tc		Tipo de Comparacion
* @param	to		Tipo de Dato
*/
alarma::alarma(int idg, char *gru, int idsg, char *subgru, char * nomb,
		Ealarma est, Tvalor vc, Tcomp tc, Tdato to) {
	asignar_propiedades(idg, gru, idsg, subgru, nomb, est, vc, tc, to);
	siguiente = NULL;
	ptr_siguiente = NULL;
}

/**
* @brief	Destructor de Alarmas
*/
alarma::~alarma() {
}

/**
* @brief	Sobrecarga del operador NEW
* @param	n_bytes		Espacio Solicitado
* @param	direccion	Direccion en Memoria Compartida
* @return	direccion	Direccion en Memoria Compartida
*/
void * alarma::operator new(size_t n_bytes, void *direccion){
	return direccion;
}

/**
* @brief	Comparacion de Alarmas
* @param	a		Alarmas
* @return	true	Alarmas con igual Nombre.
*/
bool alarma::es_igual(alarma * a){
	return (strcmp(nombre,a->nombre) == 0);
}

/**
* @brief	Asignacion de atributos a la Alarma
* @param	idg		Identificador de Grupo
* @param	gru		Nombre de Grupo
* @param	idsg	Identificador de Subgrupo
* @param	subgru	Nombre de Subgrupo
* @param	nomb	Nombre de Alarma
* @param	est		Estado
* @param	vc		Valor de Comparacion
* @param	tc		Tipo de Comparacion
* @param	to		Tipo de Dato
* @return	true	Asignacion exitosa
*/
bool alarma::asignar_propiedades(int idg, const char *gru, int idsg,
		const char *subgru, const char * nomb,
		Ealarma est, Tvalor vc, Tcomp tc, Tdato to){
	id_grupo			=	idg;
	strcpy( grupo, gru );
	id_subgrupo			=	idsg;
	strcpy( subgrupo, subgru );
	strcpy( nombre, nomb );
	estado				=	est;
	valor_comparacion	=	vc;
	tipo_comparacion	=	tc;
	tipo_origen			=	to;
	return true;
}

/**
* @brief	Asignacion de atributos a la Alarma
* @param	nomb	Nombre de Alarma
* @param	est		Estado
* @param	to		Tipo de Dato
* @return true		Asignacion exitosa
*/
bool alarma::asignar_propiedades(const char * _nomb,	Ealarma _est, Tdato _to){
	strcpy( nombre, _nomb );
	estado				=	_est;
	tipo_origen			=	_to;
	return true;
}

bool alarma::actualizar_estado( Ealarma _est){
	estado = _est;
}


Ealarma alarma::obtener_estado(){
	return estado;
}

char * alarma::obtener_nombre(){
	return &nombre[0];
}


/**
* @brief	Sobrecarga del operador de copia
* @param	a		Alarma
* @param	est		Estado
* @param	to		Tipo de Dato
* @return	alarma	Copia de Alarma
*/
alarma & alarma::operator = (const alarma & a){
	id_grupo			=	a.id_grupo;
	strcpy( grupo, a.grupo );
	id_subgrupo			=	a.id_subgrupo;
	strcpy( subgrupo, a.subgrupo );
	strcpy( nombre, a.nombre );
	estado				=	a.estado;
	valor_comparacion	=	a.valor_comparacion;
	tipo_comparacion	=	a.tipo_comparacion;
	tipo_origen			=	a.tipo_origen;
	return *this;
}

/**
* @brief	Proxima Alarma de la Lista
* @return	alarma	Puntero a Alarma siguiente
*/
alarma * alarma::ptr_proximo(){
	if(ptr_siguiente)
		return (alarma *)ptr_siguiente;
	else
		return NULL;
}

}
