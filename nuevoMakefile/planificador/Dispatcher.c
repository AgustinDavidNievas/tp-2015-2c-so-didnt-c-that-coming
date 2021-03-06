/*
 * Dispatcher.c
 *
 *  Created on: 20/9/2015
 *      Author: utnso
 */

#include "Dispatcher.h"

t_PCB* algoritmoFIFO(){
	sem_wait(&semPCB);
	sem_wait(&semReady);
	t_PCB* pcb = list_remove(colaReady, 0);
	if (pcb != NULL){
		pcb->estado = EJECUTANDO;
	}
	sem_post(&semReady);
	sem_post(&semPCB);
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

	t_PCB* (*algoritmoPlanificador)();
	if ( *algoritmo == FIFO || *algoritmo == RR ) {
		algoritmoPlanificador = &algoritmoFIFO;
	}
	/*else if ( *algoritmo == RR ) {
		algoritmoPlanificador = &algoritmoRoundRobin;
	}*/
	else {
		ErrorFatal("Algoritmo de planificacion no disponible");
		return;
	}
	free(algoritmo);

	t_cpu* cpuLibre = cpuDisponible();
	if (cpuLibre == NULL){
		return;
	}
	sem_wait(&(cpuLibre->semaforoProceso));
	cpuLibre->procesoAsignado = algoritmoPlanificador();
	if (cpuLibre->procesoAsignado != NULL){
		pthread_mutex_lock(&lockLogger);
		log_info(logger, "Proceso seleccionado PID: %d\nEstado colas:", cpuLibre->procesoAsignado->pid );
		mostrarProcesos(logger->file);
		pthread_mutex_unlock(&lockLogger);
		if(__TEST__)
			mensajeEnviado = 1;
		else
			mensajeEnviado = enviarMensajeEjecucion(*cpuLibre);


		if (!mensajeEnviado){
			Error("No se pudo ejecutar el proceso %d", cpuLibre->procesoAsignado->pid);
			//TODO Devolver a la cola de ready
		}
		marcarEntradaCpu(cpuLibre);

		sem_post(&cpuLibre->semaforoMensaje); //Ya le envie lo que tenia que enviarle, entonces espero su respuesta
	}
	sem_post(&(cpuLibre->semaforoProceso));

}

void pasarABloqueados(t_cpu *cpu, int tiempo, int proximaInstruccion){
	bool _mismoProceso(t_PCB* pcb){
		return cpu->procesoAsignado->pid == pcb->pid;
	}
	pthread_t hDormirProceso;
	t_noni *noni = malloc(sizeof(t_noni));
	sem_wait(&cpu->semaforoProceso);
	sem_wait(&semPCB);
	sem_wait(&semLock);

	t_PCB *pcb = list_find(PCBs, (void*)_mismoProceso);
	pcb->estado = ESPERANDO_IO;
	pcb->nroLinea = pcb->nroLinea == -1 ? -1 : proximaInstruccion;
	list_add(colaBloqueados, pcb);
	noni->pid = cpu->procesoAsignado->pid;
	noni->tiempo = tiempo;
	time(&pcb->tiempoInicioEspera);
	cpu->procesoAsignado = NULL;
	pthread_create(&hDormirProceso, NULL, (void*)dormirProceso, noni);

	sem_post(&cpu->semaforoProceso);
	sem_post(&semPCB);
	sem_post(&semLock);
}

void pasarAReady(t_cpu *cpu, int proximaInstruccion){
	bool _mismoProceso(t_PCB* pcb){
		return cpu->procesoAsignado->pid == pcb->pid;
	}

	sem_wait(&cpu->semaforoProceso);
	sem_wait(&semPCB);
	sem_wait(&semReady);

	t_PCB* pcb = list_find(PCBs, (void*) _mismoProceso);
	pcb->estado = LISTO;
	pcb->nroLinea = pcb->nroLinea == -1 ? -1 : proximaInstruccion;
	list_add(colaReady, pcb);
	cpu->procesoAsignado = NULL;

	sem_post(&cpu->semaforoProceso);
	sem_post(&semPCB);
	sem_post(&semReady);
}

void dormirProceso(t_noni* noni){
	bool _mismoProceso(t_PCB* pcb){
		return noni->pid == pcb->pid;
	}
	t_PCB *pcb;

	sem_wait(&semIO);
	sem_wait(&semPCB);
	sem_wait(&semLock);
	pcb = list_find(colaBloqueados, (void*)_mismoProceso);
	if ( pcb ) //Si se saco de la cola de bloqueados es porque se finalizo
		pcb->estado = BLOQUEADO;

	sem_post(&semPCB);
	sem_post(&semLock);
	if (pcb) sleep(noni->tiempo);
	sem_post(&semIO);

	if (pcb){
		sem_wait(&semPCB);
		sem_wait(&semReady);
		sem_wait(&semLock);

		pcb = list_remove_by_condition(colaBloqueados, (void*) _mismoProceso);
		pcb->estado = LISTO;
		list_add(colaReady, pcb);
		pcb->tiempoEspera += time(NULL)-pcb->tiempoInicioEspera;
		sem_post(&semPCB);
		sem_post(&semReady);
		sem_post(&semLock);
	}
	free(noni);
	ejecutarDispatcher();
}

void terminarProceso(t_cpu* cpu){
	bool _mismoProceso(t_PCB* pcb){
		return cpu->procesoAsignado->pid == pcb->pid;
	}

	sem_wait(&cpu->semaforoProceso);
	sem_wait(&semPCB);

	t_PCB *pcb = list_remove_by_condition(PCBs, (void*) _mismoProceso);

	pthread_mutex_lock(&lockLogger);
	log_info(logger, "Proceso %d finalizado.\n\tTiempo total en el sistema: %ld segundos.\n\tTiempo de espera: %lld segundos.\n\tTiempo de ejecucion: %lld segundos.\n", pcb->pid, time(NULL)-pcb->horaCreacion, pcb->tiempoEspera, pcb->tiempoEjecucion);
	pthread_mutex_unlock(&lockLogger);

	cpu->procesoAsignado = NULL;
	sem_post(&cpu->semaforoProceso);
	sem_post(&semPCB);

	free(pcb->path);
	free(pcb);
}
