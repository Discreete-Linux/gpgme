/* stringhelp.c -  standard string helper functions
 *	Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
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

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "libjnlib-config.h"
#include "stringhelp.h"


/****************
 * look for the substring SUB in buffer and return a pointer to that
 * substring in BUF or NULL if not found.
 * Comparison is case-insensitive.
 */
const char *
memistr( const char *buf, size_t buflen, const char *sub )
{
    const byte *t, *s ;
    size_t n;

    for( t=buf, n=buflen, s=sub ; n ; t++, n-- )
	if( toupper(*t) == toupper(*s) ) {
	    for( buf=t++, buflen = n--, s++;
		 n && toupper(*t) == toupper(*s); t++, s++, n-- )
		;
	    if( !*s )
		return buf;
	    t = buf; n = buflen; s = sub ;
	}

    return NULL ;
}

/****************
 * Wie strncpy(), aber es werden maximal n-1 zeichen kopiert und ein
 * '\0' angeh�ngt. Ist n = 0, so geschieht nichts, ist Destination
 * gleich NULL, so wird via jnlib_xmalloc Speicher besorgt, ist dann nicht
 * gen�gend Speicher vorhanden, so bricht die funktion ab.
 */
char *
mem2str( char *dest , const void *src , size_t n )
{
    char *d;
    const char *s;

    if( n ) {
	if( !dest )
	    dest = jnlib_xmalloc( n ) ;
	d = dest;
	s = src ;
	for(n--; n && *s; n-- )
	    *d++ = *s++;
	*d = '\0' ;
    }

    return dest ;
}


/****************
 * remove leading and trailing white spaces
 */
char *
trim_spaces( char *str )
{
    char *string, *p, *mark;

    string = str;
    /* find first non space character */
    for( p=string; *p && isspace( *(byte*)p ) ; p++ )
	;
    /* move characters */
    for( (mark = NULL); (*string = *p); string++, p++ )
	if( isspace( *(byte*)p ) ) {
	    if( !mark )
		mark = string ;
	}
	else
	    mark = NULL ;
    if( mark )
	*mark = '\0' ;  /* remove trailing spaces */

    return str ;
}

/****************
 * remove trailing white spaces
 */
char *
trim_trailing_spaces( char *string )
{
    char *p, *mark;

    for( mark = NULL, p = string; *p; p++ ) {
	if( isspace( *(byte*)p ) ) {
	    if( !mark )
		mark = p;
	}
	else
	    mark = NULL;
    }
    if( mark )
	*mark = '\0' ;

    return string ;
}



unsigned
trim_trailing_chars( byte *line, unsigned len, const char *trimchars )
{
    byte *p, *mark;
    unsigned n;

    for(mark=NULL, p=line, n=0; n < len; n++, p++ ) {
	if( strchr(trimchars, *p ) ) {
	    if( !mark )
		mark = p;
	}
	else
	    mark = NULL;
    }

    if( mark ) {
	*mark = 0;
	return mark - line;
    }
    return len;
}

/****************
 * remove trailing white spaces and return the length of the buffer
 */
unsigned
trim_trailing_ws( byte *line, unsigned len )
{
    return trim_trailing_chars( line, len, " \t\r\n" );
}


/***************
 * Extract from a given path the filename component.
 *
 */
char *
make_basename(const char *filepath)
{
    char *p;

    if ( !(p=strrchr(filepath, '/')) )
      #ifdef HAVE_DRIVE_LETTERS
	if ( !(p=strrchr(filepath, '\\')) )
	    if ( !(p=strrchr(filepath, ':')) )
      #endif
	      {
		return jnlib_xstrdup(filepath);
	      }

    return jnlib_xstrdup(p+1);
}



/***************
 * Extract from a given filename the path prepended to it.
 * If their isn't a path prepended to the filename, a dot
 * is returned ('.').
 *
 */
char *
make_dirname(const char *filepath)
{
    char *dirname;
    int  dirname_length;
    char *p;

    if ( !(p=strrchr(filepath, '/')) )
      #ifdef HAVE_DRIVE_LETTERS
	if ( !(p=strrchr(filepath, '\\')) )
	    if ( !(p=strrchr(filepath, ':')) )
      #endif
	      {
		return jnlib_xstrdup(".");
	      }

    dirname_length = p-filepath;
    dirname = jnlib_xmalloc(dirname_length+1);
    strncpy(dirname, filepath, dirname_length);
    dirname[dirname_length] = 0;

    return dirname;
}



