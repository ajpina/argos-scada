

#include "privateJsonHeader.h"


void json_tokenizer( char * fileName ) {
json_parser parser;
int         fd,i;
long        l;

    fd = open( fileName, O_RDONLY );
    l  = lseek(fd, 0L, SEEK_END );    // How big is the file
    jsonString = (char *)calloc(l+1,1);
    lseek(fd,0L,SEEK_SET);            // Back to the beginning 
    read(fd,jsonString,l);            // Read the data into our buffer
    close(fd);                        // Don't need the file anymore

// We need to find out how many tokens are needed to parse this string
    json_init(&parser);
    i = json_parse(&parser, jsonString, strlen(jsonString), NULL, 0);
//    printf("You need <%d> tokens to parse that string!\n",i);
    tokens = (jsontok_t *) calloc( i + 1, sizeof( jsontok_t ) );
    json_init(&parser);
    imax = json_parse(&parser, jsonString, strlen(jsonString), tokens, i );
//    printf("Parsing used <%d> tokens\n",imax);
} 
