#include <string.h>

#include "jsmn.h"

/**
 * Allocates a fresh unused token from the token pull.
 */
static jsmn_Token *jsmn_alloc_token(jsmn_Factory *factory, size_t len) {
    jsmn_Token *tok;
    if (factory->toknext + (len - 1) >= factory->tokslen) {
        return NULL;
    }
    for (int i = 0; i < len; i++) {
        tok = factory->toks + factory->toknext++;
        tok->type = JSMN_UNDEFINED;
        tok->data = NULL;
        tok->length = -1;
        tok->start = tok->end = -1;
        tok->size = 0;
        tok->parent = -1;
    }
    return tok - (len - 1);
}

/**
 * Fills token type and boundaries.
 */
static void jsmn_fill_token(jsmn_Token *token, jsmntype_t type,
                            int start, int end) {
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}

void jsmn_factory_init(jsmn_Factory *factory, jsmn_Token *toks, size_t len) {
    factory->toks = toks;
    factory->tokslen = len;
    factory->toknext = 0;
    factory->toksuper = -1;
}

static jsmn_Token *jsmn_prepare_append(jsmn_Factory *factory, const char *name)
{
    jsmn_Token *token;
    int n_tokens = 1;
    // Does the append going to be within a supertoken 
    if (factory->toksuper != -1) {
        // If the append is with in a object a label token is needed.
        if (factory->toks[factory->toksuper].type == JSMN_OBJECT) {
            if (name == NULL) {
                // Exit with an error, if no name for a label token is defined
                return NULL;
            }
            // Two tokens have to be appended a label and the actual token
            n_tokens = 2;
        }
    }
    // Allocate the needed tokens
    token = jsmn_alloc_token(factory, n_tokens);
    if (token == NULL) {
        return NULL;
    }
    // Increment the size of the supertoken
    if (factory->toksuper != -1) {
        factory->toks[factory->toksuper].size++;
    }
    // Append label token
    if (n_tokens == 2) {
        int toklabel = token - factory->toks;
        token->type = JSMN_LABEL;
        token->data = name;
        token->length = strlen(name);
        token->size = 1;
        // The parent token of the label will be the current supertoken
        token->parent = factory->toksuper;
        // Jump the the actual token
        token++;
        // The parent of the actual token will be the label token
        token->parent = toklabel;
    } else if (factory->toksuper != -1) {
        // Within an array, set the current supertoken as parent
        token->parent = factory->toksuper;
    } else {
        // As root token, set the parent to -1
        token->parent = -1;
    }
    return token;
}

static int jsmn_start_sequence(jsmn_Factory *factory, jsmntype_t type,
        const char *name)
{
    jsmn_Token *token;
    // Prepare the token
    token = jsmn_prepare_append(factory, name);
    if (token == NULL) {
        return JSMN_ERROR_FACTORY;
    }
    // Append object token
    token->type = type;
    factory->toksuper = token - factory->toks;
    return factory->toknext;
}

static int jsmn_end_sequence(jsmn_Factory *factory, jsmntype_t type)
{
    jsmn_Token *supertoken;
    // Check wheter there is a sequence (object or array) to end
    if (factory->toksuper == -1) {
        return JSMN_ERROR_FACTORY;
    }
    supertoken = factory->toks + factory->toksuper;
    // Check the type of the to be ended sequence
    if (supertoken->type != type) {
        return JSMN_ERROR_FACTORY;
    }
    if (supertoken->parent != -1) {
        // Found an other supertoken
        supertoken = factory->toks + supertoken->parent;
        // At this state the current supertoken must be a label or an array ...
        if (supertoken->type == JSMN_LABEL || supertoken->type == JSMN_ARRAY) {
            // ... set the supertoken of the factory to the parent of the
            // current supertoken ... 
            factory->toksuper = supertoken->parent;
        } else {
            // ... otherwise there must be an error.
            return JSMN_ERROR_FACTORY;
        }
    } else {
        // There is no supertoken -> end of root sequence
        factory->toksuper = -1;
    }
    return factory->toknext;
}

int jsmn_start_object(jsmn_Factory *factory, const char *name)
{
    return jsmn_start_sequence(factory, JSMN_OBJECT, name);
}

int jsmn_end_object(jsmn_Factory *factory)
{
    return jsmn_end_sequence(factory, JSMN_OBJECT);
}

int jsmn_start_array(jsmn_Factory *factory, const char *name)
{
    return jsmn_start_sequence(factory, JSMN_ARRAY, name);
}

int jsmn_end_array(jsmn_Factory *factory)
{
    return jsmn_end_sequence(factory, JSMN_ARRAY);
}

