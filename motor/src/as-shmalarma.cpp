/**
* @file	as-shmalarma.cpp
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

#include "as-shmalarma.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <cmath>
#include <cstdio>


namespace argos{

/**
* @brief 	Crea un objeto SHM_TablaHash_Alarmas para Almacenar Alarmas
* @return 	referencia al objeto
*/
shm_thalarma::shm_thalarma(string n){
	inicializar_shm_thalarma( n );
}

/**
* @brief 	Destructor del objeto
*/
shm_thalarma::~shm_thalarma(){
	if( -1 != fd )
		cerrar_shm_tablahash();
	if(mi_segmento_inicio)
		delete mi_segmento_inicio;
}

/**
* @brief 	Devuelve el tamano del Segmento de Memoria Compartida
* @return 	tamano_bloque	Tamano en Bytes
*/
unsigned int shm_thalarma::sizeof_bloque(){
	return tamano_bloque;
}

/**
* @brief 	Inicializa los atributos del objeto SHM_TablaHash_Alarmas
* @param	n			Nombre del Segmento de Memoria
* @return 	verdadero	Actualizacion Satisfacoria
*/
bool shm_thalarma::inicializar_shm_thalarma(std::string n){
	tamano_bloque = 0;
	nombre = n;
	th = NULL;
	fd = -1;
	mi_segmento_inicio = new unsigned long;
	return true;
}

/**
* @brief 	Crea la Memoria Compartida para la Tabla Hash
*			de Alarmas
* @param	cant		Cantidad de Alarmas a Almacenar
* @return 	verdadero	Creacion Satisfactoria
*/
bool shm_thalarma::crear_shm_tablahash(unsigned int cant){
	cant = (unsigned int)ceil(cant*1.3);
	primo_mayor_que(cant);
	tamano_bloque = (unsigned int)(sizeof(thalarma) + cant*(thalarma::sizeof_lista()) + cant*sizeof(alarma));
	fd = shm_open(nombre.c_str(), O_CREAT|O_RDWR, S_IRWXU);
	if( -1 == fd ){
		perror("Abriendo Memoria Compartida:");
		return false;
	}
	if(ftruncate(fd,tamano_bloque)<0){
		perror("Dimensionando Memoria Compartida:");
		return false;
	}
	th = (thalarma *)mmap(0,tamano_bloque,PROT_READ|PROT_WRITE,MAP_SHARED,fd,(long)0);
	if( NULL == th ){
		perror("Mapeando Memoria Compartida:");
		return false;
	}
	asignar_almacenamiento_segmento_inicio(th);
	th = new (th)thalarma(cant);
	return true;
}

/**
* @brief 	Utiliza una direccion de segmento unica para
*			que todas las instancias del objeto compartan
*			la misma memoria compartida
* @param	mi_si		Direccion del Segmento
*/
void shm_thalarma::asignar_almacenamiento_segmento_inicio(void * mi_si){
	*mi_segmento_inicio = (unsigned long)mi_si;
}

/**
* @brief 	Actualiza el apuntador a la Tabla Hash
* @param	dirrecion	Direccion Tabla Hash
*/
void shm_thalarma::asignar_ref_tablahash(void * direccion){
	th = (thalarma *)direccion;
	asignar_almacenamiento_segmento_inicio(direccion);
	unsigned int ttabla = th->obtener_tamano_tabla();
	tamano_bloque = (unsigned int)(sizeof(thalarma) + ttabla*(thalarma::sizeof_lista()) + ttabla*sizeof(alarma));
}

/**
* @brief 	Obtener acceso a una Memoria Compartida previamente
*			creada
* @return 	verdadero	Apertura Satisfactoria
*/
bool shm_thalarma::obtener_shm_tablahash(){
	fd = shm_open(nombre.c_str(), O_CREAT|O_RDWR, S_IRWXU);
	if( -1 == fd ){
		perror("Abriendo Memoria Compartida:");
		return false;
	}
	thalarma * tmp = (thalarma *)mmap(0,sizeof(thalarma),PROT_READ|PROT_WRITE,MAP_SHARED,fd,(long)0);
	if( NULL == tmp ){
		perror("Mapeando Memoria Compartida para obtener tamano:");
		return false;
	}
	unsigned int tam_tabla = tmp->obtener_tamano_tabla();
	munmap(tmp,sizeof(thalarma));
	tamano_bloque = (unsigned int)(sizeof(thalarma) + tam_tabla*(thalarma::sizeof_lista()) + tam_tabla*sizeof(alarma));
	th = (thalarma *)mmap(0,tamano_bloque,PROT_READ|PROT_WRITE,MAP_SHARED,fd,(long)0);
	if( NULL == th ){
		perror("Mapeando Memoria Compartida:");
		return false;
	}
	asignar_almacenamiento_segmento_inicio(th);
	return true;
}

