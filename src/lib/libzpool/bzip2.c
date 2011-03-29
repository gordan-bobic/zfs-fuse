/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/* #pragma ident	"%Z%%M%	%I%	%E% SMI" */

#include <sys/debug.h>
#include <sys/types.h>
#include <sys/zmod.h>

#ifdef _KERNEL
#include <sys/systm.h>
#else
#include <strings.h>
#endif
#include <bzlib.h>

size_t
bz2_compress(void *s_start, void *d_start, size_t s_len, size_t d_len, int n)
{
	ASSERT(d_len <= s_len);

	if (BZ2_bzBuffToBuffCompress(d_start, &d_len, s_start, s_len, n, 0, 50) != BZ_OK) {
		if (d_len != s_len) // unchanged in case of error
			return (s_len);

		bcopy(s_start, d_start, s_len);
		return (s_len);
	}

	return (d_len);
}

/*ARGSUSED*/
int
bz2_decompress(void *s_start, void *d_start, size_t s_len, size_t d_len, int n)
{
	ASSERT(d_len >= s_len);

	if (BZ2_bzBuffToBuffDecompress(d_start, &d_len, s_start, s_len, 0, 0) != Z_OK)
		return (-1);

	return (0);
}
