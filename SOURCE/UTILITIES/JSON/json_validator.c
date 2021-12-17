#include "privateJsonHeader.h"


int json_validator( char * fileName ) {
int         fd,rc;
long        l;

    fd = open( fileName, O_RDONLY );
    l  = lseek(fd, 0L, SEEK_END );    // How big is the file
    jsonString = (char *)calloc(l+1,1);
    lseek(fd,0L,SEEK_SET);            // Back to the beginning 
    read(fd,jsonString,l);            // Read the data into our buffer
    close(fd);                        // Don't need the file anymore
    
    rc = json_valid( jsonString );
    free( jsonString );
    jsonString = NULL;
    return rc;
} 
