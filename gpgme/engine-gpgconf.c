/* engine-gpgconf.c - gpg-conf engine.
   Copyright (C) 2000 Werner Koch (dd9jn)
   Copyright (C) 2001, 2002, 2003, 2004, 2005, 2007, 2008 g10 Code GmbH
 
   This file is part of GPGME.

   GPGME is free software; you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.
   
   GPGME is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h> /* FIXME */
#include <errno.h>

#include "gpgme.h"
#include "util.h"
#include "ops.h"
#include "wait.h"
#include "priv-io.h"
#include "sema.h"

#include "assuan.h"
#include "debug.h"

#include "engine-backend.h"


struct engine_gpgconf
{
  char *file_name;
  char *home_dir;
};

typedef struct engine_gpgconf *engine_gpgconf_t;


static char *
gpgconf_get_version (const char *file_name)
{
  return _gpgme_get_program_version (file_name ? file_name
				     : _gpgme_get_gpgconf_path ());
}


static const char *
gpgconf_get_req_version (void)
{
  return NEED_GPGCONF_VERSION;
}


static void
gpgconf_release (void *engine)
{
  engine_gpgconf_t gpgconf = engine;

  if (!gpgconf)
    return;

  if (gpgconf->file_name)
    free (gpgconf->file_name);
  if (gpgconf->home_dir)
    free (gpgconf->home_dir);

  free (gpgconf);
}


static gpgme_error_t
gpgconf_new (void **engine, const char *file_name, const char *home_dir)
{
  gpgme_error_t err = 0;
  engine_gpgconf_t gpgconf;

  gpgconf = calloc (1, sizeof *gpgconf);
  if (!gpgconf)
    return gpg_error_from_errno (errno);

  gpgconf->file_name = strdup (file_name ? file_name
			       : _gpgme_get_gpgconf_path ());
  if (!gpgconf->file_name)
    err = gpg_error_from_syserror ();

  if (!err && home_dir)
    {
      gpgconf->home_dir = strdup (home_dir);
      if (!gpgconf->home_dir)
	err = gpg_error_from_syserror ();
    }

  if (err)
    gpgconf_release (gpgconf);
  else
    *engine = gpgconf;

  return err;
}


static void
release_arg (gpgme_conf_arg_t arg, gpgme_conf_type_t alt_type)
{
  while (arg)
    {
      gpgme_conf_arg_t next = arg->next;

      if (alt_type == GPGME_CONF_STRING)
	free (arg->value.string);
      free (arg);
      arg = next;
    }
}


static void
release_opt (gpgme_conf_opt_t opt)
{
  if (opt->name)
    free (opt->name);
  if (opt->description)
    free (opt->description);
  if (opt->argname)
    free (opt->argname);

  release_arg (opt->default_value, opt->alt_type);
  if (opt->default_description)
    free (opt->default_description);
  
  release_arg (opt->no_arg_value, opt->alt_type);
  release_arg (opt->value, opt->alt_type);
  release_arg (opt->new_value, opt->alt_type);

  free (opt);
}


static void
release_comp (gpgme_conf_comp_t comp)
{
  gpgme_conf_opt_t opt;

  if (comp->name)
    free (comp->name);
  if (comp->description)
    free (comp->description);
  if (comp->program_name)
    free (comp->program_name);

  opt = comp->options;
  while (opt)
    {
      gpgme_conf_opt_t next = opt->next;
      release_opt (opt);
      opt = next;
    }

  free (comp);
}


static void
gpgconf_config_release (gpgme_conf_comp_t conf)
{
  while (conf)
    {
      gpgme_conf_comp_t next = conf->next;
      release_comp (conf);
      conf = next;
    }
}


