/**
* @file	as-thtag.h
* @class thtag
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

#ifndef THTAG_H_
#define THTAG_H_


#include "as-tags.h"

namespace argos{

class thtag{
protected:

/*--------------------------------------------------------
	Implementacion de una Lista Enlazada que
	se Encontrara en cada Renglon de la Tabla
	Hash para solventar las Colisiones que se
	generen
----------------------------------------------------------*/
	class lista {
	public:
		tags * primero;											//!< Primer Tag de la Lista
		void * ptr_primero;										//!< Primer Tag de la Lista Memoria Compartida
		unsigned int cantidad;									//!< Cantidad de Tags en la Lista

/*--------------------------------------------------------
	Borra Elementos de la Lista de
	Tags
----------------------------------------------------------*/
		void liberar_lista(){
			while(!lista_vacia()){
				tags * tmp = primero;
				primero = primero->siguiente;
				delete tmp;
			}
			cantidad = 0;
		}

		lista(){
			primero = NULL;
			cantidad = 0;
			ptr_primero = NULL;
		}

		~lista(){
			liberar_lista();
			cantidad = 0;
		}

/*--------------------------------------------------------
	Devuelve una direccion en el Segmento de
	Memoria Compartida (direccion)
----------------------------------------------------------*/
		void * operator new[](size_t n_bytes, void *direccion){
			return direccion;
		}

/*--------------------------------------------------------
	Inserta un Tag en la Lista
----------------------------------------------------------*/
		bool insertar(tags *p, void *direccion, void *inicio){
			tags  *tmp = new (direccion)tags(p->nombre,p->valor_campo,p->tipo);
			if(ptr_primero)
				tmp->siguiente = cabeza(inicio);
			primero = tmp;

			ptr_primero = (void *)((intptr_t)primero - (intptr_t)inicio);

			if(primero->siguiente){
				primero->ptr_siguiente = (void *)((intptr_t)primero->siguiente - (intptr_t)inicio);
			}
			cantidad++;
			return true;
		}

/*--------------------------------------------------------
	Busca un Tag en la Lista
----------------------------------------------------------*/
		bool buscar(tags * t, void *inicio){
			tags * tmp = cabeza(inicio);
			bool res = false;
			for(unsigned int i = 0; i<cantidad; i++){
				if(t->es_igual(tmp)){
					t->asignar_propiedades(tmp->nombre,tmp->valor_campo,tmp->tipo);
					res = true;
					break;
				}
				else if(tmp->ptr_proximo()){

					tmp = (tags *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);

				}
			}
			return res;
		}

/*--------------------------------------------------------
	Busca un Tag en la Lista y Devuelve
	su referencia
----------------------------------------------------------*/
		tags * buscar_ptr_tag(tags * t, void *inicio){
			tags * tmp = cabeza(inicio);
			for(unsigned int i = 0; i<cantidad; i++){
				if(t->es_igual(tmp)){
					return tmp;
				}
				else if(tmp->ptr_proximo()){
					tmp = (tags *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);
				}
			}
			return NULL;
		}

/*--------------------------------------------------------
	Actualiza el Estado de un Tag
----------------------------------------------------------*/
		bool actualizar(tags * t, void *inicio){
			tags * tmp = cabeza(inicio);
			bool res = false;
			for(unsigned int i = 0; i<cantidad; i++){
				if(t->es_igual(tmp)){
					//tmp->asignar_propiedades(t->nombre,t->valor_campo,t->tipo);
					tmp->valor_campo = t->valor_campo;
					res = true;
					break;
				}
				else if(tmp->ptr_proximo()){

					tmp = (tags *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);
				}
			}
			return res;
		}

/*--------------------------------------------------------
	Devuelve el Primer Tag de la Lista
----------------------------------------------------------*/
		tags * cabeza(void *inicio){
			if(ptr_primero){

				return (tags *)((intptr_t)ptr_primero + (intptr_t)inicio);
			}
			else
				return NULL;
		}

		bool lista_vacia(){
			return primero == NULL;
		}

/*--------------------------------------------------------
	Imprime por Pantalla la Lista de Tags
----------------------------------------------------------*/
		void imprimir_lista(void *inicio){
			tags * tmp = this->cabeza(inicio);
			for(unsigned int i = 0; i<cantidad; i++){				
				cout << "\t " << *tmp << endl;
				if(tmp->ptr_proximo()){
					tmp = (tags *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);
				}
				else
					break;
			}
		}

	};

