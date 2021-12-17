

#include "privateJsonHeader.h"



/**
 * Fills next available token with JSON primitive.
 */
int json_parse_primitive(json_parser *parser, const char *js,
                                const size_t len, jsontok_t *tokens,
                                const size_t num_tokens) {
  jsontok_t *token;
  int start;

  start = parser->pos;

  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
    switch (js[parser->pos]) {
    case '\t':
    case '\r':
    case '\n':
    case ' ':
    case ',':
    case ']':
    case '}':
      goto found;
    default:
                   /* to quiet a warning from gcc*/
      break;
    }
    if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
      parser->pos = start;
      return json_ERROR_INVAL;
    }
  }

  parser->pos = start;
  return json_ERROR_PART;


found:
  if (tokens == NULL) {
    parser->pos--;
    return 0;
  }
  token = json_alloc_token(parser, tokens, num_tokens);
  if (token == NULL) {
    parser->pos = start;
    return json_ERROR_NOMEM;
  }
  json_fill_token(token, json_PRIMITIVE, start, parser->pos);
  token->parent = parser->toksuper;  // GWC
  parser->pos--;
  return 0;
}

