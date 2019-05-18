/**
* @file	as-shmhistorico.cpp
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


#include "as-shmhistorico.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <cmath>
#include <cstdio>

namespace argos{


/**
* @brief 	Crea un objeto SHM_Tendencia y SHM_Alarmero
*					para Almacenar Tags y Alarmas
* @return 	referencia al objeto
*/
shm_historico::shm_historico(std::string t, std::string a){
	inicializar_shm_tendencia(t);
	inicializar_shm_alarmero(a);
}


/**
* @brief 	Destructor del objeto
*/
shm_historico::~shm_historico(){
	if( -1 != fd_t ){
		cerrar_shm_tendencia();
	}
	if( -1 != fd_a ){
		cerrar_shm_alarmero();
	}
	if(mi_segmento_inicio_t)
		delete mi_segmento_inicio_t;
	if(mi_segmento_inicio_a)
		delete mi_segmento_inicio_a;
}


/**
* @brief 	Inicializa los atributos del objeto SHM_Tendencia
* @param	t			Nombre del Segmento de Memoria
* @return 	verdadero	Actualizacion Satisfacoria
*/
bool shm_historico::inicializar_shm_tendencia(std::string t){
	tamano_bloque_t = 0;
	nombre_tendencia = t;
	ptr_tab_tendencia = NULL;
	fd_t = -1;
	mi_segmento_inicio_t = new unsigned long;
	return true;
}

/**
* @brief 	Inicializa los atributos del objeto SHM_Alarmero
* @param	a			Nombre del Segmento de Memoria
* @return 	verdadero	Actualizacion Satisfacoria
*/
bool shm_historico::inicializar_shm_alarmero(std::string a){
	tamano_bloque_a = 0;
	nombre_alarmero = a;
	ptr_tab_alarmero = NULL;
	fd_a = -1;
	mi_segmento_inicio_a = new unsigned long;
	return true;
}


/**
* @brief 	Utiliza una direccion de segmento unica para
*			que todas las instancias del objeto compartan
*			la misma memoria compartida
* @param	mi_si		Direccion del Segmento
*/
void shm_historico::asignar_almacenamiento_segmento_inicio(void * mi_si_t, void * mi_si_a){
	if(mi_si_t)
		*mi_segmento_inicio_t = (unsigned long)mi_si_t;
	if(mi_si_a)
		*mi_segmento_inicio_a = (unsigned long)mi_si_a;
}


/**
* @brief 	Actualiza el apuntador a la Tabla Hash
* @param	dirrecion	Direccion Tabla Hash
*/
void shm_historico::asignar_ref_historico(void * direccion_t, void * direccion_a ){
	ptr_tab_tendencia->tend = (tendencia *)direccion_t;
	ptr_tab_alarmero->alarm = (alarmero *)direccion_a;
	asignar_almacenamiento_segmento_inicio(direccion_t, direccion_a);
	//unsigned int ttabla = th->obtener_tamano_tabla();
	//tamano_bloque = (unsigned int)(sizeof(thtag) + ttabla*(thtag::sizeof_lista()) + ttabla*sizeof(tags));
}


/**
* @brief 	Devuelve el tamano del Segmento de Memoria Compartida
* @return 	tamano_bloque	Tamano en Bytes
*/
unsigned int shm_historico::sizeof_bloque_tendencia(){
	return tamano_bloque_t;
}


/**
* @brief 	Devuelve el tamano del Segmento de Memoria Compartida
* @return 	tamano_bloque	Tamano en Bytes
*/
unsigned int shm_historico::sizeof_bloque_alarmero(){
	return tamano_bloque_a;
}


