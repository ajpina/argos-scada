/**
* @file	as-historico.cpp
* @brief		<b>Tipo Abstracto de Datos para el Almacenamiento
*				de Valores Historicos y Alarmas</b><br>
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

#include "as-historico.h"
#include "as-util.h"

using namespace argos;


/**
* @brief Crea un objeto Historico para albergar el Tag a muestrear
* @return referencia al objeto
*/
historico::historico(){
	tipo_log = MUESTREO_;
	tiempo_muestreo = 1000;
	maximo = (float) 0xFFFFFFFF;
	minimo = (float) 0xFFFFFFFF;
	muestras = 100;
	cant_funciones = 0;
}

/**
* @brief Destructor del objeto Historico
*/
historico::~historico(){

}


tags * historico::obtener_tag(){
	return &t;
}

/**
* @brief Asigna la propiedad de tipo de muestreo al objeto historico
* @param tl		Tipo de muestreo que se realizar
*/
void historico::asignar_tipo(Tlog tl){
	tipo_log = tl;
}

/**
* @brief Asigna la propiedad de frecuencia de muestreo al objeto historico
* @param t		Frecuencia de muestreo
*/
void historico::asignar_tiempo(int t){
	tiempo_muestreo = (unsigned int)t;
}

void historico::asignar_maximo(float max){
	maximo = max;
}

void historico::asignar_minimo(float min){
	minimo = min;
}

/**
* @brief Asigna la Cantidad de Muestras a Recolectar
* @param m		Numero de Muestras
*/
void historico::asignar_muestras(int m){
	muestras = (unsigned int)m;
}

/**
* @brief Asigna las Funciones para el Manejo de las Muestras
* @param tf		Funciones
*/
void historico::asignar_funcion(Tfunciones tf){
	funciones[cant_funciones++] = tf;
}

/**
* @brief Reinicia el contador de Funciones
*/
void historico::reset_cant_funciones(){
	cant_funciones = 0;
}

/**
* @brief Copia de Historicos
* @param h		Valor Historico
*/
void historico::copiar(const historico & h){
	t.asignar_propiedades( &(h.t.nombre[0]), h.t.valor_campo, h.t.tipo, NULL, NULL );
	tipo_log = h.tipo_log;
	tiempo_muestreo = h.tiempo_muestreo;
	maximo = h.maximo;
	minimo = h.minimo;
	muestras = h.muestras;
	cant_funciones = h.cant_funciones;
	for(unsigned int i = 0; i < cant_funciones; i++)
		funciones[i] = h.funciones[i];
}

/**
* @brief Devuelve la Cantidad de Muestras
* @return muestras	Numero de Muestras
*/
unsigned int historico::obtener_muestras(){
	return muestras;
}

/**
* @brief Devuelve los Tiempos de Muestreo
* @return tiempo_muestreo	Frecuencia de Adquicision
*/
unsigned int historico::obtener_tiempo_muestreo(){
	return tiempo_muestreo;
}

/**
* @brief Devuelve el Tipo de Registro del Historico
* @return tipo_log		Tipo de Registro
*/
Tlog historico::obtener_tipo_log(){
	return tipo_log;
}

/**
* @brief Devuelve una Funcion de Manejo de Muestras
* @param	pos		Posicion de la Funcion dentro del Arreglo
* @return	funcion	Tipo de Funcion
*/
Tfunciones historico::obtener_funcion(unsigned int _pos){
	return funciones[_pos];
}

Tvalor tendencia::obtener_inicio_tendencia( struct timeval inicio, struct timeval fin,
										int & puntos, int & indice, struct timeval & fecha_pto ){
	size_t 		pos_inicio,
				pos_fin;

	pos_fin = posicion - 1;
	pos_fin = pos_fin < 0 ? _MAX_CANT_MUESTRAS_ - 1 : pos_fin;
	for(  ; fin.tv_sec <= fecha[pos_fin].tv_sec; pos_fin-- ){
		if( pos_fin == 0 ){
			pos_fin = _MAX_CANT_MUESTRAS_ - 1;
		}
		if( pos_fin == posicion ){
			puntos = 0;
			Tvalor temp;
			temp.L = _ERR_DATO_;
			fecha_pto.tv_sec = 0;
			fecha_pto.tv_usec = 0;
			return temp;
		}
	}

	pos_inicio = pos_fin - 1;
	pos_inicio = pos_inicio < 0 ? _MAX_CANT_MUESTRAS_ - 1 : pos_inicio;
	for( ; inicio.tv_sec <= fecha[pos_inicio].tv_sec; pos_inicio-- ){
		if( pos_inicio == 0 ){
			pos_inicio = _MAX_CANT_MUESTRAS_ - 1;
		}
		if( pos_inicio == pos_fin ){
			if( ++pos_inicio >= _MAX_CANT_MUESTRAS_ )
				pos_inicio = 0;
			break;
		}
	}

	puntos = pos_fin - pos_inicio;
	puntos = puntos < 0 ? puntos + _MAX_CANT_MUESTRAS_ : puntos;
	indice = pos_inicio;
	fecha_pto.tv_sec = fecha[pos_inicio].tv_sec;
	fecha_pto.tv_usec = fecha[pos_inicio].tv_usec;
	return punto[pos_inicio];
}

