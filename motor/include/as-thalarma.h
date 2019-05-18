/**
* @file	as-thalarma.h
* @class thalarma
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

#ifndef THALARMA_H_
#define THALARMA_H_


#include "as-alarma.h"

namespace argos{

class thalarma{
protected:

/*--------------------------------------------------------
	Implementacion de una Lista Enlazada que
	se Encontrara en cada Renglon de la Tabla
	Hash para solventar las Colisiones que se
	generen
----------------------------------------------------------*/
	class lista {
	public:
		alarma * primero;		//!< Primer Alarma de la Lista
		void * ptr_primero;		//!< Primer Alarma de la Lista Memoria Compartida
		unsigned int cantidad;	//!< Cantidad de Alarmas en la Lista

/*--------------------------------------------------------
	Borra Elementos de la Lista de
	Alarmas
----------------------------------------------------------*/
		void liberar_lista(){
			while(!lista_vacia()){
				alarma * tmp = primero;
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
	Inserta una Alarma en la Lista
----------------------------------------------------------*/
		bool insertar(alarma *p, void *direccion, void *inicio){
			alarma  *tmp = new (direccion)alarma(p->id_grupo, p->grupo,
						p->id_subgrupo, p->subgrupo, p->nombre,	p->estado,
						p->valor_comparacion, p->tipo_comparacion, p->tipo_origen);
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
	Busca una Alarma en la Lista
----------------------------------------------------------*/
		bool buscar(alarma * t, void *inicio){
			alarma * tmp = cabeza(inicio);
			bool res = false;
			for(unsigned int i = 0; i<cantidad; i++){
				if(t->es_igual(tmp)){
					t->asignar_propiedades(tmp->id_grupo, tmp->grupo,
						tmp->id_subgrupo, tmp->subgrupo, tmp->nombre, tmp->estado,
						tmp->valor_comparacion, tmp->tipo_comparacion, tmp->tipo_origen);
					res = true;
					break;
				}
				else if(tmp->ptr_proximo()){
					tmp = (alarma *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);
				}
			}
			return res;
		}

/*--------------------------------------------------------
	Busca una Alarma en la Lista y Devuelve
	su referencia
----------------------------------------------------------*/
		alarma * buscar_ptr_alarma(alarma * a, void *inicio){
			alarma * tmp = cabeza(inicio);
			for(unsigned int i = 0; i<cantidad; i++){
				if(a->es_igual(tmp)){
					return tmp;
				}
				else if(tmp->ptr_proximo()){
					tmp = (alarma *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);
				}
			}
			return NULL;
		}

/*--------------------------------------------------------
	Actualiza el Estado de una Alarma
----------------------------------------------------------*/
		bool actualizar(alarma * t, void *inicio){
			alarma * tmp = cabeza(inicio);
			bool res = false;
			for(unsigned int i = 0; i<cantidad; i++){
				if(t->es_igual(tmp)){
					//tmp->asignar_propiedades(t->id_grupo, t->grupo,
					//	t->id_subgrupo, t->subgrupo, t->nombre, t->estado,
					//	t->valor_comparacion, t->tipo_comparacion, t->tipo_origen);
					tmp->estado = t->estado;
					res = true;
					break;
				}
				else if(tmp->ptr_proximo()){
					tmp = (alarma *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);
				}
			}
			return res;
		}

/*--------------------------------------------------------
	Devuelve la Primera Alarma de la Lista
----------------------------------------------------------*/
		alarma * cabeza(void *inicio){
			if(ptr_primero){
				return (alarma *)((intptr_t)ptr_primero + (intptr_t)inicio);
			}
			else
				return NULL;
		}

		bool lista_vacia(){
			return primero == NULL;
		}

/*--------------------------------------------------------
	Imprime por Pantalla la Lista de Alarmas
----------------------------------------------------------*/
		void imprimir_lista(void *inicio){
			alarma * tmp = this->cabeza(inicio);
			for(unsigned int i = 0; i<cantidad; i++){
				cout << *tmp << endl;
				if(tmp->ptr_proximo()){
					tmp = (alarma *)((intptr_t)tmp->ptr_proximo() + (intptr_t)inicio);

				}
				else
					break;
			}
		}
	};

private:
	pthread_mutex_t mutex;			//!< Sincroniza el acceso a la Memoria Compartida
	unsigned int cantidad_alarmas;	//!< Cantidad de Alarmas en la Tabla Hash
	unsigned int tamano_tabla;		//!< Tamano de la Tabla Hash
	unsigned int colisiones;		//!< Contador de Colisiones de la Tabla
	lista *tablaRT;					//!< Cada Renglon de la Tabla es una Lista
	void * ptr_tablaRT;				//!< Referencia al Renglon en Memoria Compartida
	void * ptr_mem_disponible;		//!< Direccion en Memoria Compartida Disponible
									//!< para Insertar

/*--------------------------------------------------------
	Crea el Objeto Tabla Hash donde Cada Renglon
	es una Lista Enlazada
----------------------------------------------------------*/
	int crear_tabla(unsigned int s, void * i);

public:

	thalarma();
	thalarma(unsigned int s);
	~thalarma();

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
	Inserta una Alarma en un Renglon de la
	Tabla Hash
----------------------------------------------------------*/
	int insertar_alarma(alarma t, void * i);

/**
* @brief	Buscar Alarmas en la Tabla Hash
* @param	t			Alarma a Buscar
* @param	inicio		Direccion de la Memoria Compartida
* @return	verdadero	Alarma Encontrada
*/
	inline bool buscar_alarma(alarma * t, void *inicio){
		unsigned int key = funcion_hash(*t);
		lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
		return (*(ptr_lista + key)).buscar(t, inicio);
	}

/**
* @brief	Buscar Alarmas en la Tabla Hash
* @param	a			Alarma a Buscar
* @param	inicio		Direccion de la Memoria Compartida
* @return	alarma		Referencia a la Alarma Encontrada
*/
	inline alarma * obtener_ptr_alarma(alarma * a, void *inicio){
		unsigned int key = funcion_hash(*a);
		lista * ptr_lista = (lista *)((intptr_t)inicio + (intptr_t)ptr_tablaRT + sizeof(intptr_t));
		return (*(ptr_lista + key)).buscar_ptr_alarma(a, inicio);
	}

/**
* @brief	Actualizar el Estado de Alarmas en la Tabla Hash
* @param	t			Alarma a Buscar
* @param	inicio		Direccion de la Memoria Compartida
* @return	verdadero	Alarma Actualizada
*/
	inline bool actualizar_alarma(alarma * t, void *inicio){
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
* @brief	Devolver Cantidad de Alarmas en la Tabla Hash
* @return	cantidad_alarmas	Cantidad de Alarmas en la Tabla Hash
*/
	inline unsigned int obtener_cantidad_alarmas(){
		return cantidad_alarmas;
	}

/**
* @brief	Devolver las Colisiones Generadas al Insertar
*			Alarmas en la Tabla Hash
* @return	colisiones	Cantidad de Colisiones en la Tabla Hash
*/
	inline unsigned int obtener_colisiones(){
		return colisiones;
	}

/**
* @brief	Funcion Hash para Insertar	Alarmas en la Tabla Hash
* @param	t		Alarma que se desea insertar
* @return	valor	Clave o Indice en la Tabla Hash
*/
	inline unsigned int funcion_hash(const alarma & t){
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
	void actualizar_tablahash(void * i, thalarma * nueva_th, void * ni);

	static unsigned int sizeof_lista(){
		return sizeof(thalarma::lista);
	}

/*--------------------------------------------------------
	Crea un Arreglo de Alarmas desde la Tabla Hash
----------------------------------------------------------*/
	void copiar_alarmas_a_arreglo( alarma * arr,unsigned int * ta, void * i );
	void copiar_alarmas_a_arreglo_portable( Talarmas_portable * arr,unsigned int * ta, void * i );
};


}

#endif /*SSTHALARMA_H_*/