/**
* @brief 	Crea la Memoria Compartida para el Buffer
*			de Tags
* @param	cant		Cantidad de Tags a Almacenar
* @return 	verdadero	Creacion Satisfactoria
*/
bool shm_historico::crear_shm_tendencia(unsigned int cant){
	tamano_bloque_t = (unsigned int)(sizeof(Ttab_tendencia) + cant*sizeof(tendencia));
	fd_t = shm_open(nombre_tendencia.c_str(), O_CREAT|O_RDWR, S_IRWXU);
	if( -1 == fd_t ){
		perror("Abriendo Memoria Compartida:");
		return false;
	}
	if(ftruncate(fd_t,tamano_bloque_t)<0){
		perror("Dimensionando Memoria Compartida:");
		return false;
	}
	ptr_tab_tendencia = (Ttab_tendencia *)mmap(0,tamano_bloque_t,PROT_READ|PROT_WRITE,MAP_SHARED,fd_t,(long)0);
	if( NULL == ptr_tab_tendencia ){
		perror("Mapeando Memoria Compartida:");
		return false;
	}
//	asignar_almacenamiento_segmento_inicio(ptr_tab_tendencia, NULL);
//	ptr_tab_tendencia->tend = (tendencia *)((intptr_t)ptr_tab_tendencia + sizeof(ptr_tab_tendencia->cantidad));
	asignar_almacenamiento_segmento_inicio((void *)((intptr_t)ptr_tab_tendencia + sizeof(ptr_tab_tendencia->cantidad)), NULL);
	//ptr_tab_tendencia->tend = new ( (void *)((intptr_t)ptr_tab_tendencia + sizeof(ptr_tab_tendencia->cantidad)) )tendencia[cant];
	//ptr_tab_tendencia->tend = new ( (void *)(ptr_tab_tendencia))tendencia[cant];
	//asignar_almacenamiento_segmento_inicio(ptr_tab_tendencia->tend, NULL);
	ptr_tab_tendencia->cantidad = cant;
	return true;
}


/**
* @brief 	Crea la Memoria Compartida para el Buffer
*			de Alarmas
* @param	cant		Cantidad de Alarmas a Almacenar
* @return 	verdadero	Creacion Satisfactoria
*/
bool shm_historico::crear_shm_alarmero(unsigned int cant){
	tamano_bloque_a = (unsigned int)(sizeof(Ttab_alarmero) + cant*sizeof(alarmero));
	fd_a = shm_open(nombre_alarmero.c_str(), O_CREAT|O_RDWR, S_IRWXU);
	if( -1 == fd_a ){
		perror("Abriendo Memoria Compartida:");
		return false;
	}
	if(ftruncate(fd_a,tamano_bloque_a)<0){
		perror("Dimensionando Memoria Compartida:");
		return false;
	}
	ptr_tab_alarmero = (Ttab_alarmero *)mmap(0,tamano_bloque_a,PROT_READ|PROT_WRITE,MAP_SHARED,fd_a,(long)0);
	if( NULL == ptr_tab_alarmero ){
		perror("Mapeando Memoria Compartida:");
		return false;
	}
	asignar_almacenamiento_segmento_inicio(NULL, (void *)((intptr_t)ptr_tab_alarmero + sizeof(ptr_tab_alarmero->buf_len)));
	//ptr_tab_alarmero->alarm = new ( (void *)((intptr_t)ptr_tab_alarmero + sizeof(ptr_tab_alarmero->buf_len)) )alarmero[cant];
	ptr_tab_alarmero->buf_len = cant;
	return true;
}


/**
* @brief 	Obtener acceso a una Memoria Compartida previamente
*			creada
* @return 	verdadero	Apertura Satisfactoria
*/
bool shm_historico::obtener_shm_tendencia(){
	fd_t = shm_open(nombre_tendencia.c_str(), O_CREAT|O_RDWR, S_IRWXU);
	if( -1 == fd_t ){
		perror("Abriendo Memoria Compartida:");
		return false;
	}
	Ttab_tendencia * tmp = (Ttab_tendencia *)mmap(0,sizeof(Ttab_tendencia),PROT_READ|PROT_WRITE,MAP_SHARED,fd_t,(long)0);
	if( NULL == tmp ){
		perror("Mapeando Memoria Compartida para obtener tamano:");
		return false;
	}
	unsigned int tam_tabla = tmp->cantidad;
	munmap(tmp,sizeof(Ttab_tendencia));
	tamano_bloque_t = (unsigned int)(sizeof(Ttab_tendencia) + tam_tabla*sizeof(tendencia));
	ptr_tab_tendencia = (Ttab_tendencia *)mmap(0,tamano_bloque_t,PROT_READ|PROT_WRITE,MAP_SHARED,fd_t,(long)0);
	if( NULL == ptr_tab_tendencia ){
		perror("Mapeando Memoria Compartida:");
		return false;
	}

	asignar_almacenamiento_segmento_inicio((void *)((intptr_t)ptr_tab_tendencia + sizeof(ptr_tab_tendencia->cantidad)), NULL);
	//ptr_tab_tendencia->tend = (tendencia *)((intptr_t)ptr_tab_tendencia + sizeof(ptr_tab_tendencia->cantidad));
	return true;
}