static gpgme_error_t
gpgconf_read (void *engine, char *arg1, char *arg2,
	      gpgme_error_t (*cb) (void *hook, char *line),
	      void *hook)
{
  struct engine_gpgconf *gpgconf = engine;
  gpgme_error_t err = 0;
#define LINELENGTH 1024
  char linebuf[LINELENGTH] = "";
  int linelen = 0;
  char *argv[4] = { NULL /* file_name */, NULL, NULL, NULL };
  int rp[2];
  struct spawn_fd_item_s cfd[] = { {-1, 1 /* STDOUT_FILENO */, -1, 0},
				   {-1, -1} };
  int status;
  int nread;
  char *mark = NULL;

  argv[1] = arg1;
  argv[2] = arg2;


  /* FIXME: Deal with engine->home_dir.  */

  /* _gpgme_engine_new guarantees that this is not NULL.  */
  argv[0] = gpgconf->file_name;
  
  if (_gpgme_io_pipe (rp, 1) < 0)
    return gpg_error_from_syserror ();

  cfd[0].fd = rp[1];

  status = _gpgme_io_spawn (gpgconf->file_name, argv, cfd, NULL);
  if (status < 0)
    {
      _gpgme_io_close (rp[0]);
      _gpgme_io_close (rp[1]);
      return gpg_error_from_syserror ();
    }

  do
    {
      nread = _gpgme_io_read (rp[0], 
                              linebuf + linelen, LINELENGTH - linelen - 1);
      if (nread > 0)
	{
          char *line;
          const char *lastmark = NULL;
          size_t nused;

	  linelen += nread;
	  linebuf[linelen] = '\0';

	  for (line=linebuf; (mark = strchr (line, '\n')); line = mark+1 )
	    {
              lastmark = mark;
	      if (mark > line && mark[-1] == '\r')
		mark[-1] = '\0';
              else
                mark[0] = '\0';

	      /* Got a full line.  Due to the CR removal code (which
                 occurs only on Windows) we might be one-off and thus
                 would see empty lines.  Don't pass them to the
                 callback. */
	      err = *line? (*cb) (hook, line) : 0;
	      if (err)
		goto leave;
	    }

          nused = lastmark? (lastmark + 1 - linebuf) : 0;
          memmove (linebuf, linebuf + nused, linelen - nused);
          linelen -= nused;
	}
    }
  while (nread > 0 && linelen < LINELENGTH - 1);
  
  if (!err && nread < 0)
    err = gpg_error_from_syserror ();
  if (!err && nread > 0)
    err = gpg_error (GPG_ERR_LINE_TOO_LONG);

 leave:
  _gpgme_io_close (rp[0]);

  return err;
}


static gpgme_error_t
gpgconf_config_load_cb (void *hook, char *line)
{
  gpgme_conf_comp_t *comp_p = hook;
  gpgme_conf_comp_t comp = *comp_p;
#define NR_FIELDS 16
  char *field[NR_FIELDS];
  int fields = 0;

  while (line && fields < NR_FIELDS)
    {
      field[fields++] = line;
      line = strchr (line, ':');
      if (line)
	*(line++) = '\0';
    }

  /* We require at least the first 3 fields.  */
  if (fields < 2)
    return gpg_error (GPG_ERR_INV_ENGINE);

  /* Find the pointer to the new component in the list.  */
  while (comp && comp->next)
    comp = comp->next;
  if (comp)
    comp_p = &comp->next;

  comp = calloc (1, sizeof (*comp));
  if (!comp)
    return gpg_error_from_syserror ();
  /* Prepare return value.  */
  comp->_last_opt_p = &comp->options;
  *comp_p = comp;

  comp->name = strdup (field[0]);
  if (!comp->name)
    return gpg_error_from_syserror ();

  comp->description = strdup (field[1]);
  if (!comp->description)
    return gpg_error_from_syserror ();

  if (fields >= 3)
    {
      comp->program_name = strdup (field[2]);
      if (!comp->program_name)
	return gpg_error_from_syserror ();
    }

  return 0;
}


