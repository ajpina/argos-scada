/**
* @file	as-thalarma.cpp
* @brief		<b>Tipo Abstracto de Datos para el Manejo
*				de Alarmas en una Tabla Hash con una Lista
*				Enlazada para el Manejo de Colisiones</b><br>
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

#include "as-thalarma.h"
#include <ctime>
#include <sys/time.h>

namespace argos{

/**
* @brief 	Crea un objeto TablaHash_Alarmas para Almacenar Alarmas
* @return 	referencia al objeto
*/
thalarma::thalarma(){
	crear_tabla(103,this);
}

/**
* @brief 	Crea un objeto TablaHash_Alarmas para Almacenar Alarmas
* @param	s	Cantidad de Alarmas
* @return 	referencia al objeto
*/
thalarma::thalarma(unsigned int s){
	crear_tabla(s,this);
}

/**
* @brief 	Destructor del objeto
*/
thalarma::~thalarma(){
}

/**
* @brief	Sobrecarga del operador NEW
* @param	n_bytes		Espacio Solicitado
* @param	direccion	Direccion en Memoria Compartida
* @return	direccion	Direccion en Memoria Compartida
*/
void * thalarma::operator new(size_t n_bytes, void *direccion){
	return direccion;
}

/**
* @brief 	Crea la Tabla Hash de Alarmas, cada renglon
*			es una Lista Enlazada
* @param	s			Cantidad de Alarmas a Almacenar
* @param	inicio		Direccion de Inicio de la Memoria Compartida
* @return 	1			Creacion Satisfactoria
*/
int thalarma::crear_tabla(unsigned int s, void *inicio){
	pthread_mutexattr_t recursivo;
	pthread_mutexattr_init(&recursivo);
	pthread_mutexattr_settype(&recursivo, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex, &recursivo);

	pthread_mutex_lock(&mutex);
	ptr_tablaRT = ptr_mem_disponible = (void *)(sizeof(thalarma));
	try{
		tablaRT = new((void *)((intptr_t)inicio + (intptr_t)ptr_mem_disponible))lista[s];

	}catch(std::bad_alloc&){
		cout << "Memoria Insuficiente";
	}
	ptr_mem_disponible = (void *)((intptr_t)ptr_mem_disponible + ((s+1)*sizeof(lista)));
	tamano_tabla = (unsigned int)s;
	pthread_mutex_unlock(&mutex);
	cantidad_alarmas = colisiones = 0;
	return 1;
}

/**
* @brief 	Agregar Alarmas a la Tabla Hash
* @param	t			Alarma
* @param	inicio		Direccion de Inicio de la Memoria Compartida
* @return 	1			Insercion Satisfactoria
*/
int thalarma::insertar_alarma(alarma t, void *inicio){
	unsigned int key = funcion_hash(t);
	pthread_mutex_lock(&mutex);
	lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
	colisiones += !((*(ptr_lista + key)).lista_vacia()) ? 1 : 0;
	cantidad_alarmas += (*(ptr_lista + key)).insertar(&t, (void *)((intptr_t)inicio + (intptr_t)ptr_mem_disponible), inicio);
	ptr_mem_disponible = (void *)((intptr_t)ptr_mem_disponible + sizeof(alarma));
	pthread_mutex_unlock(&mutex);
	return 1;
}

/**
* @brief 	Imprime por pantalla el contenido de la
*			Tabla Hash
* @param	inicio		Direccion de Inicio de la Memoria Compartida
*/
void thalarma::imprimir_tabla(void *inicio){
	struct timeval cTime1, cTime2;
	gettimeofday( &cTime1, (struct timezone*)0 );

	lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
	cout << "——————————————————————————————————————————————————" << endl;
	cout <<"\033[1;" << 34 << "mFila    Nombre                         Estado\033[0m" << endl;
        cout << "——————————————————————————————————————————————————" << endl;

	for(unsigned int i = 0; i<tamano_tabla; i++){
		if(!(*(ptr_lista + i)).lista_vacia()){
			cout <<"  "<< i << "     ";
			(*(ptr_lista + i)).imprimir_lista(inicio);
			cout << "——————————————————————————————————————————————————" << endl;			
		}
	}
	cout << endl << "Tamano Tabla -> " << tamano_tabla << endl
	     << "Colisiones -> " << colisiones << endl << "Alarmas -> " << cantidad_alarmas << endl;

	gettimeofday( &cTime2, (struct timezone*)0 );
    	if(cTime1.tv_usec > cTime2.tv_usec){
    	cTime2.tv_usec = 1000000;
    	--cTime2.tv_usec;
	}
	printf("Tiempo que tardo: %ld s, %ld us. \n", cTime2.tv_sec - cTime1.tv_sec, cTime2.tv_usec - cTime1.tv_usec );

}

