#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../jsmn.h"

/*
 * A small example of jsmn parsing when JSON structure is known and number of
 * tokens is predictable.
 */

static const char *JSON_STRING =
	"{\"user\": \"johndoe\", \"admin\": false, \"uid\": 1000,\n  "
	"\"groups\": [\"users\", \"wheel\", \"audio\", \"video\"]}";

static int jsoneq(jsmn_Token *tok, const char *s) {
	if (tok->type == JSMN_LABEL && strncmp(tok->data, s, tok->length) == 0) {
		return 0;
	}
	return -1;
}

int main() {
	int i;
	int r;
	jsmn_Parser p;
	jsmn_Token t[128]; /* We expect no more than 128 tokens */

	jsmn_parser_init(&p, t, sizeof(t)/sizeof(t[0]));
	r = jsmn_parse(&p, JSON_STRING, strlen(JSON_STRING));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		printf("Object expected\n");
		return 1;
	}

	/* Loop over all keys of the root object */
	for (i = 1; i < r; i++) {
		if (jsoneq(&t[i], "user") == 0) {
			/* We may use strndup() to fetch string value */
			printf("- User: %.*s\n", t[i+1].length, t[i+1].data);
			i++;
		} else if (jsoneq(&t[i], "admin") == 0) {
			/* We may additionally check if the value is either "true" or "false" */
			printf("- Admin: %.*s\n", t[i+1].length, t[i+1].data);
			i++;
		} else if (jsoneq(&t[i], "uid") == 0) {
			/* We may want to do strtol() here to get numeric value */
			printf("- UID: %.*s\n", t[i+1].length, t[i+1].data);
			i++;
		} else if (jsoneq(&t[i], "groups") == 0) {
			int j;
			printf("- Groups:\n");
			if (t[i+1].type != JSMN_ARRAY) {
				continue; /* We expect groups to be an array of strings */
			}
			for (j = 0; j < t[i+1].size; j++) {
				jsmn_Token *g = &t[i+j+2];
				printf("  * %.*s\n", g->length, g->data);
			}
			i += t[i+1].size + 1;
		} else {
			printf("Unexpected key: %.*s\n", t[i+1].length, t[i+1].data);
		}
	}
	return EXIT_SUCCESS;
}