private:
	pthread_mutex_t mutex;			//!< Sincroniza el acceso a la Memoria Compartida
	unsigned int cantidad_tags;		//!< Cantidad de Tags en la Tabla Hash
	unsigned int tamano_tabla;		//!< Tamano de la Tabla Hash
	unsigned int colisiones;		//!< Contador de Colisiones de la Tabla
	lista *tablaRT;					//!< Cada Renglon de la Tabla es una Lista
	void * ptr_tablaRT;				//!< Referencia al Renglon en Memoria Compartida
	void * ptr_mem_disponible;		//!< Direccion en Memoria Compartida Disponible

/*--------------------------------------------------------
	Crea el Objeto Tabla Hash donde Cada Renglon
	es una Lista Enlazada
----------------------------------------------------------*/
	int crear_tabla(unsigned int s, void * i);

public:

	thtag();
	thtag(unsigned int s);
	~thtag();

/*--------------------------------------------------------
	Devuelve una direccion en el Segmento de
	Memoria Compartida (direccion)
----------------------------------------------------------*/
	void * operator new(size_t n_bytes, void *direccion);

/*--------------------------------------------------------
	No hace nada ya que la direccion entregada se
	encuentra en el Segmento de Memoria Compartida
----------------------------------------------------------*/
	void operator delete(void *){}

/*--------------------------------------------------------
	Inserta un Tag en un Renglon de la
	Tabla Hash
----------------------------------------------------------*/
	int insertar_tag(tags t, void * i);

/**
* @brief	Buscar Tags en la Tabla Hash
* @param	t			Tag a Buscar
* @param	inicio		Direccion de la Memoria Compartida
* @return	verdadero	Tag Encontrado
*/
	inline bool buscar_tag(tags * t, void *inicio){
		unsigned int key = funcion_hash(*t);
		lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
		return (*(ptr_lista + key)).buscar(t, inicio);
	}

/**
* @brief	Buscar Tags en la Tabla Hash
* @param	t			Tag a Buscar
* @param	inicio		Direccion de la Memoria Compartida
* @return	tags		Referencia al Tag Encontrado
*/
	inline tags * obtener_ptr_tag(tags * t, void *inicio){
		unsigned int key = funcion_hash(*t);
		lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
		return (*(ptr_lista + key)).buscar_ptr_tag(t, inicio);
	}

/**
* @brief	Actualizar el Valor del Tag en la Tabla Hash
* @param	t			Tag a Actualizar
* @param	inicio		Direccion de la Memoria Compartida
* @return	verdadero	Tag Actualizado
*/
	inline bool actualizar_tag(tags * t, void *inicio){
		unsigned int key = funcion_hash(*t);
		lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
		return (*(ptr_lista + key)).actualizar(t, inicio);
	}

/**
* @brief	Devolver el Tamano de la Tabla Hash
* @return	tamano_tabla	Cantidad de Renglones de la Tabla Hash
*/
	inline unsigned int obtener_tamano_tabla(){
		return tamano_tabla;
	}

/**
* @brief	Devolver Cantidad de Tags en la Tabla Hash
* @return	cantidad_tags	Cantidad de Tags en la Tabla Hash
*/
	inline unsigned int obtener_cantidad_tags(){
		return cantidad_tags;
	}

/**
* @brief	Devolver las Colisiones Generadas al Insertar
*			Tags en la Tabla Hash
* @return	colisiones	Cantidad de Colisiones en la Tabla Hash
*/
	inline unsigned int obtener_colisiones(){
		return colisiones;
	}

/**
* @brief	Funcion Hash para Insertar	Tags en la Tabla Hash
* @param	t		Tag que se desea insertar
* @return	valor	Clave o Indice en la Tabla Hash
*/
	inline unsigned int funcion_hash(const tags & t){
		unsigned int valor = 5381;
		for(unsigned int i=0;i<strlen(t.nombre);i++)
			valor = ((valor << 5) + valor) + t.nombre[i];
		return ((valor & 0x7FFFFFFF) % obtener_tamano_tabla());
	}

/*--------------------------------------------------------
	Imprime el contenido de la Tabla Hash
	por Pantalla
----------------------------------------------------------*/
	void imprimir_tabla(void * i);

/*--------------------------------------------------------
	Modifica la Direccion de Referencia para la Tabla Hash
	de Alarmas
----------------------------------------------------------*/
	void actualizar_tablahash(void * i, thtag * nueva_th, void * ni);

	static unsigned int sizeof_lista(){
		return sizeof(thtag::lista);
	}

/*--------------------------------------------------------
	Crea un Arreglo de Alarmas desde la Tabla Hash
----------------------------------------------------------*/
	void copiar_tags_a_arreglo( tags * arr,unsigned int * ta, void * i );
	void copiar_tags_a_arreglo_portable( Ttags_portable * arr,unsigned int * ta, void * i );
};

}

#endif /*SSTHTAG_H_*/