/**
* @brief 	Obtener acceso a una Memoria Compartida previamente
*			creada
* @return 	verdadero	Apertura Satisfactoria
*/
bool shm_historico::obtener_shm_alarmero(){
	fd_a = shm_open(nombre_alarmero.c_str(), O_CREAT|O_RDWR, S_IRWXU);
	if( -1 == fd_a ){
		perror("Abriendo Memoria Compartida:");
		return false;
	}
	Ttab_alarmero * tmp = (Ttab_alarmero *)mmap(0,sizeof(Ttab_alarmero),PROT_READ|PROT_WRITE,MAP_SHARED,fd_a,(long)0);
	if( NULL == tmp ){
		perror("Mapeando Memoria Compartida para obtener tamano:");
		return false;
	}
	unsigned int tam_tabla = tmp->buf_len;
	munmap(tmp,sizeof(Ttab_alarmero));
	tamano_bloque_a = (unsigned int)(sizeof(Ttab_alarmero) + tam_tabla*sizeof(alarmero));
	ptr_tab_alarmero = (Ttab_alarmero *)mmap(0,tamano_bloque_a,PROT_READ|PROT_WRITE,MAP_SHARED,fd_a,(long)0);
	if( NULL == ptr_tab_alarmero ){
		perror("Mapeando Memoria Compartida:");
		return false;
	}
	asignar_almacenamiento_segmento_inicio(NULL,(void *)((intptr_t)ptr_tab_alarmero + sizeof(ptr_tab_alarmero->buf_len)));
	return true;
}


/**
* @brief 	Cierra la Memoria Compartida
* @return 	verdadero	Cierre Satisfactorio
*/
bool shm_historico::cerrar_shm_tendencia(){
	close(fd_t);
	return true;
}


/**
* @brief 	Cierra la Memoria Compartida
* @return 	verdadero	Cierre Satisfactorio
*/
bool shm_historico::cerrar_shm_alarmero(){
	close(fd_a);
	return true;
}


/**
* @brief 	Imprime por pantalla el contenido de la
*			Tabla Hash
*/
void shm_historico::imprimir_shm_tendencia(){
	tendencia * tmp;
	for(int i = 0; i < ptr_tab_tendencia->cantidad; i++){
		tmp = (tendencia *)(*mi_segmento_inicio_t);
		tmp = tmp + i;
		if(tmp){
			cout << "_________________________________________________________________" << endl;
			cout << tmp->h << endl;
			cout << "\033[1;" << 34 << "mPos\t\tFecha\t\t\tValor\033[0m" << endl;
			cout << "_________________________________________________________________" << endl;
			tmp->imprimir_tendencia();
			cout << "_________________________________________________________________\n" << endl;
		}

	}
}


/**
* @brief 	Imprime por pantalla el contenido de la
*			Tabla Hash
*/
void shm_historico::imprimir_shm_alarmero(){

}

tendencia * shm_historico::obtener_tendencia(unsigned int _pos){
	if( _pos < 0 || _pos > ptr_tab_tendencia->cantidad ){
		perror("Indice invalido");
		return NULL;
	}
	tendencia * tmp = (tendencia *)(*mi_segmento_inicio_t);
	return (tendencia *)(tmp + _pos);
}

alarmero * shm_historico::obtener_alarmero(unsigned int _pos){
	if( _pos < 0 || _pos > ptr_tab_alarmero->buf_len ){
		perror("Indice invalido");
		return NULL;
	}
	alarmero * tmp = (alarmero *)(*mi_segmento_inicio_a);
	return (alarmero *)(tmp + _pos);
}





}