static int jsmn_append_simple(jsmn_Factory *factory, jsmntype_t type,
        const char *name, const char *value)
{
    jsmn_Token *token;
    // Prepare the token
    token = jsmn_prepare_append(factory, name);
    if (token == NULL) {
        return JSMN_ERROR_FACTORY;
    }
    // Append the data to the string or primitive token
    token->type = type;
    token->data = value;
    if (value != NULL) {
        token->length = strlen(value);
    }
    return factory->toknext;
}

int jsmn_append_string(jsmn_Factory *factory, const char *name,
        const char *value)
{
    return jsmn_append_simple(factory, JSMN_STRING, name, value);
}

int jsmn_append_primitive(jsmn_Factory *factory, const char *name,
        const char *value)
{
    return jsmn_append_simple(factory, JSMN_PRIMITIVE, name, value);
}

int jsmn_dump(jsmn_Token *t, jsmn_write_handle_t cb) {
    if (t->type == JSMN_PRIMITIVE) {
        cb(t->data, t->length);
        return 1;
    } else if (t->type == JSMN_LABEL || t->type == JSMN_STRING) {
        cb("\"", 1);
        if (t->length > 0) {
            cb(t->data, t->length);
        }
        cb("\":", t->type == JSMN_LABEL ? 2 : 1);
        return 1;
    } else if (t->type == JSMN_OBJECT) {
        int j = 0;
        cb("{", 1);
        for (int i = 0; i < t->size; i++) {
            j += jsmn_dump(t + 1 + j, cb);
            j += jsmn_dump(t + 1 + j, cb);
            if (i < t->size - 1) {
                cb(",", 1);
            }
        }
        cb("}", 1);
        return j + 1;
    } else if (t->type == JSMN_ARRAY) {
        int j = 0;
        cb("[", 1);
        for (int i = 0; i < t->size; i++) {
            j += jsmn_dump(t + 1 + j, cb);
            if (i < t->size - 1) {
                cb(",", 1);
            }
        }
        cb("]", 1);
        return j + 1;
    }
    return 0;
}

void jsmn_parser_init(jsmn_Parser *parser, jsmn_Token *toks, size_t len) {
    jsmn_factory_init((jsmn_Factory *)parser, toks, len);
    parser->pos = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
static int jsmn_parse_primitive(jsmn_Parser *parser, const char *js,
        size_t len) {
    jsmn_Token *token;
    jsmn_Factory *factory = (jsmn_Factory *)parser;
    int start = parser->pos;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        switch (js[parser->pos]) {
            // In strict mode primitive must be followed by "," or "}" or "]"
            case '\t' : case '\r' : case '\n' : case ' ' :
            case ','  : case ']'  : case '}' :
                goto found;
        }
        if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
            parser->pos = start;
            return JSMN_ERROR_INVAL;
        }
    }
    // In strict mode primitive must be followed by a comma/object/array
    parser->pos = start;
    return JSMN_ERROR_PART;

found:
    if (factory->toks == NULL) {
        parser->pos--;
        return 0;
    }
    token = jsmn_alloc_token(factory, 1);
    if (token == NULL) {
        parser->pos = start;
        return JSMN_ERROR_NOMEM;
    }
    jsmn_fill_token(token, JSMN_PRIMITIVE, start, parser->pos);
    token->parent = factory->toksuper;
    parser->pos--;
    return 0;
}

/**
 * Fills next token with JSON string.
 */
static int jsmn_parse_string(jsmn_Parser *parser, const char *js, size_t len) {
    jsmn_Token *token;
    jsmn_Factory *factory = (jsmn_Factory *)parser;
    int start = parser->pos;

    parser->pos++;

    // Skip starting quote
    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        char c = js[parser->pos];

        // Quote: end of string
        if (c == '\"') {
            jsmntype_t type;
            if (factory->toks == NULL) {
                return 0;
            }
            token = jsmn_alloc_token(factory, 1);
            if (token == NULL) {
                parser->pos = start;
                return JSMN_ERROR_NOMEM;
            }
            // Check wheter string is a label or an ordinary value
            type = JSMN_STRING;
            if (factory->toks[factory->toksuper].type == JSMN_OBJECT) {
                type = JSMN_LABEL;
            }
            jsmn_fill_token(token, type, start+1, parser->pos);
            token->parent = factory->toksuper;
            return 0;
        }

        // Backslash: Quoted symbol expected
        if (c == '\\' && parser->pos + 1 < len) {
            int i;
            parser->pos++;
            switch (js[parser->pos]) {
                // Allowed escaped symbols
                case '\"': case '/' : case '\\' : case 'b' :
                case 'f' : case 'r' : case 'n'  : case 't' :
                    break;
                // Allows escaped symbol \uXXXX
                case 'u':
                    parser->pos++;
                    for(i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++) {
                        // If it isn't a hex character we have an error
                        if(!((js[parser->pos] >= 48 && js[parser->pos] <= 57) || /* 0-9 */
                                    (js[parser->pos] >= 65 && js[parser->pos] <= 70) || /* A-F */
                                    (js[parser->pos] >= 97 && js[parser->pos] <= 102))) { /* a-f */
                            parser->pos = start;
                            return JSMN_ERROR_INVAL;
                        }
                        parser->pos++;
                    }
                    parser->pos--;
                    break;
                // Unexpected symbol
                default:
                    parser->pos = start;
                    return JSMN_ERROR_INVAL;
            }
        }
    }
    parser->pos = start;
    return JSMN_ERROR_PART;
}