Tvalor tendencia::obtener_siguiente_punto( int & puntos, int & indice, struct timeval & fecha_pto ){
	indice++;
	puntos--;

	if( puntos > 0 ){
		if( indice >= _MAX_CANT_MUESTRAS_ )
			indice = 0;
		fecha_pto.tv_sec = fecha[indice].tv_sec;
		fecha_pto.tv_usec = fecha[indice].tv_usec;
		return punto[indice];
	}
	Tvalor temp;
	temp.L = _ERR_DATO_;
	fecha_pto.tv_sec = 0;
	fecha_pto.tv_usec = 0;
	return temp;
}


Tvalor * tendencia::obtener_punto( unsigned int _pos ){
	if( _pos >= _MAX_CANT_MUESTRAS_ || _pos < 0 )
		return NULL;
	return &punto[_pos];
}


void tendencia::agregar_historico( const historico & _h ){
	h.copiar( _h );
}

tags * tendencia::obtener_tag(){
	return h.obtener_tag();
}

bool tendencia::insertar_nuevo_punto( tags * _t, unsigned int _pos ){
	if( !_t ){
		_t = obtener_tag();
	}

	if( _pos >= _MAX_CANT_MUESTRAS_ )
		return false;

	struct timeval ahora;

	switch(_t->tipo){
		case CARAC_:
			punto[_pos].C = _t->valor_campo.C;
		break;
		case CORTO_:
			punto[_pos].S = _t->valor_campo.S;
		break;
		case ENTERO_:
			punto[_pos].I = _t->valor_campo.I;
		break;
		case LARGO_:
			punto[_pos].L = _t->valor_campo.L;
		break;
		case REAL_:
			punto[_pos].F = _t->valor_campo.F;
		break;
		case DREAL_:
			punto[_pos].D = _t->valor_campo.D;
		break;
		case BIT_:
			punto[_pos].B = _t->valor_campo.B;
		break;
		case TERROR_:
		default:
			punto[_pos].L = _ERR_DATO_;
	}
	punto[_pos+1].L = _ERR_DATO_;
	gettimeofday(&ahora,(struct timezone *)0);
	fecha[_pos].tv_sec = ahora.tv_sec;
	fecha[_pos].tv_usec = ahora.tv_usec;
	posicion = _pos + 1;

	return true;
}

unsigned int tendencia::obtener_posicion(){
	return posicion;
}

Tlog tendencia::obtener_tipo_log(){
	return h.obtener_tipo_log();
}

unsigned int tendencia::obtener_muestras(){
	return h.obtener_muestras();
}

struct timeval tendencia::obtener_fecha_punto( unsigned int _pos ){
	return fecha[_pos];
}

unsigned int tendencia::obtener_tiempo_muestreo(){
	return h.obtener_tiempo_muestreo();
}

char * tendencia::obtener_nombre_tag(){
	return h.t.obtener_nombre();
}

Tfunciones tendencia::obtener_funcion_historico( unsigned int _index ){
	return h.obtener_funcion(_index);
}

void tendencia::imprimir_tendencia(){
	time_t 				tiempo;
	tm 						*ltiempo;
	unsigned int	i,
								pos;

	for(i=0,pos=posicion; i < _MAX_CANT_MUESTRAS_; i++,pos++){
		if(pos>=_MAX_CANT_MUESTRAS_)
			pos = 0;
		tiempo = fecha[pos].tv_sec;
		ltiempo = localtime(&tiempo);
		cout << pos << "\t" <<
				numero2string<int>( ltiempo->tm_year+1900, std::dec ) << "-" <<
				numero2string<int>( ltiempo->tm_mon+1, std::dec ) << "-" <<
				numero2string<int>( ltiempo->tm_mday, std::dec ) << " " <<
				numero2string<int>( ltiempo->tm_hour, std::dec ) << ":" <<
				numero2string<int>( ltiempo->tm_min, std::dec ) << ":" <<
				numero2string<int>( ltiempo->tm_sec, std::dec ) << "." <<
				numero2string<int>( fecha[pos].tv_usec, std::dec ) << "\t\t" <<
				valor2texto(punto[pos],h.t.tipo_dato()) <<
				endl;
	}
}


bool alarmero::insertar_nueva_alarma( alarma * _a ){
	a = *_a;
	struct timeval ahora;
	gettimeofday(&ahora,(struct timezone *)0);
	fecha.tv_sec = ahora.tv_sec;
	fecha.tv_usec = ahora.tv_usec;
	return true;
}

alarma * alarmero::obtener_alarma(){
	return &a;
}

struct timeval alarmero::obtener_fecha_alarma(){
	return fecha;
}
