/* logging.h
 *	Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.
 *
 * This file is part of GnuPG.
 *
 * GnuPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GnuPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef LIBJNLIB_LOGGING_H
#define LIBJNLIB_LOGGING_H

#include <stdio.h>
#include "mischelp.h"


int  log_get_errorcount (int clear);
void log_set_file( const char *name );
void log_set_prefix (const char *text, unsigned int flags);
int  log_get_fd(void);
FILE *log_get_stream (void);

#ifdef JNLIB_GCC_M_FUNCTION
  void bug_at( const char *file, int line, const char *func ) JNLIB_GCC_A_NR;
# define BUG() bug_at( __FILE__ , __LINE__, __FUNCTION__ )
#else
  void bug_at( const char *file, int line );
# define BUG() bug_at( __FILE__ , __LINE__ )
#endif

/* To avoid mandatory inclusion of stdarg and other stuff, do it only
   if explicitly requested to do so. */
#ifdef JNLIB_NEED_LOG_LOGV
#include <stdarg.h>
enum jnlib_log_levels {
    JNLIB_LOG_BEGIN,
    JNLIB_LOG_CONT,
    JNLIB_LOG_INFO,
    JNLIB_LOG_WARN,
    JNLIB_LOG_ERROR,
    JNLIB_LOG_FATAL,
    JNLIB_LOG_BUG,
    JNLIB_LOG_DEBUG
};
void log_logv (int level, const char *fmt, va_list arg_ptr);
#endif /*JNLIB_NEED_LOG_LOGV*/


void log_bug( const char *fmt, ... )	JNLIB_GCC_A_NR_PRINTF(1,2);
void log_fatal( const char *fmt, ... )	JNLIB_GCC_A_NR_PRINTF(1,2);
void log_error( const char *fmt, ... )	JNLIB_GCC_A_PRINTF(1,2);
void log_info( const char *fmt, ... )	JNLIB_GCC_A_PRINTF(1,2);
void log_debug( const char *fmt, ... )	JNLIB_GCC_A_PRINTF(1,2);
void log_printf( const char *fmt, ... ) JNLIB_GCC_A_PRINTF(1,2);
void log_printhex (const char *text, const void *buffer, size_t length);


#endif /*LIBJNLIB_LOGGING_H*/