static gpgme_error_t
gpgconf_parse_option (gpgme_conf_opt_t opt,
		      gpgme_conf_arg_t *arg_p, char *line)
{
  gpgme_error_t err;
  char *mark;

  if (!line[0])
    return 0;

  while (line)
    {
      gpgme_conf_arg_t arg;

      mark = strchr (line, ',');
      if (mark)
	*mark = '\0';

      arg = calloc (1, sizeof (*arg));
      if (!arg)
	return gpg_error_from_syserror ();
      *arg_p = arg;
      arg_p = &arg->next;

      if (*line == '\0')
	arg->no_arg = 1;
      else
	{
	  switch (opt->alt_type)
	    {
	      /* arg->value.count is an alias for arg->value.uint32.  */
	    case GPGME_CONF_NONE:
	    case GPGME_CONF_UINT32:
	      arg->value.uint32 = strtoul (line, NULL, 0);
	      break;
	      
	    case GPGME_CONF_INT32:
	      arg->value.uint32 = strtol (line, NULL, 0);
	      break;
	      
	    case GPGME_CONF_STRING:
              /* The complex types below are only here to silent the
                 compiler warning. */
            case GPGME_CONF_FILENAME: 
            case GPGME_CONF_LDAP_SERVER:
            case GPGME_CONF_KEY_FPR:
            case GPGME_CONF_PUB_KEY:
            case GPGME_CONF_SEC_KEY:
            case GPGME_CONF_ALIAS_LIST:
	      /* Skip quote character.  */
	      line++;
	      
	      err = _gpgme_decode_percent_string (line, &arg->value.string,
						  0, 0);
	      if (err)
		return err;
	      break;
	    }
	}

      /* Find beginning of next value.  */
      if (mark++ && *mark)
	line = mark;
      else
	line = NULL;
    }

  return 0;
}


static gpgme_error_t
gpgconf_config_load_cb2 (void *hook, char *line)
{
  gpgme_error_t err;
  gpgme_conf_comp_t comp = hook;
  gpgme_conf_opt_t *opt_p = comp->_last_opt_p;
  gpgme_conf_opt_t opt;
#define NR_FIELDS 16
  char *field[NR_FIELDS];
  int fields = 0;

  while (line && fields < NR_FIELDS)
    {
      field[fields++] = line;
      line = strchr (line, ':');
      if (line)
	*(line++) = '\0';
    }

  /* We require at least the first 10 fields.  */
  if (fields < 10)
    return gpg_error (GPG_ERR_INV_ENGINE);

  opt = calloc (1, sizeof (*opt));
  if (!opt)
    return gpg_error_from_syserror ();

  comp->_last_opt_p = &opt->next;
  *opt_p = opt;

  if (field[0][0])
    {
      opt->name = strdup (field[0]);
      if (!opt->name)
	return gpg_error_from_syserror ();
    }

  opt->flags = strtoul (field[1], NULL, 0);

  opt->level = strtoul (field[2], NULL, 0);

  if (field[3][0])
    {
      opt->description = strdup (field[3]);
      if (!opt->description)
	return gpg_error_from_syserror ();
    }

  opt->type = strtoul (field[4], NULL, 0);

  opt->alt_type = strtoul (field[5], NULL, 0);

  if (field[6][0])
    {
      opt->argname = strdup (field[6]);
      if (!opt->argname)
	return gpg_error_from_syserror ();
    }

  if (opt->flags & GPGME_CONF_DEFAULT)
    {
      err = gpgconf_parse_option (opt, &opt->default_value, field[7]);
      if (err)
	return err;
    }
  else if ((opt->flags & GPGME_CONF_DEFAULT_DESC) && field[7][0])
    {
      opt->default_description = strdup (field[7]);
      if (!opt->default_description)
	return gpg_error_from_syserror ();
    }

  if (opt->flags & GPGME_CONF_NO_ARG_DESC)
    {
      opt->no_arg_description = strdup (field[8]);
      if (!opt->no_arg_description)
	return gpg_error_from_syserror ();
    }
  else
    {
      err = gpgconf_parse_option (opt, &opt->no_arg_value, field[8]);
      if (err)
	return err;
    }

  err = gpgconf_parse_option (opt, &opt->value, field[9]);
  if (err)
    return err;

  return 0;
}


