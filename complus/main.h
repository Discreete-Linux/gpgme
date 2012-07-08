/* main.h -  GPGME COM+ component
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

#ifndef COMPLUS_MAIN_H
#define COMPLUS_MAIN_H

#include "xmalloc.h"
#include "stringhelp.h"
#include "logging.h"


#define _(a) (a)
#define N_(a) (a)


struct {
    int verbose;
    int quiet;
    unsigned int debug;
    char *homedir;
} opt;



#endif /* COMPLUS_MAIN_H */







