#include <stdint.h>

#define AtomicPublish 103
#define AtomicAttach 104
#define AtomicCreateLocal 105
#define AtomicRead 106

#define MS_UPDATE 8

void createObject(char *name, char *description, int *version, int *error);
char *createLocal(char *name, char *view, uint32_t *flags, uint8_t *public);
void *attach(char *name, char *fragname, uint64_t *version, uint8_t *complete, int *fd, int *size);
void *reattach(void *data, uint64_t version);
void publish(char *name, char *fragname, void *data, int* size, uint64_t *version);
void detach(void *data);
