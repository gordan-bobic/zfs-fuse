/*
 * xz_pipe_comp.c
 * A simple example of pipe-only xz compressor implementation.
 * version: 2010-07-12 - by Daniel Mealha Cabrita
 * Not copyrighted -- provided to the public domain.
 *
 * Compiling:
 * Link with liblzma. GCC example:
 * $ gcc -llzma xz_pipe_comp.c -o xz_pipe_comp
 *
 * Usage example:
 * $ cat some_file | ./xz_pipe_comp > some_file.xz
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <lzma.h>
#include <syslog.h>
#include <string.h>

/* COMPRESSION SETTINGS */

/* boolean setting, analogous to xz CLI option: -e */
#define COMPRESSION_EXTREME false

/* see: /usr/include/lzma/check.h LZMA_CHECK_* */
#define INTEGRITY_CHECK LZMA_CHECK_NONE

/* error codes */
#define RET_OK			0
#define RET_ERROR_INIT		1
#define RET_ERROR_INPUT		2
#define RET_ERROR_OUTPUT	3
#define RET_ERROR_COMPRESSION	4

size_t
lzma_compress(void *s_start, void *d_start, size_t s_len, size_t d_len, int level)
{
	uint32_t preset = level | (COMPRESSION_EXTREME ? LZMA_PRESET_EXTREME : 0);
	lzma_check check = INTEGRITY_CHECK;
	lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
	lzma_action action;
	lzma_ret ret_xz;

	/* initialize xz encoder */
	ret_xz = lzma_easy_encoder (&strm, preset, check);
	if (ret_xz != LZMA_OK) {
		syslog (LOG_WARNING, "lzma_easy_encoder error: %d\n", (int) ret_xz);
		if (d_len == s_len)
			memcpy(d_start, s_start, s_len);
		return s_len;
	}

	strm.next_in = s_start;
	strm.avail_in = s_len;

	strm.next_out = d_start;
	strm.avail_out = d_len;

	/* compress data */
	ret_xz = lzma_code (&strm, LZMA_RUN);
	if (ret_xz == LZMA_OK) 
		ret_xz = lzma_code (&strm, LZMA_FINISH);
	lzma_end (&strm);

	if ((ret_xz != LZMA_STREAM_END)) {
		if (d_len == s_len)
			memcpy(d_start, s_start, s_len);
		return s_len;
	}
	/* write compressed data */
	return d_len - strm.avail_out;
}

/*ARGSUSED*/
int
lzma_decompress(void *s_start, void *d_start, size_t s_len, size_t d_len, int n)
{
	lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
	const uint32_t flags = LZMA_TELL_UNSUPPORTED_CHECK | LZMA_CONCATENATED;
	const uint64_t memory_limit = UINT64_MAX; /* no memory limit */
	lzma_action action;
	lzma_ret ret_xz;

	/* initialize xz decoder */
	ret_xz = lzma_stream_decoder (&strm, memory_limit, flags);
	if (ret_xz != LZMA_OK) {
		syslog (LOG_WARNING, "lzma_stream_decoder error: %d\n", (int) ret_xz);
		return -1;
	}

	strm.next_in = s_start;
	strm.avail_in = s_len;

	strm.next_out = d_start;
	strm.avail_out = d_len;

	/* decompress data */
	ret_xz = lzma_code (&strm, LZMA_RUN);
	if (ret_xz == LZMA_OK)
		ret_xz = lzma_code (&strm, LZMA_FINISH);

	lzma_end (&strm);
	if ((ret_xz != LZMA_OK) && (ret_xz != LZMA_STREAM_END)) {
		return -1;
	}
	return 0;
}

