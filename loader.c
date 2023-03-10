/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h>

#include "exec_parser.h"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

static so_exec_t *exec;
static int descriptor;

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	/* TODO - actual loader implementation */
	so_seg_t aux;
	int i, poz=-1, size=4096;
	//caut segmentul in care da eroarea si retin pozitia segmentului respectiv in poz 
	for(i=0; i<exec->segments_no; i++){
		aux=exec->segments[i];
		if(info->si_addr >= aux.vaddr && info->si_addr <= aux.vaddr+ aux.mem_size){
			poz=i;
			break;		
		}
	}

	if(poz == -1)
		exit(139);

	//in var retin diferenta intre adresa unde da eroarea si adresa de inceput vaddr
	//start retine doar zona de memorie deja mapata in pagini
	int var=info->si_addr - exec->segments[poz].vaddr;
	int start=var-var%size;
	
	//mapez zona de memorie si aloc un buffer pentru a retine datele citite
	void *addres=mmap(exec->segments[poz].vaddr+start, size, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	char *buffer=malloc(size);

	//diferenta dintre file_size si var specifica daca pagina pe care vrem sa mapa mai incape in file_size sau nu
	if(exec->segments[poz].file_size-var > 0){
		//in caz afirmativ verificam daca putem incarca toata pagina sau doar o parte dn aceasta. Daca putem incarca in totalitate o pagina noua facem acest lucru
		if(exec->segments[poz].file_size-start >= size){
			if(exec->segments[poz].data != 0)
				exit(139);
			
			else{
				read(descriptor, buffer, size);
				memcpy(addres, buffer, size);
				mprotect(addres, size, exec->segments[poz].perm);
			}
		}
		//daca nu incape sa incarcam o noua pagina, ce depaseste file_size setam cu 0
		else {
				if(exec->segments[poz].data != 0)
					exit(139);
				else{
					exec->segments[poz].data= "mapat";
					read(descriptor, buffer, size);
					memset(addres+exec->segments[poz].file_size-start, 0, size-(exec->segments[poz].file_size-start));
					memcpy(addres, buffer, size);
					mprotect(addres, size-exec->segments[poz].file_size+start, exec->segments[poz].perm);
				}
			}
	}
	//in caz negativ setam toata zona respectiva de memorie cu 0
	else{
		if(exec->segments[poz].data != 0)
				exit(139);
		else{
			exec->segments[poz].data= "mapat";
			memset(addres,0, size);
		}
	}
	
	free(buffer);
}

int so_init_loader(void)
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, NULL);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	descriptor=open(path, O_RDONLY);
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