static gpgme_error_t
gpgconf_conf_load (void *engine, gpgme_conf_comp_t *comp_p)
{
  gpgme_error_t err;
  gpgme_conf_comp_t comp = NULL;
  gpgme_conf_comp_t cur_comp;

  *comp_p = NULL;

  err = gpgconf_read (engine, "--list-components", NULL,
		      gpgconf_config_load_cb, &comp);
  if (err)
    {
      gpgconf_release (comp);
      return err;
    }

  cur_comp = comp;
  while (!err && cur_comp)
    {
      err = gpgconf_read (engine, "--list-options", cur_comp->name,
			  gpgconf_config_load_cb2, cur_comp);
      cur_comp = cur_comp->next;
    }

  if (err)
    {
      gpgconf_release (comp);
      return err;
    }

  *comp_p = comp;
  return 0;
}



gpgme_error_t
_gpgme_conf_arg_new (gpgme_conf_arg_t *arg_p,
		     gpgme_conf_type_t type, void *value)
{
  gpgme_conf_arg_t arg;

  arg = calloc (1, sizeof (*arg));
  if (!arg)
    return gpg_error_from_syserror ();

  if (!value)
    arg->no_arg = 1;
  else
    {
      /* We need to switch on type here because the alt-type is not
         yet known.  */
      switch (type)
	{
	case GPGME_CONF_NONE:
	case GPGME_CONF_UINT32:
	  arg->value.uint32 = *((unsigned int *) value);
	  break;
	  
	case GPGME_CONF_INT32:
	  arg->value.int32 = *((int *) value);
	  break;
	  
	case GPGME_CONF_STRING:
	case GPGME_CONF_FILENAME:
	case GPGME_CONF_LDAP_SERVER:
        case GPGME_CONF_KEY_FPR:
        case GPGME_CONF_PUB_KEY:
        case GPGME_CONF_SEC_KEY:
        case GPGME_CONF_ALIAS_LIST:
	  arg->value.string = strdup (value);
	  if (!arg->value.string)
	    {
	      free (arg);
	      return gpg_error_from_syserror ();
	    }
	  break;
	  
	default:
	  free (arg);
	  return gpg_error (GPG_ERR_INV_VALUE);
	}
    }

  *arg_p = arg;
  return 0;
}


void
_gpgme_conf_arg_release (gpgme_conf_arg_t arg, gpgme_conf_type_t type)
{
  /* Lacking the alt_type we need to switch on type here.  */
  switch (type)
    {
    case GPGME_CONF_NONE:
    case GPGME_CONF_UINT32:
    case GPGME_CONF_INT32:
    case GPGME_CONF_STRING:
    default:
      break;
       
    case GPGME_CONF_FILENAME:
    case GPGME_CONF_LDAP_SERVER:
    case GPGME_CONF_KEY_FPR:
    case GPGME_CONF_PUB_KEY:
    case GPGME_CONF_SEC_KEY:
    case GPGME_CONF_ALIAS_LIST:
      type = GPGME_CONF_STRING;
      break;
    }

  release_arg (arg, type);
}


gpgme_error_t
_gpgme_conf_opt_change (gpgme_conf_opt_t opt, int reset, gpgme_conf_arg_t arg)
{
  if (opt->new_value)
    release_arg (opt->new_value, opt->alt_type);

  if (reset)
    {
      opt->new_value = NULL;
      opt->change_value = 0;
    }
  else
    {
      opt->new_value = arg;
      opt->change_value = 1;
    }
  return 0;
}


/* FIXME: Major problem: We don't get errors from gpgconf.  */

