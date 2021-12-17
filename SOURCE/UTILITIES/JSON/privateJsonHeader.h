#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * JSON type identifier. Basic types are:
 * 	+ Object
 * 	+ Array
 * 	+ String
 * 	+ Other primitive: number, boolean (true/false) or null
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
typedef enum {
  json_UNDEFINED = 0,
  json_OBJECT = 1,
  json_ARRAY = 2,
  json_STRING = 3,
  json_PRIMITIVE = 4
} jsontype_t;

enum jsonerr {
  /* Not enough tokens were provided */
  json_ERROR_NOMEM = -1,
  /* Invalid character inside JSON string */
  json_ERROR_INVAL = -2,
  /* The string is not a full JSON packet, more bytes expected */
  json_ERROR_PART = -3
};

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * JSON token description.
 * type		type (object, array, string etc.)
 * start	start position in JSON data string
 * end		end position in JSON data string
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
typedef struct {
  jsontype_t type;
  int start;
  int end;
  int size;
  int parent;   // GWC
  int child;    // GWC
  int sibling;  // GWC
} jsontok_t;

/*         GWC Stuff    */
extern char       * jsonString;
extern jsontok_t  * tokens;
extern int          imax;
extern int          currentNode;

#define TRUE               1
#define FALSE              0

#define NO_NODE           -1
#define ROOT_NODE          0



#define PATH_START_ERROR   -10
#define NOT_AN_ARRAY       -11
#define BAD_SUBSCRIPT      -12
#define BAD_PATH           -13
#define NO_CHILDREN        -14
#define NO_SIBLING         -15


#define GOTOCHILD   if (tokens[node].child == NO_NODE) return NO_CHILDREN; else node = tokens[node].child;
#define GOTOSIBLING if (tokens[node].sibling == NO_NODE) return NO_SIBLING; else node = tokens[node].sibling;

typedef void (* jsonCallback )  ( char * N, char * V, int level, void * userData );

/*      End of GWC Stuff    */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string.
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
typedef struct {
  unsigned int pos;     /* offset in the JSON string */
  unsigned int toknext; /* next token to allocate */
  int toksuper;         /* superior token node, e.g. parent object or array */
} json_parser;



extern jsontok_t *json_alloc_token(json_parser *parser, jsontok_t *tokens,
                                 const size_t num_tokens);

extern void json_fill_token(jsontok_t *token, const jsontype_t type,
                          const int start, const int end);

extern int json_parse_primitive(json_parser *parser, const char *js,
                              const size_t len, jsontok_t *tokens,
                              const size_t num_tokens);

extern int json_parse_string(json_parser *parser, const char *js,
                           const size_t len, jsontok_t *tokens,
                           const size_t num_tokens);

extern int json_parse(json_parser *parser, const char *js, const size_t len,
                      jsontok_t *tokens, const unsigned int num_tokens);

extern void json_init(json_parser *parser);

extern char * tokenName( int type );

extern void json_tokenizer( char * fileName );

extern void findChildrenOf( int parent, int max );

extern void json_buildDom( );

extern char * getTokenValue( int S, int E );

extern void walkTree( int startNode, int level, jsonCallback cb, void * userdata  );

extern char * arrayStep( char * Path, int * pos );

extern char * objectStep( char * Path, int * pos );

extern int jsonNavigatePath( char * jsonPath );

extern void getNVpairs( int j, int level, jsonCallback cb, void * userdata );

extern char * json_error_string( int e );

extern char * json_extract ( char * path );

extern int json_array_length( char * path );

extern char * json_type( char * path );

extern int json_tree( char * jsonPath, jsonCallback cb, void * userData );
  
extern int json_each( char * jsonPath, jsonCallback cb, void * userData );
 
extern int json_valid( char * try );

extern int json_validator( char * filename );

extern void json_finalize( );

extern int json_squeezer ( char * dd, char * fn );