/**
* @brief 	Modifica la Direccion de Referencia para la Tabla Hash
*			de Alarmas
* @param	inicio			Direccion de Inicio de la Memoria Compartida
* @param	nueva_th		Direccion de Tabla Hash
* @param	nuevo_inicio	Nueva Direccion de la Memoria Compartida
*/
void thalarma::actualizar_tablahash(void *inicio, thalarma * nueva_th, void * nuevo_inicio){
	lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
	alarma * tmp;
	for(unsigned int i = 0; i < tamano_tabla; i++){
		if(!(*(ptr_lista + i)).lista_vacia()){
			tmp = (*(ptr_lista + i)).cabeza(inicio);
			for(unsigned int j = 0; j < (*(ptr_lista + i)).cantidad; j++){
				nueva_th->buscar_alarma(tmp, nuevo_inicio);
				if(tmp->ptr_proximo()){
					tmp = (alarma *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);

				}
				else
					break;
			}
		}
	}
}

/**
* @brief 	Copia las Alarmas desde la Tabla Hash a un Arreglo
* @param	arreglo			Destino
* @param 	tamano_arreglo	Tamano del Arreglo
* @param	inicio			Direccion de Inicio de la Memoria Compartida
*/
void thalarma::copiar_alarmas_a_arreglo( alarma * arreglo,unsigned int * tamano_arreglo, void * inicio ){
	lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
	unsigned int count = 0;
	alarma * tmp;
	for(unsigned int i = 0; i < tamano_tabla && count < (*tamano_arreglo); i ++){
		if(!(*(ptr_lista + i)).lista_vacia()){
			tmp = (*(ptr_lista + i)).cabeza(inicio);
			arreglo[count++].asignar_propiedades(tmp->id_grupo, tmp->grupo,
						tmp->id_subgrupo, tmp->subgrupo, tmp->nombre, tmp->estado,
						tmp->valor_comparacion, tmp->tipo_comparacion, tmp->tipo_origen);
			for(unsigned int j = 0; j < (*(ptr_lista + i)).cantidad; j++){
				if( tmp->ptr_proximo() ){
					tmp = (alarma *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);
					arreglo[count++].asignar_propiedades(tmp->id_grupo, tmp->grupo,
						tmp->id_subgrupo, tmp->subgrupo, tmp->nombre, tmp->estado,
						tmp->valor_comparacion, tmp->tipo_comparacion, tmp->tipo_origen);
				}
			}
		}
	}
	*tamano_arreglo = count;
}

/**
* @brief 	Copia las Alarmas desde la Tabla Hash a un Arreglo
* @param	arreglo			Destino
* @param 	tamano_arreglo	Tamano del Arreglo
* @param	inicio			Direccion de Inicio de la Memoria Compartida
*/
void thalarma::copiar_alarmas_a_arreglo_portable( Talarmas_portable * arreglo,unsigned int * tamano_arreglo, void * inicio ){
	lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
	unsigned int count = 0;
	alarma * tmp;
	for(unsigned int i = 0; i < tamano_tabla && count < (*tamano_arreglo); i ++){
		if(!(*(ptr_lista + i)).lista_vacia()){
			tmp = (*(ptr_lista + i)).cabeza(inicio);
			strcpy(arreglo[count].nombre, tmp->nombre);
			arreglo[count].estado = tmp->estado;
			arreglo[count++].tipo_origen = tmp->tipo_origen;
			for(unsigned int j = 0; j < (*(ptr_lista + i)).cantidad; j++){
				if( tmp->ptr_proximo() ){
					tmp = (alarma *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);
					strcpy(arreglo[count].nombre, tmp->nombre);
                    arreglo[count].estado = tmp->estado;
                    arreglo[count++].tipo_origen = tmp->tipo_origen;
				}
			}
		}
	}
	*tamano_arreglo = count;
}


}
