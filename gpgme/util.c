/* util.c
 *	Copyright (C) 2000 Werner Koch (dd9jn)
 *
 * This file is part of GPGME.
 *
 * GPGME is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPGME is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "util.h"

void *
_gpgme_malloc (size_t n )
{
    return malloc (n);
}

void *
_gpgme_calloc (size_t n, size_t m )
{
    return calloc (n, m);
}

void *
_gpgme_realloc (void *p, size_t n)
{
    return realloc (p, n );
}


char *
_gpgme_strdup (const char *p)
{
    return strdup (p);
}


void 
_gpgme_free ( void *a )
{
    free (a);
}