static gpgme_error_t
gpgconf_write (void *engine, char *arg1, char *arg2, gpgme_data_t conf)
{
  struct engine_gpgconf *gpgconf = engine;
  gpgme_error_t err = 0;
#define BUFLEN 1024
  char buf[BUFLEN];
  int buflen = 0;
  char *argv[] = { NULL /* file_name */, arg1, arg2, 0 };
  int rp[2];
  struct spawn_fd_item_s cfd[] = { {-1, 0 /* STDIN_FILENO */}, {-1, -1} };
  int status;
  int nwrite;

  /* FIXME: Deal with engine->home_dir.  */

  /* _gpgme_engine_new guarantees that this is not NULL.  */
  argv[0] = gpgconf->file_name;
  argv[0] = "/nowhere/path-needs-to-be-fixed/gpgconf";

  if (_gpgme_io_pipe (rp, 0) < 0)
    return gpg_error_from_syserror ();

  cfd[0].fd = rp[0];

  status = _gpgme_io_spawn (gpgconf->file_name, argv, cfd, NULL);
  if (status < 0)
    {
      _gpgme_io_close (rp[0]);
      _gpgme_io_close (rp[1]);
      return gpg_error_from_syserror ();
    }

  for (;;)
    {
      if (buflen == 0)
	{
	  do
	    {
	      buflen = gpgme_data_read (conf, buf, BUFLEN);
	    }
	  while (buflen < 0 && errno == EAGAIN);

	  if (buflen < 0)
	    {
	      err = gpg_error_from_syserror ();
	      _gpgme_io_close (rp[1]);
	      return err;
	    }
	  else if (buflen == 0)
	    {
	      /* All is written.  */
	      _gpgme_io_close (rp[1]);
	      return 0;
	    }
	}

      do
	{
	  nwrite = _gpgme_io_write (rp[1], buf, buflen);
	}
      while (nwrite < 0 && errno == EAGAIN);

      if (nwrite > 0)
	{
	  buflen -= nwrite;
	  if (buflen > 0)
	    memmove (&buf[0], &buf[nwrite], buflen);
	}
      else if (nwrite < 0)
	{
	  _gpgme_io_close (rp[1]);
	  return gpg_error_from_syserror ();
	}
    }

  return 0;
}


static gpgme_error_t
arg_to_data (gpgme_data_t conf, gpgme_conf_opt_t option, gpgme_conf_arg_t arg)
{
  gpgme_error_t err = 0;
  int amt = 0;
  char buf[16];

  while (amt >= 0 && arg)
    {
      switch (option->alt_type)
	{
	case GPGME_CONF_NONE:
	case GPGME_CONF_UINT32:
	default:
	  snprintf (buf, sizeof (buf), "%u", arg->value.uint32);
	  buf[sizeof (buf) - 1] = '\0';
	  amt = gpgme_data_write (conf, buf, strlen (buf));
	  break;
	  
	case GPGME_CONF_INT32:
	  snprintf (buf, sizeof (buf), "%i", arg->value.uint32);
	  buf[sizeof (buf) - 1] = '\0';
	  amt = gpgme_data_write (conf, buf, strlen (buf));
	  break;
	
          
	case GPGME_CONF_STRING:
          /* The complex types below are only here to silent the
             compiler warning. */
        case GPGME_CONF_FILENAME: 
        case GPGME_CONF_LDAP_SERVER:
        case GPGME_CONF_KEY_FPR:
        case GPGME_CONF_PUB_KEY:
        case GPGME_CONF_SEC_KEY:
        case GPGME_CONF_ALIAS_LIST:
	  /* One quote character, and three times to allow
	     for percent escaping.  */
	  {
	    char *ptr = arg->value.string;
	    amt = gpgme_data_write (conf, "\"", 1);
	    if (amt < 0)
	      break;

	    while (!err && *ptr)
	      {
		switch (*ptr)
		  {
		  case '%':
		    amt = gpgme_data_write (conf, "%25", 3);
		    break;

		  case ':':
		    amt = gpgme_data_write (conf, "%3a", 3);
		    break;

		  case ',':
		    amt = gpgme_data_write (conf, "%2c", 3);
		    break;

		  default:
		    amt = gpgme_data_write (conf, ptr, 1);
		  }
		ptr++;
	      }
	  }
	  break;
	}

      if (amt < 0)
	break;

      arg = arg->next;
      /* Comma separator.  */
      if (arg)
	amt = gpgme_data_write (conf, ",", 1);
    }

  if (amt < 0)
    return gpg_error_from_syserror ();
  
  return 0;
}


