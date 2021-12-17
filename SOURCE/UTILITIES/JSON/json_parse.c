

#include "privateJsonHeader.h"


/**
 * Parse JSON string and fill tokens.
 */
int json_parse(json_parser *parser, const char *js, const size_t len,
                        jsontok_t *tokens, const unsigned int num_tokens) {
  int r;
  int i;
  jsontok_t *token;
  int count = parser->toknext;

  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
    char c;
    jsontype_t type;

    c = js[parser->pos];
    switch (c) {
    case '{':
    case '[':
      count++;
      if (tokens == NULL) {
        break;
      }
      token = json_alloc_token(parser, tokens, num_tokens);
      if (token == NULL) {
        return json_ERROR_NOMEM;
      }
      if (parser->toksuper != -1) {
        jsontok_t *t = &tokens[parser->toksuper];

        if (t->type == json_OBJECT) {
          return json_ERROR_INVAL;
        }

        t->size++;

        token->parent = parser->toksuper; // GWC

      }
      token->type = (c == '{' ? json_OBJECT : json_ARRAY);
      token->start = parser->pos;
      parser->toksuper = parser->toknext - 1;
      break;
    case '}':
    case ']':
      if (tokens == NULL) {
        break;
      }
      type = (c == '}' ? json_OBJECT : json_ARRAY);

      if (parser->toknext < 1) {
        return json_ERROR_INVAL;
      }
      token = &tokens[parser->toknext - 1];
      for (;;) {
        if (token->start != -1 && token->end == -1) {
          if (token->type != type) {
            return json_ERROR_INVAL;
          }
          token->end = parser->pos + 1;
          parser->toksuper = token->parent; // GWC 
          break;
        }
        if (token->parent == -1) { // GWC 
          if (token->type != type || parser->toksuper == -1) { // GWC 
            return json_ERROR_INVAL; // GWC 
          } // GWC 
          break; // GWC 
        } // GWC 
        token = &tokens[token->parent]; // GWC 
      }

      break;
    case '\"':
      r = json_parse_string(parser, js, len, tokens, num_tokens);
      if (r < 0) {
        return r;
      }
      count++;
      if (parser->toksuper != -1 && tokens != NULL) {
        tokens[parser->toksuper].size++;
      }
      break;
    case '\t':
    case '\r':
    case '\n':
    case ' ':
      break;
    case ':':
      parser->toksuper = parser->toknext - 1;
      break;
    case ',':
      if (tokens != NULL && parser->toksuper != -1 &&
          tokens[parser->toksuper].type != json_ARRAY &&
          tokens[parser->toksuper].type != json_OBJECT) {

        parser->toksuper = tokens[parser->toksuper].parent; // GWC 

      }
      break;

    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 't':
    case 'f':
    case 'n':
      /* And they must not be keys of the object */
      if (tokens != NULL && parser->toksuper != -1) {
        const jsontok_t *t = &tokens[parser->toksuper];
        if (t->type == json_OBJECT ||
            (t->type == json_STRING && t->size != 0)) {
          return json_ERROR_INVAL;
        }
      }

      r = json_parse_primitive(parser, js, len, tokens, num_tokens);
      if (r < 0) {
        return r;
      }
      count++;
      if (parser->toksuper != -1 && tokens != NULL) {
        tokens[parser->toksuper].size++;
      }
      break;


    default:
      return json_ERROR_INVAL;

    }
  }

  if (tokens != NULL) {
    for (i = parser->toknext - 1; i >= 0; i--) {
      /* Unmatched opened object or array */
      if (tokens[i].start != -1 && tokens[i].end == -1) {
        return json_ERROR_PART;
      }
    }
  }

  return count;
}

