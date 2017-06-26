/**
 * @file    util/jsmn.h
 *
 * @brief   JSMN a Minimalistic JSON Parser
 *
 * JSMN (pronounced like 'jasmine') is a minimalistic JSON parser in C. It can
 * be easily integrated into resource-limited or embedded projects.
 *
 * Philosophy
 * ----------
 *
 * Most JSON parsers offer you a bunch of functions to load JSON data, parse it
 * and extract any value by its name. jsmn proves that checking the correctness
 * of every JSON packet or allocating temporary objects to store parsed JSON
 * fields often is an overkill.
 *
 * Other Info
 * ----------
 *
 * This is not anymore the original code as it was. It has been extended and
 * modified by Neratec Solustions AG. The original source code can be found at
 * [github.com][1].
 *
 * [1]: https://github.com/zserge/jsmn/tree/35086597a72d94d8393e6a90b96e553
 */
/*-----------------------------------------------------------------------------
  Copyright (c) 2010 Serge A. Zaitsev

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  ---------------------------------------------------------------------------*/
#ifndef _UTIL_JSMN_H_
#define _UTIL_JSMN_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief JSON Type
 */
typedef enum {
    JSMN_UNDEFINED = 0,
    JSMN_OBJECT = 1,
    JSMN_ARRAY = 2,
    /** Label used within an object to name the value token */
    JSMN_LABEL = 3,
    JSMN_STRING = 4,
    /** Primitive: number, boolean (true/false) or null */
    JSMN_PRIMITIVE = 5
} jsmntype_t;

/**
 * @brief JSMN Error Definitions
 */
enum jsmnerr {
    /** Not enough tokens were provided */
    JSMN_ERROR_NOMEM = -1,
    /** Invalid character inside JSON string */
    JSMN_ERROR_INVAL = -2,
    /** The string is not a full JSON packet, more bytes expected */
    JSMN_ERROR_PART = -3,
    /** Something went wrong while composing the JSON tokens */
    JSMN_ERROR_FACTORY = -4
};

/**
 * @brief JSON Token
 */
typedef struct {
    /** JSMN Type (object, array, string etc.) */
    jsmntype_t type;
    const char *data;
    int length;
    /** Start position in JSON data string */
    int start;
    /** End position in JSON data string */
    int end;
    int size;
    int parent;
} jsmn_Token;

/**
 * @brief JSON Factory
 *
 * The factory is used to compose the JSON tokens either from a string by
 * parsing or by using functions like 'jsmn_start_object', 'jsmn_append_string'
 * and so on.
 */
typedef struct {
    jsmn_Token *toks; // array of tokens
    size_t tokslen; // length of token array toks
    unsigned int toknext; // next token to allocate
    int toksuper; // superior token node, e.g parent object or array
    int toklabel; // Label
} jsmn_Factory;

/**
 * @brief JSON Parser
 *
 * Contains a factory and the current position in the JSON string.
 */
typedef struct {
    jsmn_Factory factory;
    unsigned int pos; // offset in the JSON string
} jsmn_Parser;

/**
 * @brief Write Handler
 * 
 * Function pointer for writing JSON string to a buffer or device used during
 * the 'jsmn_dump' function.
 */
typedef int (*jsmn_write_handle_t)(const char *data, size_t length);

/**
 * @brief Initialise Factory
 */
void jsmn_factory_init(jsmn_Factory *factory, jsmn_Token *toks, size_t len);

/**
 * @brief Start a New JSON Object
 */
int jsmn_start_object(jsmn_Factory *factory, const char *name);

/**
 * @brief End the Current JSON Object
 */
int jsmn_end_object(jsmn_Factory *factory);

/**
 * @brief Start a New JSON Array
 */
int jsmn_start_array(jsmn_Factory *factory, const char *name);

/**
 * @brief End the Current JSON Array
 */
int jsmn_end_array(jsmn_Factory *factory);

/**
 * @brief Append a JSON String
 */
int jsmn_append_string(jsmn_Factory *factory, const char *name,
        const char *value);

/**
 * @brief Append a JSON Primitive
 */
int jsmn_append_primitive(jsmn_Factory *factory, const char *name,
        const char *value);

/**
 * @brief Dump JSMN Tokens as a JSON String.
 */
int jsmn_dump(jsmn_Token *t, jsmn_write_handle_t cb);

/**
 * @brief Initialise Parser
 */
void jsmn_parser_init(jsmn_Parser *parser, jsmn_Token *toks, size_t len);

/**
 * @brief Parse a JSON String to JSMN Tokens
 * 
 * It parses a JSON data string into and array of tokens, each describing a
 * single part of the JSON data.
 */
int jsmn_parse(jsmn_Parser *parser, const char *js, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _UTIL_JSMN_H_ */