int jsmn_parse(jsmn_Parser *parser, const char *js, size_t len) {
    jsmn_Factory *factory = (jsmn_Factory *)parser;
    jsmn_Token *token;
    jsmn_Token *tokens = factory->toks;
    int count = factory->toknext;
    int r;
    int i;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        char c;
        jsmntype_t type;

        c = js[parser->pos];
        switch (c) {
            case '{': case '[':
                count++;
                if (tokens == NULL) {
                    break;
                }
                token = jsmn_alloc_token(factory, 1);
                if (token == NULL)
                    return JSMN_ERROR_NOMEM;
                if (factory->toksuper != -1) {
                    tokens[factory->toksuper].size++;
                    token->parent = factory->toksuper;
                }
                token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
                token->start = parser->pos;
                factory->toksuper = factory->toknext - 1;
                break;
            case '}': case ']':
                if (tokens == NULL)
                    break;
                type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
                if (factory->toknext < 1) {
                    return JSMN_ERROR_INVAL;
                }
                token = &tokens[factory->toknext - 1];
                for (;;) {
                    if (token->start != -1 && token->end == -1) {
                        if (token->type != type) {
                            return JSMN_ERROR_INVAL;
                        }
                        token->end = parser->pos + 1;
                        factory->toksuper = token->parent;
                        break;
                    }
                    if (token->parent == -1) {
                        if(token->type != type || factory->toksuper == -1) {
                            return JSMN_ERROR_INVAL;
                        }
                        break;
                    }
                    token = &tokens[token->parent];
                }
                break;
            case '\"':
                r = jsmn_parse_string(parser, js, len);
                if (r < 0) return r;
                count++;
                if (factory->toksuper != -1 && tokens != NULL)
                    tokens[factory->toksuper].size++;
                break;
            case '\t' : case '\r' : case '\n' : case ' ':
                break;
            case ':':
                factory->toksuper = factory->toknext - 1;
                break;
            case ',':
                if (tokens != NULL && factory->toksuper != -1 &&
                        tokens[factory->toksuper].type != JSMN_ARRAY &&
                        tokens[factory->toksuper].type != JSMN_OBJECT) {
                    factory->toksuper = tokens[factory->toksuper].parent;
                }
                break;
            // In strict mode primitives are: numbers and booleans
            case '-': case '0': case '1' : case '2': case '3' : case '4':
            case '5': case '6': case '7' : case '8': case '9':
            case 't': case 'f': case 'n' :
                // And they must not be keys of the object
                if (tokens != NULL && factory->toksuper != -1) {
                    jsmn_Token *t = &tokens[factory->toksuper];
                    if (t->type == JSMN_OBJECT ||
                            (t->type == JSMN_STRING && t->size != 0)) {
                        return JSMN_ERROR_INVAL;
                    }
                }
                r = jsmn_parse_primitive(parser, js, len);
                if (r < 0) return r;
                count++;
                if (factory->toksuper != -1 && tokens != NULL)
                    tokens[factory->toksuper].size++;
                break;

            // Unexpected char in strict mode
            default:
                return JSMN_ERROR_INVAL;
        }
    }

    if (tokens != NULL) {
        for (i = factory->toknext - 1; i >= 0; i--) {
            // Unmatched opened object or array
            if (tokens[i].start != -1 && tokens[i].end == -1) {
                return JSMN_ERROR_PART;
            }
        }
        for (i = 0; i < count; i++) {
            switch (tokens[i].type) {
                case JSMN_LABEL: case JSMN_STRING: case JSMN_PRIMITIVE:
                    // Calculate data pointer and length
                    tokens[i].data = js + tokens[i].start;
                    tokens[i].length = tokens[i].end - tokens[i].start;
                    break;
                default:
                    break;
            }
        }
    }

    return count;
}
