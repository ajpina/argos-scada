

#include "privateJsonHeader.h"


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Recursively find children of a given parent and link them into
 * the tree ...
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

void findChildrenOf( int parent, int max ) {
int  i,lastChild;
   for(i = parent+1;i < max; i++) {
       if ( tokens[i].parent == parent ) {
           if ( tokens[parent].child == -1 ) { 
              tokens[parent].child = i; 
              lastChild = i;
           } else {
              tokens[lastChild].sibling = i;
              lastChild = i;
           }
       }
   }
}
