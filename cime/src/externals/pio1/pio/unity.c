#include "rmt.h"
#include "unity.h"
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

HopClnt *client;
/* sem_t *lock=NULL; */

struct metadata{
        void *data;
        char *frag_name;
        int fd;
        uint64_t version;
        unsigned long size;
        struct metadata *next;
};

struct metadata *list = NULL;

void insert_mapped_region(struct metadata *new) {
        new->next = list;
        list = new;
}

struct metadata *get_mapped_region(void *data) {
        struct metadata *temp=list;
        while (temp != NULL) {
                if(temp->data == data)
                        return temp;
                temp = temp->next;
        }
        return NULL;
}


void hrpc_send(HopMsg *msg, HopMsg **rsv){
	/* while (lock==NULL) { */
	/* 	lock = sem_open("/sem_lock", O_RDWR); */
	/* } */

	/* sem_wait(lock); */
	if(client == NULL)
                client = hclntcreate(get_ip(), 5004, 0);
        hrpc(client, msg, rsv);
	/* sem_post(lock); */
}

void createObject(char *name, char *description, int *version, int *error) {
        HopMsg *msg = msg_alloc(), *rsv;
        HopValue val;

	/* if (lock==NULL) { */
	/* 	lock = sem_open("/sem_lock", O_CREAT, 0666, 1); */
	/* } */
	
        val.len=(strlen(description))*sizeof(char);
        val.data = description;
        pack_tcreate(msg, name, "0", &val);

        hrpc_send(msg, &rsv);
        *version = rsv->version;
	/* if (rsv->ecode != 0) { */

	/* 	fflush(stdout); */
	/* 	/\* printf("error: %s\n", rsv->edescr); *\/ */
	/* } */

	
        *error = rsv->ecode;
	
        return;
}

char *createLocal(char *name, char *view, uint32_t *flags, uint8_t *public) {
        HopMsg *msg = msg_alloc(), *rsv;
        HopValue vals[4];
        char myview[100], *tmp;
        uint64_t size;
        int len, i=1;
        char *frag_name;

        vals[0].len = sizeof(char)*(strlen(view));
        vals[0].data = view;

        vals[1].len = sizeof(char)*(strlen(view));
        vals[1].data=view;

        vals[2].len = sizeof(uint32_t);
        vals[2].data = malloc(vals[2].len);
        pint32(vals[2].data, *flags);

        vals[3].len = sizeof(uint8_t);
        vals[3].data = malloc(vals[3].len);
        pint8(vals[3].data,*public);

        pack_tatomic(msg, name, AtomicCreateLocal, 4, vals);
        hrpc_send(msg, &rsv);

	if (rsv->ecode != 0) {
		fflush(stdout);
		if (rsv->ecode == 5) 
			printf("error: %s %s\n", view, rsv->edescr);
	}
	
        frag_name = malloc(sizeof(char) * strlen(rsv->vals[0].data));
        strcpy(frag_name, rsv->vals[0].data);

	for (len=0; len < strlen(frag_name); len++)
                if (frag_name[len] == '\b') {
                        frag_name[len] = 0;
                        break;
                }

	return frag_name;
}

void *attach(char *name, char *fragname, uint64_t *version, uint8_t *complete, int *fd, int *size) {
        HopMsg *msg = msg_alloc(), *rsv;
        HopValue vals[2];
        char shm_name[64];
        int i=1;
        unsigned long lsize;
	void *ptr;

        sprintf(shm_name, "%s|%s", name, fragname);

        vals[0].len=sizeof(uint64_t);
        vals[0].data = malloc(vals[0].len);
        pint64(vals[0].data, *version);

        vals[1].len=sizeof(uint8_t);
        vals[1].data = malloc(vals[1].len);
        pint8(vals[1].data, *complete);

        pack_tatomic(msg, shm_name, AtomicAttach, 2, vals);
        hrpc_send(msg, &rsv);

        *fd = shm_open(shm_name, O_RDWR, 0666);

	if (*fd == -1) {
		int err = errno;
		printf("shm_open failed: %d\n", err);
		printf("%s\n", strerror(err));
                return NULL;
	}

        gint64(rsv->vals[1].data, &lsize);

	*size = ((int)lsize);
	return mmap(NULL, *size, PROT_READ|PROT_WRITE, MAP_PRIVATE, *fd, 0);

	/* if(md->data == -1) { */
	/* 	int err = errno; */
	/* 	printf("Mmap failed: %d\n", err); */
	/* 	printf("%s\n", strerror(err)); */
        /*         return NULL; */
	/* } */

	/* if(mlock(md->data, md->size) == -1){ */
	/* 	int err = errno; */
	/* 	printf("Mlock failed: %s\n", strerror(err)); */
	/* 	printf("errno: %d\n", err); */
	/* } */
	/* else */
	/* 	printf("Mlock success: %s %lu\n", shm_name, md->size); */

}

void *reattach(void *data, uint64_t version){
        struct metadata *md;
        HopMsg *msg = msg_alloc(), *rsv;
        char key[100];

        md = get_mapped_region(data);
        sprintf(key, ":%s\n", md->frag_name);

        pack_tget(msg, key, version);
        hrpc_send(msg, &rsv);
        munmap(data, md->size);
        md->data = mmap(NULL, md->size,
                        PROT_READ|PROT_WRITE, MAP_PRIVATE, md->fd, 0);
        return md->data;
}

void publish(char *name, char *fragname, void *data, int* size, uint64_t *version) {
        HopMsg *msg = msg_alloc(), *rsv;
        char key[100];
	HopValue val[2];

	val[0].len = sizeof(uint64_t);
	val[0].data = malloc(val[0].len);
	pint64(val[0].data, *version);
	val[1].len = sizeof(uint64_t);
	val[1].data = malloc(val[1].len);
	pint64(val[1].data, *version);

	sprintf(key, "%s|%s", name, fragname);
	msync(data, *size, MS_SYNC | MS_UPDATE);

	pack_tatomic(msg, key, AtomicPublish, 2, val);
	hrpc_send(msg, &rsv);

        return;
}

/* void listObjects() { */
/* 	HopMsg *msg = msg_alloc(), *rsv; */
/* 	char key[100]; */
/* 	uint64_t version=1; */

/* 	key="#/objects" */

/* 	pack_tget(msg, key, ver); */
/* 	hrpc_send(msg, &rsv); */
/* } */


/* void detach(void *data) { */
/*         struct metadata *md; */

/*         md = container_of(&data, struct metadata, data); */
/*         munmap(data, md->size); */
/*         close(md->fd); */
/*         free(md); */
/*         /\* notify runtime? *\/ */

/*         return; */
/* } */
