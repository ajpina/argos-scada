

#include "privateJsonHeader.h"




/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * This function builds a tree from the array of tokens the jsonParser
 * built ... the tree of course is the DOM of the json String and the 
 * root node of the tree is always in tokens[0]
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

void json_buildDom( ) {
        for( int j = 0; j < imax; j++) {
           findChildrenOf(j,imax);
        }
}
