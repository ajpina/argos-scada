

#include "privateJsonHeader.h"


void walkTree( int startNode, int level, jsonCallback cb, void * userdata  ) {
char * N;
char * V;
int    p;

     if ( tokens[startNode].child == NO_NODE ) {
           p = tokens[startNode].parent;
           N = getTokenValue( tokens[p].start, tokens[p].end );
           V = getTokenValue( tokens[startNode].start, tokens[startNode].end );
           if ( cb != NULL ) 
              cb( N, V, level, userdata );  // Invoke the user callback
           free(N);  
           free(V);
     }
     if ( tokens[startNode].child   != NO_NODE )  { 
        walkTree( tokens[startNode].child,   level + 1, cb, userdata ); 
     }
     if ( level != 0 ) {
        if ( tokens[startNode].sibling != NO_NODE )  { 
           walkTree( tokens[startNode].sibling, level    , cb, userdata ); 
        }     
     }
}

