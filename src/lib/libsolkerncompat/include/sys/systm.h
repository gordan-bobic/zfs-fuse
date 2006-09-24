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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Copyright 2006 Ricardo Correia.
 * Use is subject to license terms.
 */

#ifndef _SYS_SYSTM_H
#define _SYS_SYSTM_H

#include <sys/debug.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/dditypes.h>

#include <string.h>

extern uint64_t physmem;

#define	lbolt	(gethrtime() >> 23)
#define	lbolt64	(gethrtime() >> 23)
#define	hz	119	/* frequency when using gethrtime() >> 23 for lbolt */

extern struct vnode *rootdir;	/* pointer to vnode of root directory */

extern void delay(clock_t ticks);

/*
 * These must be implemented in the program itself.
 * For zfs-fuse, take a look at zfs-fuse/zfsfuse_ioctl.c
 */
extern int xcopyin(const void *src, void *dest, size_t size);
extern int xcopyout(const void *src, void *dest, size_t size);
#define copyout(kaddr, uaddr, count) xcopyout(kaddr, uaddr, count)

#endif
