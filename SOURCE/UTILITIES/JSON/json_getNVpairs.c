

#include "privateJsonHeader.h"


void getNVpairs( int j, int level, jsonCallback cb, void * userdata )  {
char * N;
char * V;
int    p;

     if ( level > 2 ) return;
     if ( ( tokens[j].type  != json_OBJECT ) && 
          ( tokens[j].type  != json_ARRAY )  &&
          ( tokens[j].child == NO_NODE ) ) {
              p = tokens[j].parent;
              N = getTokenValue( tokens[p].start, tokens[p].end );
              V = getTokenValue( tokens[j].start, tokens[j].end );
              if ( cb != NULL ) cb( N, V, level, userdata );  // Invoke the user callback
              free(N);  
              free(V);
     }
     if ( tokens[j].child != NO_NODE )   getNVpairs( tokens[j].child, level + 1,cb, userdata );
     if ( level != 0 ) {
          if ( tokens[j].sibling != NO_NODE ) 
             getNVpairs( tokens[j].sibling, level,cb, userdata );
     }
}

