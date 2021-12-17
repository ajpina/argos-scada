

#include "privateJsonHeader.h"


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * This routine only parses a portion of the full json path specification
 * It would not be very difficult to make it fully compliant but I have
 * never (thus far) had a need for anything beyond what it currently provides
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

int  jsonNavigatePath( char * jsonPath ) {
int   node    = -1;
int   charPos =  0;
int   j,n,found;
char  * objectName = NULL;

    if ( jsonPath[0] == '$') {       // Absolute path starting from tree root
        node = ROOT_NODE;
    } else {
        if ( jsonPath[0] == '@') {   // Relative path from where ever we were
           node = currentNode;
        } else {
           return PATH_START_ERROR;
        }
    }
    charPos++;
    while ( jsonPath[charPos] != '\0' ) {
	// Now we need either an <array-step> or an <object-step>
	    if ( jsonPath[charPos] == '[' ) {  // We have an <array-step>
	       objectName = arrayStep(jsonPath,&charPos);
	       j = atoi( objectName );
	       free( objectName );
	       if ( tokens[node].type != json_ARRAY ) {
		  return NOT_AN_ARRAY;
	       }
	       GOTOCHILD
	       for(int i = 0; i < j; i++) { 
                  GOTOSIBLING 
               }
	    } else {  // Object Step
		 if ( jsonPath[charPos] != '.' ) {
		    return BAD_PATH;
		 } else {
		    objectName = objectStep( jsonPath, &charPos);
		    GOTOCHILD
		    found = FALSE;
		    while ( !found ) {
		        n = tokens[node].end - tokens[node].start;
		        if ( strncmp(objectName, jsonString + tokens[node].start, n ) == 0 ) {
		           found = TRUE;
		        } else { GOTOSIBLING }
		    }
                    free( objectName );
                    if ( found ) {
                        GOTOCHILD
                    } else {
                        return BAD_PATH;
                    }
		 }
	     }
    }
    currentNode = node;
    return node;
}
