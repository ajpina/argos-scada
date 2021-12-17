#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


typedef void (* jsonCallback )  ( char * N, char * V, int level, void * userData );

extern void   json_tokenizer(char *fileName);
extern void   json_buildDom(void);
extern char * json_error_string(int e);
extern char * json_extract(char *path);
extern int    json_array_length(char *path);
extern char * json_type(char *path);
extern int    json_tree(char *jsonPath, jsonCallback cb, void *userData);
extern int    json_each(char *jsonPath, jsonCallback cb, void *userData);
extern int    json_valid(char *try);
extern int    json_validator( char * filename );
extern void   json_finalize(void);
extern int    json_squeezer( char *, char * );



