#include "unity.h"
#include "syscalls.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void createobject_(char *name, int *version, int *error) {
	createObject(name, ddesc, version, error);
	
        return;
}

void createlocal_(char *name, char *frag, int *start, int *length, int *f, int *p, char *fragname) {
	char *view = malloc(900*sizeof(char)), *tmp;
	uint32_t flags[1];
	uint8_t public[1];
	int i;
	
	*flags=*f;
	*public=*p;

        for(i = 0; frag[i]; i++){
                frag[i] = tolower(frag[i]);
        }

	sprintf(view, "{ var %s[i:%d] = %s[i+%d] }", frag, *length, frag, *start);
	
	strcpy(fragname,createLocal(name, view, flags, public));
	return;
}

void attach_ (char *name, char *fragname, int *version, int *comp, void **data, int *fd, int *size) {
	uint64_t ver[1];
	uint8_t complete[1];

	*ver = *version;
	complete[0]=*comp;

        *data = attach(name, fragname, ver, complete, fd, size);
        return;
}

void publish_(char *name,  char *fragname, void **data, int *size, int *version) {
	uint64_t ver, s;
	ver = *version;
	publish(name, fragname, *data, size, &ver);
}

/* void deleteobject_(char *name) { */
/*         HopMsg *msg = msg_alloc(), *rsv; */
/*         pack_tremove(msg, name); */
/*         hrpc_send(msg, &rsv); */
/* } */

void unityclose_(int *fd) {
	close(*fd);
}

//void detach_(char * name, char *fragname, void **data, int *size, int *fd) {
void detach_(void **data, int *size) {
	munmap(*data, *size);
}
/*         struct metadata *next, *md = list; */
/*         while (md != NULL) { */
/*                 munmap(md->data, md->size); */
/*                 close(md->fd); */
/*                 next = md->next; */
/*                 free(md); */
/*                 md = next; */
/*         } */

/*         return; */

/* } */
