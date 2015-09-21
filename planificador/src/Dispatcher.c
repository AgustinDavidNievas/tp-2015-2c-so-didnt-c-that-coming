/*
 * Dispatcher.c
 *
 *  Created on: 20/9/2015
 *      Author: utnso
 */

#include "Dispatcher.h"

t_PCB* algoritmoFIFO(){
	sem_wait(&semReady);
	t_PCB* pcb = list_remove(colaReady, colaReady->elements_count);
	sem_post(&semReady);
	return pcb;
}

t_PCB* algoritmoRoundRobin(){
	return NULL;
}

t_cpu* cpuDisponible(){

	bool* _estaLibre(t_cpu* cpu){
		return (cpu->procesoAsignado == NULL);
	}

	sem_wait(&semListaCpu);
	t_cpu *cpu = list_find(lista_cpu, (void*) _estaLibre);
	sem_post(&semListaCpu);

	return cpu;
}

void Dispatcher(void *args){

	t_algoritmo* algoritmo = args;
	int mensajeEnviado;

	void* algoritmoPlanificador;
	if ( *algoritmo == FIFO ) {
		algoritmoPlanificador = (void*)algoritmoFIFO;
	}
	else if ( *algoritmo == RR ) {
		algoritmoPlanificador = (void*)algoritmoRoundRobin;
	}
	else {
		ErrorFatal("Algoritmo de planificacion no disponible");
		return;
	}

	t_cpu* cpuLibre = cpuDisponible();

	sem_wait(&(cpuLibre->semaforo));
	cpuLibre->procesoAsignado = (t_PCB*)algoritmoPlanificador;
	mensajeEnviado = enviarMensajeEjecucion(*cpuLibre);
	sem_post(&(cpuLibre->semaforo));

	if (!mensajeEnviado){
		Error("No se pudo ejecutar el proceso &d", cpuLibre->procesoAsignado->pid);
		//TODO Devolver a la cola de ready
	}
}