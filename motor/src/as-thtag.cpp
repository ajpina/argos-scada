/**
* @file	as-thtag.cpp
* @brief		<b>Tipo Abstracto de Datos para el Manejo
*				de Tags en una Tabla Hash con una Lista
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

#include "as-thtag.h"
#include <ctime>
#include <sys/time.h>

namespace argos{

/**
* @brief 	Crea un objeto TablaHash_Tags para Almacenar Tags
* @return 	referencia al objeto
*/
thtag::thtag(){
	crear_tabla(103,this);
}

/**
* @brief 	Crea un objeto TablaHash_Tags para Almacenar Tags
* @param	s	Cantidad de Tags
* @return 	referencia al objeto
*/
thtag::thtag(unsigned int s){
	crear_tabla(s,this);
}

/**
* @brief 	Destructor del objeto
*/
thtag::~thtag(){
}

/**
* @brief	Sobrecarga del operador NEW
* @param	n_bytes		Espacio Solicitado
* @param	direccion	Direccion en Memoria Compartida
* @return	direccion	Direccion en Memoria Compartida
*/
void * thtag::operator new(size_t n_bytes, void *direccion){
	return direccion;
}

/**
* @brief 	Crea la Tabla Hash de Tags, cada renglon
*			es una Lista Enlazada
* @param	s			Cantidad de Tags a Almacenar
* @param	inicio		Direccion de Inicio de la Memoria Compartida
* @return 	1			Creacion Satisfactoria
*/
int thtag::crear_tabla(unsigned int s, void *inicio){
	pthread_mutexattr_t recursivo;
	pthread_mutexattr_init(&recursivo);
	pthread_mutexattr_settype(&recursivo, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex, &recursivo);

	pthread_mutex_lock(&mutex);
	ptr_tablaRT = ptr_mem_disponible = (void *)(sizeof(thtag));
	try{

		tablaRT = new((void *)((intptr_t)inicio + (intptr_t)ptr_mem_disponible))lista[s];

	}catch(std::bad_alloc&){
		cout << "Memoria Insuficiente";
	}

	ptr_mem_disponible = (void *)((intptr_t)ptr_mem_disponible + ((s+1)*sizeof(lista)));
	tamano_tabla = (unsigned int)s;
	pthread_mutex_unlock(&mutex);
	cantidad_tags = colisiones = 0;
	return 1;
}

/**
* @brief 	Agregar Tags a la Tabla Hash
* @param	t			Tag
* @param	inicio		Direccion de Inicio de la Memoria Compartida
* @return 	1			Insercion Satisfactoria
*/
int thtag::insertar_tag(tags t, void *inicio){
	unsigned int key = funcion_hash(t);
	pthread_mutex_lock(&mutex);

	lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
	colisiones += !((*(ptr_lista + key)).lista_vacia()) ? 1 : 0;
	cantidad_tags += (*(ptr_lista + key)).insertar(&t, (void *)((intptr_t)inicio + (intptr_t)ptr_mem_disponible), inicio);
	ptr_mem_disponible = (void *)((intptr_t)ptr_mem_disponible + sizeof(tags));
	pthread_mutex_unlock(&mutex);
	return 1;
}

/**
* @brief 	Imprime por pantalla el contenido de la
*			Tabla Hash
* @param	inicio		Direccion de Inicio de la Memoria Compartida
*/
void thtag::imprimir_tabla(void *inicio){
	struct timeval cTime1, cTime2;
	gettimeofday( &cTime1, (struct timezone*)0 );

	lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));

	cout << "———————————————————————————————————————————————————————————————————————————————" << endl;
	cout << "\033[1;" << 34 << "mFila    Nombre                              Valor            Tipo\033[0m" << endl;
        cout << "———————————————————————————————————————————————————————————————————————————————" << endl;
     
	for(unsigned int i = 0; i<tamano_tabla; i++){
		if(!(*(ptr_lista + i)).lista_vacia()){
			cout << "  "<< i;
			(*(ptr_lista + i)).imprimir_lista(inicio);
			cout << "———————————————————————————————————————————————————————————————————————————————" << endl;
		}
	}
	cout << endl << "Tamano Tabla -> " << tamano_tabla << endl
		<< "Colisiones -> " << colisiones << endl << "Tags -> " << cantidad_tags << endl;

	gettimeofday( &cTime2, (struct timezone*)0 );
    	if(cTime1.tv_usec > cTime2.tv_usec){
    	cTime2.tv_usec = 1000000;
    	--cTime2.tv_usec;
	}
	printf("Tiempo que tardo: %ld s, %ld us. \n", cTime2.tv_sec - cTime1.tv_sec, cTime2.tv_usec - cTime1.tv_usec );
}