/**
* @brief 	Cierra la Memoria Compartida
* @return 	verdadero	Cierre Satisfactorio
*/
bool shm_thalarma::cerrar_shm_tablahash(){
	close(fd);
	return true;
}

/**
* @brief 	Agregar Alarmas a la Tabla Hash
* @param	t			Alarma
* @return 	verdadero	Insercion Satisfactoria
*/
bool shm_thalarma::insertar_alarma(alarma t){
	th->insertar_alarma(t, (void *)(*mi_segmento_inicio));
	return true;
}

/**
* @brief 	Buscar Alarmas en la Tabla Hash
* @param	t			Alarma
* @return 	verdadero	Encontrada
*/
bool shm_thalarma::buscar_alarma(alarma * t){
	if(!th)
		return false;
	return th->buscar_alarma(t, (void *)(*mi_segmento_inicio));
}

/**
* @brief 	Buscar Alarmas en la Tabla Hash
* @param	nombre		Nombre Alarma
* @return 	alarma		Alarma Encontrada
*/
alarma * shm_thalarma::buscar_alarma( std::string nombre ){
	alarma 		a;
	Tvalor		val;
	char		char_nombre[_LONG_NOMB_TAG_];

	if(!th)
		return NULL;
	strcpy( char_nombre, nombre.c_str() );
	a.asignar_propiedades(0,"singrupo",0,"sinsubgrupo",
					char_nombre,BAJO_,val,MAYOR_QUE_,ENTERO_);
	return th->obtener_ptr_alarma(&a, (void *)(*mi_segmento_inicio) );
}

/**
* @brief 	Actualiza el Estado de la Alarma en la Tabla Hash
* @param	t			Alarma
* @return 	verdadero	Actualizacion Satisfactoria
*/
bool shm_thalarma::actualizar_alarma(alarma * t){
	return th->actualizar_alarma(t, (void *)(*mi_segmento_inicio));
}

/**
* @brief 	Elimina la Alarma en la Tabla Hash
* @todo		Implementar Eliminacion de Alarmas
* @param	t			Alarma
* @return 	verdadero	Eliminacion Satisfactoria
*/
bool shm_thalarma::borrar_alarma(alarma t){
	return false;
}

/**
* @brief 	Imprime por pantalla el contenido de la
*			Tabla Hash
*/
void shm_thalarma::imprimir_shm_tablahash(){
	th->imprimir_tabla((void *)(*mi_segmento_inicio));
	cout << "Bytes -> " << tamano_bloque << endl;
}

/**
* @brief 	Modifica la Direccion de Referencia para la Tabla Hash
*			de Alarmas
* @param	nueva_th	Direccion de Tabla Hash
* @param	ni			Direccion de Memoria Compartida
*/
void shm_thalarma::actualizar_shm_tablahash(thalarma * nueva_th, void * ni){
	th->actualizar_tablahash((void *)(*mi_segmento_inicio), nueva_th, ni);
}

/**
* @brief 	Crea un Arreglo de Alarmas desde la Tabla Hash
* @param	arreglo			Destino
* @param 	tamano_arreglo	Tamano del Arreglo
*/
void shm_thalarma::obtener_alarmas(alarma * arreglo, unsigned int * tamano_arreglo ){
	th->copiar_alarmas_a_arreglo( arreglo,tamano_arreglo,(void *)(*mi_segmento_inicio));
}

/**
* @brief 	Crea un Arreglo de Alarmas Portables desde la Tabla Hash
* @param	arreglo			Destino
* @param 	tamano_arreglo	Tamano del Arreglo
*/
void shm_thalarma::obtener_alarmas_portable(Talarmas_portable * arreglo, unsigned int * tamano_arreglo ){
	th->copiar_alarmas_a_arreglo_portable( arreglo,tamano_arreglo,(void *)(*mi_segmento_inicio));
}

}
