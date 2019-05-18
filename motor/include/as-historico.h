/**
* @file	as-historico.h
* @class historico
* @class tendencia
* @class alarmero
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

#ifndef HISTORICO_H_
#define HISTORICO_H_

#include <sys/time.h>
#include "as-util.h"
#include "as-tags.h"
#include "as-alarma.h"



namespace argos{

class historico{

private:
	Tlog tipo_log;								//!< Muestreo o Eventos
	unsigned int tiempo_muestreo;				//!< Tiempo entre Muestreos
	float maximo;								//!<
	float minimo;								//!<
	unsigned int muestras;						//!< Cantidad de Muestras
	Tfunciones funciones[_MAX_CANT_FUNCIONES_];	//!< Funciones para interpolacion
	unsigned int cant_funciones;				//!< Cantidad de Funciones aplicables

public:
	tags t;										//!< Tag a muestrear
	historico();
	~historico();

	tags * obtener_tag();

/*--------------------------------------------------------
	Asignar el Tipo de Registro (Muestreo o Por Evento)
----------------------------------------------------------*/
	void asignar_tipo(Tlog);

/*--------------------------------------------------------
	Asignar el Tiempo de Muestreo
----------------------------------------------------------*/
	void asignar_tiempo(int);

	void asignar_maximo(float);
	void asignar_minimo(float);

/*--------------------------------------------------------
	Asignar Cantidad de Muestras
----------------------------------------------------------*/
	void asignar_muestras(int);

/*--------------------------------------------------------
	Asignar Funcion para el Manejo de las Muestras
----------------------------------------------------------*/
	void asignar_funcion(Tfunciones);
	void reset_cant_funciones();

/*--------------------------------------------------------
	Funcion de copia
----------------------------------------------------------*/
	void copiar(const historico &);

/*--------------------------------------------------------
	Devolver Muestras y Tiempo de Muestreo del Historico
----------------------------------------------------------*/
	unsigned int obtener_muestras();
	unsigned int obtener_tiempo_muestreo();

/*--------------------------------------------------------
	Devolver el tipo de registro para los datos
----------------------------------------------------------*/
	Tlog obtener_tipo_log();

/*--------------------------------------------------------
	Devolver las Funciones de Manejo
----------------------------------------------------------*/
	Tfunciones obtener_funcion(unsigned int);

/*--------------------------------------------------------
	Operadores de Comparacion entre datos historicos
	por nombre del Tag
----------------------------------------------------------*/
	int operator == (const historico & der) const{
		return strcmp(t.nombre,der.t.nombre) == 0;
	}

	int operator < (const historico & der) const{
		return strcmp(t.nombre,der.t.nombre) < 0;
	}

	int operator > (const historico & der) const{
		return strcmp(t.nombre,der.t.nombre) > 0;
	}

/*--------------------------------------------------------
	Operador de Salida para los Datos Historicos
----------------------------------------------------------*/
	friend ostream& operator << (ostream & os, const historico & h){
		os 	<< "\033[1;" << 34 << "mNombre\t\t\t\tValor\t\t\tTipo\033[0m\n"
				<< h.t << "\n"
				<< "\033[1;" << 34 << "mTipo de LOG:\033[0m " << log2texto(h.tipo_log) << "\n"
				<< "\033[1;" << 34 << "mTiempo:\033[0m " << h.tiempo_muestreo << "ms\n"
				<< "\033[1;" << 34 << "mValor Maximo:\033[0m " << h.maximo << "\n"
				<< "\033[1;" << 34 << "mValor Minimo:\033[0m " << h.minimo << "\n"
				<< "\033[1;" << 34 << "mCantidad de Muestras:\033[0m " << h.muestras << "\n"
				<< "\033[1;" << 34 << "mTipo de Funcion:\033[0m " << func2texto(h.funciones[0]) <<
			"\033[0m"  << endl;
		return os;
	}

};

/*--------------------------------------------------------
	Estructura de Datos para los Valores Historicos
----------------------------------------------------------*/
class tendencia{
public:
	historico h;								//!< Dato historico
	Tvalor punto[_MAX_CANT_MUESTRAS_];			//!< Arreglo con las Muestras
	struct timeval fecha[_MAX_CANT_MUESTRAS_];	//!< Fecha de Adquisicion
	unsigned int posicion;						//!< Posicion dentro del Buffer Circular



/**
* @brief	Sobrecarga del operador NEW
* @param	n_bytes		Espacio Solicitado
* @param	direccion	Direccion en Memoria Compartida
* @return	direccion	Direccion en Memoria Compartida
*/
	void * operator new[](size_t n_bytes, void *direccion){
		return direccion;
	}

	void operator delete(void *){}

	Tvalor obtener_inicio_tendencia( struct timeval, struct timeval, int &, int &, struct timeval & );
	Tvalor obtener_siguiente_punto( int &, int &, struct timeval & );
	Tvalor * obtener_punto( unsigned int );

	void agregar_historico( const historico & );
	tags * obtener_tag();
	bool insertar_nuevo_punto( tags *, unsigned int );
	unsigned int obtener_posicion();
	Tlog obtener_tipo_log();
	unsigned int obtener_muestras();
	struct timeval obtener_fecha_punto( unsigned int );
	unsigned int obtener_tiempo_muestreo();
	char * obtener_nombre_tag();
	Tfunciones obtener_funcion_historico(unsigned int);
	void imprimir_tendencia();

};


/*--------------------------------------------------------
	Estructura de Datos para el Almacenamiento de Alarmas
----------------------------------------------------------*/
class alarmero{
public:
	alarma	a;									//!< Alarma
	struct timeval fecha;						//!< Fecha de Adquisicion



/**
* @brief	Sobrecarga del operador NEW
* @param	n_bytes		Espacio Solicitado
* @param	direccion	Direccion en Memoria Compartida
* @return	direccion	Direccion en Memoria Compartida
*/
	void * operator new[](size_t n_bytes, void *direccion){
		return direccion;
	}

	void operator delete(void *){}

	bool insertar_nueva_alarma( alarma * );
	alarma * obtener_alarma();
	struct timeval obtener_fecha_alarma();

};

}

#endif /*SSHISTORICO_H_*/
