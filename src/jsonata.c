/*
 * Copyright (c) Quentin Young, 2020
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "duktape.h"
#include "jsonata.h"

static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
unsigned char *base64_decode(const unsigned char *src, size_t len,
			     size_t *out_len)
{
	unsigned char dtable[256], *out, *pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char)i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	pos = out = malloc(olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					free(out);
					return NULL;
				}
				break;
			}
		}
	}

	*out_len = pos - out;
	return out;
}

/*
 * Handle fatal errors in Duktape.
 *
 * Just prints an error message and aborts. This isn't really a handler, it's
 * just a breadcrumb. We should never ever hit this, it will abort pyjsonata.
 */
static void my_fatal(void *udata, const char *msg)
{
	/* ignored in this case, silence warning */
	(void)udata;

	/* Note that 'msg' may be NULL. */
	fprintf(stderr, "*** FATAL ERROR: %s\n", (msg ? msg : "no message"));
	fflush(stderr);

	/* This is bad. This kills the crab. */
	abort();
}


#define JSONATA_SUCCESS 0
#define JSONATA_ERR_UNSPEC 1
#define JSONATA_ERR_OOM 2
#define JSONATA_ERR_JSON_INVALID 3
#define JSONATA_ERR_INTERNAL_ERR 4
#define JSONATA_ERR_INVALID_ARGS 5
#define JSONATA_ERR_DUKTAPE_ERR 6

static const char *error_strings[] = {
	[JSONATA_ERR_UNSPEC] = "Unknown error",
	[JSONATA_ERR_OOM] = "Can't allocate memory",
	[JSONATA_ERR_JSON_INVALID] = "Invalid JSON",
	[JSONATA_ERR_INTERNAL_ERR] = "Internal error",
	[JSONATA_ERR_INVALID_ARGS] = "Invalid arguments",
	[JSONATA_ERR_DUKTAPE_ERR] = "Duktape error",
};

static char error_buf[BUFSIZ];

#define JSONATA_MAKE_ERRMSG(ec, message)                                       \
	snprintf(error_buf, sizeof(error_buf), "%s: %s", error_strings[(ec)],  \
		 (message))

#ifdef FUZZING
char *FuzzingJsonata;
size_t FuzzingJsonataLen;
#endif

int free_result(char *result)
{
	free(result);
	return 0;
}

/*
 * Evaluate JSONata expression on JSON.
 *
 * expression
 *    JSONata expression
 *
 * json
 *    Input JSON
 *
 * result
 *    On success, the result of evaluating expression on json.
 *    NULL on failure.
 *
 * error
 *    On failure, is either NULL or a human readable error string describing
 *    the failure.
 *    NULL on success.
 *
 * Returns:
 *    0 on success
 *    JSONATA_ERR_* otherwise
 *
 *    Upon success:
 *       *result SHALL be set, and will point to the result string.
 *       The caller MUST free this string.
 *
 *       The value of *error is NULL.
 *
 *    Upon failure:
 *       *error MAY be set. If it is set, it points to a static error message.
 *       If not, it is GUARANTEED to be NULL. The caller MUST NOT free this
 *       string.
 *
 * 	 The value of *result is NULL.
 *
 *    So, in sum, as long as your free() is POSIX compliant and may be called
 *    on NULL pointers:
 *
 *    - ALWAYS free(*result)
 *    - NEVER free(*error)
 *
 */