/**
* @brief 	Modifica la Direccion de Referencia para la Tabla Hash
*			de Tags
* @param	inicio			Direccion de Inicio de la Memoria Compartida
* @param	nueva_th		Direccion de Tabla Hash
* @param	nuevo_inicio	Nueva Direccion de la Memoria Compartida
*/
void thtag::actualizar_tablahash(void *inicio, thtag * nueva_th, void * nuevo_inicio){
	lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));

	tags * tmp;
	for(unsigned int i = 0; i < tamano_tabla; i++){
		if(!(*(ptr_lista + i)).lista_vacia()){
			tmp = (*(ptr_lista + i)).cabeza(inicio);
			for(unsigned int j = 0; j < (*(ptr_lista + i)).cantidad; j++){
				nueva_th->buscar_tag(tmp, nuevo_inicio);
				if(tmp->ptr_proximo()){

					tmp = (tags *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);

				}
				else
					break;
			}
		}
	}
}

/**
* @brief 	Copia los Tags desde la Tabla Hash a un Arreglo
* @param	arreglo			Destino
* @param 	tamano_arreglo	Tamano del Arreglo
* @param	inicio			Direccion de Inicio de la Memoria Compartida
*/
void thtag::copiar_tags_a_arreglo( tags * arreglo,unsigned int * tamano_arreglo, void * inicio ){

	lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
	unsigned int count = 0;
	tags * tmp;
	for(unsigned int i = 0; i < tamano_tabla && count < (*tamano_arreglo); i ++){
		if(!(*(ptr_lista + i)).lista_vacia()){
			tmp = (*(ptr_lista + i)).cabeza(inicio);
			arreglo[count++].asignar_propiedades(tmp->nombre,tmp->valor_campo,tmp->tipo);
			for(unsigned int j = 0; j < (*(ptr_lista + i)).cantidad; j++){
				if( tmp->ptr_proximo() ){
					tmp = (tags *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);
					arreglo[count++].asignar_propiedades(tmp->nombre,tmp->valor_campo,tmp->tipo);
				}
			}
		}
	}
	*tamano_arreglo = count;
}

/**
* @brief 	Copia los Tags desde la Tabla Hash a un Arreglo
* @param	arreglo			Destino
* @param 	tamano_arreglo	Tamano del Arreglo
* @param	inicio			Direccion de Inicio de la Memoria Compartida
*/
void thtag::copiar_tags_a_arreglo_portable( Ttags_portable * arreglo,unsigned int * tamano_arreglo, void * inicio ){

	lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
	unsigned int count = 0;
	tags * tmp;
	for(unsigned int i = 0; i < tamano_tabla && count < (*tamano_arreglo); i ++){
		if(!(*(ptr_lista + i)).lista_vacia()){
			tmp = (*(ptr_lista + i)).cabeza(inicio);
			strcpy(arreglo[count].nombre, tmp->nombre);
            arreglo[count].tipo  = tmp->tipo;
            switch(tmp->tipo){
                case CARAC_:
                    arreglo[count++].valor_campo.C = tmp->valor_campo.C;
                break;
                case CORTO_:
                    arreglo[count++].valor_campo.S = tmp->valor_campo.S;
                break;
                case ENTERO_:
                    arreglo[count++].valor_campo.I = tmp->valor_campo.I;
                break;
                case LARGO_:
                    arreglo[count++].valor_campo.L = tmp->valor_campo.L;
                break;
                case REAL_:
                    arreglo[count++].valor_campo.F = tmp->valor_campo.F;
                break;
                case DREAL_:
                    arreglo[count++].valor_campo.D = tmp->valor_campo.D;
                break;
                case BIT_:
                    arreglo[count++].valor_campo.B = tmp->valor_campo.B;
                break;
                case TERROR_:
                default:
                    arreglo[count++].valor_campo.L = _ERR_DATO_;
            }
			for(unsigned int j = 0; j < (*(ptr_lista + i)).cantidad; j++){
				if( tmp->ptr_proximo() ){
					tmp = (tags *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);
					strcpy(arreglo[count].nombre, tmp->nombre);
                    arreglo[count].tipo  = tmp->tipo;
                    switch(tmp->tipo){
                        case CARAC_:
                            arreglo[count++].valor_campo.C = tmp->valor_campo.C;
                        break;
                        case CORTO_:
                            arreglo[count++].valor_campo.S = tmp->valor_campo.S;
                        break;
                        case ENTERO_:
                            arreglo[count++].valor_campo.I = tmp->valor_campo.I;
                        break;
                        case LARGO_:
                            arreglo[count++].valor_campo.L = tmp->valor_campo.L;
                        break;
                        case REAL_:
                            arreglo[count++].valor_campo.F = tmp->valor_campo.F;
                        break;
                        case DREAL_:
                            arreglo[count++].valor_campo.D = tmp->valor_campo.D;
                        break;
                        case BIT_:
                            arreglo[count++].valor_campo.B = tmp->valor_campo.B;
                        break;
                        case TERROR_:
                        default:
                            arreglo[count++].valor_campo.L = _ERR_DATO_;
                    }
				}
			}
		}
	}
	*tamano_arreglo = count;
}


}