static gpgme_error_t
gpgconf_conf_save (void *engine, gpgme_conf_comp_t comp)
{
  gpgme_error_t err;
  int amt = 0;
  /* We use a data object to store the new configuration.  */
  gpgme_data_t conf;
  gpgme_conf_opt_t option;
  int something_changed = 0;

  err = gpgme_data_new (&conf);
  if (err)
    return err;

  option = comp->options;
  while (!err && amt >= 0 && option)
    {
      if (option->change_value)
	{
	  unsigned int flags = 0;
	  char buf[16];

	  something_changed = 1;

	  amt = gpgme_data_write (conf, option->name, strlen (option->name));
	  if (amt >= 0)
	    amt = gpgme_data_write (conf, ":", 1);
	  if (amt < 0)
	    break;

	  if (!option->new_value)
	    flags |= GPGME_CONF_DEFAULT;
	  snprintf (buf, sizeof (buf), "%u", flags);
	  buf[sizeof (buf) - 1] = '\0';

	  amt = gpgme_data_write (conf, buf, strlen (buf));
	  if (amt >= 0)
	    amt = gpgme_data_write (conf, ":", 1);
	  if (amt < 0)
	    break;

	  if (option->new_value)
	    {
	      err = arg_to_data (conf, option, option->new_value);
	      if (err)
		break;
	    }
	  amt = gpgme_data_write (conf, "\n", 1);
	}
      option = option->next;
    }
  if (!err && amt < 0)
    err = gpg_error_from_syserror ();
  if (err || !something_changed)
    goto bail;

  err = gpgme_data_seek (conf, 0, SEEK_SET);
  if (err)
    goto bail;

  err = gpgconf_write (engine, "--change-options", comp->name, conf);
 bail:
  gpgme_data_release (conf);
  return err;
}


static void
gpgconf_set_io_cbs (void *engine, gpgme_io_cbs_t io_cbs)
{
  /* Nothing to do.  */
}


/* Currently, we do not use the engine interface for the various
   operations.  */
void
_gpgme_conf_release (gpgme_conf_comp_t conf)
{
  gpgconf_config_release (conf);
}


struct engine_ops _gpgme_engine_ops_gpgconf =
  {
    /* Static functions.  */
    _gpgme_get_gpgconf_path,
    gpgconf_get_version,
    gpgconf_get_req_version,
    gpgconf_new,

    /* Member functions.  */
    gpgconf_release,
    NULL,		/* reset */
    NULL,		/* set_status_handler */
    NULL,		/* set_command_handler */
    NULL,		/* set_colon_line_handler */
    NULL,		/* set_locale */
    NULL,		/* decrypt */
    NULL,		/* delete */
    NULL,		/* edit */
    NULL,		/* encrypt */
    NULL,		/* encrypt_sign */
    NULL,		/* export */
    NULL,		/* export_ext */
    NULL,		/* genkey */
    NULL,		/* import */
    NULL,		/* keylist */
    NULL,		/* keylist_ext */
    NULL,		/* sign */
    NULL,		/* trustlist */
    NULL,		/* verify */
    NULL,		/* getauditlog */
    gpgconf_conf_load,
    gpgconf_conf_save,
    gpgconf_set_io_cbs,
    NULL,		/* io_event */
    NULL		/* cancel */
  };