/****************
 * Construct a filename from the NULL terminated list of parts.
 * Tilde expansion is done here.
 */
char *
make_filename( const char *first_part, ... )
{
    va_list arg_ptr ;
    size_t n;
    const char *s;
    char *name, *home, *p;

    va_start( arg_ptr, first_part ) ;
    n = strlen(first_part)+1;
    while( (s=va_arg(arg_ptr, const char *)) )
	n += strlen(s) + 1;
    va_end(arg_ptr);

    home = NULL;
    if( *first_part == '~' && first_part[1] == '/'
			   && (home = getenv("HOME")) && *home )
	n += strlen(home);

    name = jnlib_xmalloc(n);
    p = home ? stpcpy(stpcpy(name,home), first_part+1)
	     : stpcpy(name, first_part);
    va_start( arg_ptr, first_part ) ;
    while( (s=va_arg(arg_ptr, const char *)) )
	p = stpcpy(stpcpy(p,"/"), s);
    va_end(arg_ptr);

    return name;
}


int
compare_filenames( const char *a, const char *b )
{
    /* ? check whether this is an absolute filename and
     * resolve symlinks?
     */
  #ifdef HAVE_DRIVE_LETTERS
    return stricmp(a,b);
  #else
    return strcmp(a,b);
  #endif
}


/****************************************************
 ******** locale insensitive ctype functions ********
 ****************************************************/
/* FIXME: replace them by a table lookup and macros */
int
ascii_isupper (int c)
{
    return c >= 'A' && c <= 'Z';
}

int
ascii_islower (int c)
{
    return c >= 'a' && c <= 'z';
}

int 
ascii_toupper (int c)
{
    if (c >= 'a' && c <= 'z')
        c &= ~0x20;
    return c;
}

int 
ascii_tolower (int c)
{
    if (c >= 'A' && c <= 'Z')
        c |= 0x20;
    return c;
}


int
ascii_strcasecmp( const char *a, const char *b )
{
    if (a == b)
        return 0;

    for (; *a && *b; a++, b++) {
	if (*a != *b && ascii_toupper(*a) != ascii_toupper(*b))
	    break;
    }
    return *a == *b? 0 : (ascii_toupper (*a) - ascii_toupper (*b));
}

int
ascii_memcasecmp( const char *a, const char *b, size_t n )
{
    if (a == b)
        return 0;
    for ( ; n; n--, a++, b++ ) {
	if( *a != *b  && ascii_toupper (*a) != ascii_toupper (*b) )
            return *a == *b? 0 : (ascii_toupper (*a) - ascii_toupper (*b));
    }
    return 0;
}

int
ascii_strcmp( const char *a, const char *b )
{
    if (a == b)
        return 0;

    for (; *a && *b; a++, b++) {
	if (*a != *b )
	    break;
    }
    return *a == *b? 0 : (*(signed char *)a - *(signed char *)b);
}



/*********************************************
 ********** missing string functions *********
 *********************************************/

#ifndef HAVE_STPCPY
char *
stpcpy(char *a,const char *b)
{
    while( *b )
	*a++ = *b++;
    *a = 0;

    return (char*)a;
}
#endif

#ifndef HAVE_STRLWR
char *
strlwr(char *s)
{
    char *p;
    for(p=s; *p; p++ )
	*p = tolower(*p);
    return s;
}
#endif


#ifndef HAVE_STRCASECMP
int
strcasecmp( const char *a, const char *b )
{
    for( ; *a && *b; a++, b++ ) {
	if( *a != *b && toupper(*a) != toupper(*b) )
	    break;
    }
    return *(const byte*)a - *(const byte*)b;
}
#endif


/****************
 * mingw32/cpd has a memicmp()
 */
#ifndef HAVE_MEMICMP
int
memicmp( const char *a, const char *b, size_t n )
{
    for( ; n; n--, a++, b++ )
	if( *a != *b  && toupper(*(const byte*)a) != toupper(*(const byte*)b) )
	    return *(const byte *)a - *(const byte*)b;
    return 0;
}
#endif