int jsonata(const char *expression, const char *json, char **result,
	    const char **error)
{
#define JSONATA_ERROR(ec, message)                                             \
	do {                                                                   \
		rc = (ec);                                                     \
		JSONATA_MAKE_ERRMSG((ec), (message));                          \
		*error = error_buf;                                            \
		goto done;                                                     \
	} while (0)

#define SAFE_STRDUP(dst, src)                                                  \
	do {                                                                   \
		(dst) = strdup((src));                                         \
		if (!dst) {                                                    \
			JSONATA_ERROR(JSONATA_ERR_OOM, (src));                 \
		}                                                              \
	} while (0)

	int rc = 1;
	char *jsonata_str = NULL;
	char *expression_clean = NULL;
	const char *tc = NULL;
	char *ot = NULL;
	char *prog = NULL;
	*result = NULL;
	*error = NULL;
	duk_context *ctx = NULL;
	size_t jsonata_len, progbufsz, progbuf_written;

	/* Check arguments */
	if (json == NULL || expression == NULL) {
		JSONATA_ERROR(JSONATA_ERR_INVALID_ARGS,
			      "Transform or data tree are NULL");
	}

	ctx = duk_create_heap(NULL, NULL, NULL, NULL, my_fatal);

	if (ctx == NULL) {
		JSONATA_ERROR(JSONATA_ERR_OOM, "Cannot create Duktape heap");
	}

	/* Decode parser library */
#ifndef FUZZING
	jsonata_str = (char *)base64_decode((const unsigned char *)jsonata_b64,
					    sizeof(jsonata_b64), &jsonata_len);
#else /* FUZZING */
	jsonata_str = FuzzingJsonata;
	jsonata_len = FuzzingJsonataLen;
#endif

	/* Check for decode or malloc failure */
	/* TODO: presently OOM case is mapped to INTERNAL_ERR */
	if (jsonata_str == NULL) {
		JSONATA_ERROR(JSONATA_ERR_INTERNAL_ERR,
			      "Cannot decode base64 jsonata");
	}

	/* Strip whitespace from expression */
	expression_clean = strdup(expression);
	if (expression_clean == NULL) {
		JSONATA_ERROR(JSONATA_ERR_OOM, "Cannot copy expression");
	}

	tc = ot = expression_clean;
	do
		while (*tc == '\n')
			++tc;
	while ((*ot++ = *tc++));

	/* Compute program buffer size */
	progbufsz = strlen(expression_clean);
	progbufsz += strlen(json);
	progbufsz += 256;
	prog = calloc(progbufsz, 1);

	/* Check for calloc failure */
	if (prog == NULL) {
		JSONATA_ERROR(JSONATA_ERR_OOM,
			      "Cannot allocate program buffer");
	}

	/* Write program */
	progbuf_written = snprintf(
		prog, progbufsz, "JSON.stringify(jsonata('%s').evaluate(%s));",
		expression_clean, json);

	/* Check for buffer overflow */
	if (progbuf_written >= progbufsz) {
		JSONATA_ERROR(
			JSONATA_ERR_INTERNAL_ERR,
			"Program buffer size insufficient for expression program");
	}

	/* Load jsonata into context */
	if (duk_peval_string(ctx, jsonata_str) != 0) {
		JSONATA_ERROR(JSONATA_ERR_DUKTAPE_ERR,
			      duk_safe_to_string(ctx, -1));
	}

	/* Run expression program */
	if (duk_peval_string(ctx, prog) != 0) {
		JSONATA_ERROR(JSONATA_ERR_DUKTAPE_ERR,
			      duk_safe_to_string(ctx, -1));
	} else {
		SAFE_STRDUP(*result, duk_safe_to_string(ctx, -1));
		rc = JSONATA_SUCCESS;
	}

done:
#ifndef FUZZING
	free(jsonata_str);
#endif
	free(prog);
	free(expression_clean);
	duk_destroy_heap(ctx);

	/* Handle the error of not handling errors */
	if (!*result && !*error) {
		rc = JSONATA_ERR_INTERNAL_ERR;
		JSONATA_MAKE_ERRMSG(JSONATA_ERR_INTERNAL_ERR,
				    "No result or other error message");
		*error = error_buf;
	}

	return rc;
}

#ifdef FUZZING

static int read_text_file(const char *filename, char **input)
{
	long fsize;
	int read;
	FILE *fp = fopen(filename, "r");

	if (fp == NULL)
		return -1;

	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	*input = calloc(fsize + 1, 1);
	fseek(fp, 0, SEEK_SET);
	read = fread(*input, 1, fsize, fp);
	fclose(fp);

	return read;
}

char *argument;

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
	const char *need =
#if defined(FUZZ_EXPRESSION)
		"input JSON";
#else
		"JSONata expression";
#endif
	if (*argc < 2) {
		fprintf(stderr, "Need to provide file path to %s\n", need);
		exit(1);
	}

	fprintf(stderr, "Loading argument file '%s'\n", (*argv)[*argc - 1]);
	read_text_file((*argv)[*argc - 1], &argument);
	*argc -= 1;

	/* Pre-decode jsonata.js to speed up fuzzing */
	FuzzingJsonata =
		(char *)base64_decode((const unsigned char *)jsonata_b64,
				      sizeof(jsonata_b64), &FuzzingJsonataLen);

	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t len)
{
	char *result = NULL;
	const char *error = NULL;

	/* Make C string of fuzzing data */
	char *str = calloc(len + 1, 1);
	if (!str) {
		abort();
	}

	memcpy(str, data, len);
	str[len] = '\0';

	int rc;

#if defined(FUZZ_EXPRESSION)
	rc = jsonata(str, argument, &result, &error);
#elif defined(FUZZ_JSON)
	rc = jsonata(argument, str, &result, &error);
#endif

	if (result) {
		fprintf(stderr, "Result: %s\n", result);
	}
	if (error) {
		fprintf(stderr, "Error[%d]: %s\n", rc, error);
	}

	free(result);
	free(str);

	return 0;
}

#endif /* FUZZING */
