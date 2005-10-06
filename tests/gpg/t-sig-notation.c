/* t-sig-notation.c - Regression test.
   Copyright (C) 2005 g10 Code GmbH

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
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/* We need to include config.h so that we know whether we are building
   with large file system (LFS) support. */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gpgme.h>

#include "t-support.h"


static struct {
  const char *name;
  const char *value;
  gpgme_sig_notation_flags_t flags;
  int seen;
} expected_notations[] = { 
  { "laughing@me",
    "Just Squeeze Me",
    GPGME_SIG_NOTATION_HUMAN_READABLE },
  { "preferred-email-encoding@pgp.com",
    "pgpmime",
    GPGME_SIG_NOTATION_HUMAN_READABLE | GPGME_SIG_NOTATION_CRITICAL },
  { NULL, 
    "http://www.gnu.org/policy/",
    0 }
};

static void
check_result (gpgme_verify_result_t result)
{
  int i;
  gpgme_sig_notation_t r;
  
  gpgme_signature_t sig;

  sig = result->signatures;
  if (!sig || sig->next)
    {
      fprintf (stderr, "%s:%i: Unexpected number of signatures\n",
	       __FILE__, __LINE__);
      exit (1);
    }

  for (i=0; i < DIM(expected_notations); i++ )
    expected_notations[i].seen = 0;
  
  for (r = result->signatures->notations; r; r = r->next)
    {
      int any = 0;
      for (i=0; i < DIM(expected_notations); i++)
	{
	  if ( ((r->name && expected_notations[i].name
		 && !strcmp (r->name, expected_notations[i].name)
		 && r->name_len
		 == strlen (expected_notations[i].name))
		|| (!r->name && !expected_notations[i].name
		    && r->name_len == 0))
	       && r->value
	       && !strcmp (r->value, expected_notations[i].value)
	       && r->value_len == strlen (expected_notations[i].value)
	       && r->flags
	       == (expected_notations[i].flags & ~GPGME_SIG_NOTATION_CRITICAL)
	       && r->human_readable
	       == !!(r->flags & GPGME_SIG_NOTATION_HUMAN_READABLE)
	       && r->critical == 0)
	    {
	      expected_notations[i].seen++;
	      any++;
	    }
	}
      if (!any)
	{
	  fprintf (stderr, "%s:%i: Unexpected notation data\n",
		   __FILE__, __LINE__);
	  exit (1);
	}
    }
  for (i=0; i < DIM(expected_notations); i++ )
    {
      if (expected_notations[i].seen != 1)
	{
	  fprintf (stderr, "%s:%i: Missing or duplicate notation data\n",
		   __FILE__, __LINE__);
	  exit (1);
	}
    }
}


int 
main (int argc, char *argv[])
{
  gpgme_ctx_t ctx;
  gpgme_error_t err;
  gpgme_data_t in, out;
  gpgme_verify_result_t result;
  char *agent_info;
  int i;

  init_gpgme (GPGME_PROTOCOL_OpenPGP);

  err = gpgme_new (&ctx);
  fail_if_err (err);

  agent_info = getenv ("GPG_AGENT_INFO");
  if (!(agent_info && strchr (agent_info, ':')))
    gpgme_set_passphrase_cb (ctx, passphrase_cb, NULL);

  err = gpgme_data_new_from_mem (&in, "Hallo Leute\n", 12, 0);
  fail_if_err (err);
  err = gpgme_data_new (&out);
  fail_if_err (err);

  for (i = 0; i < sizeof (expected_notations) / sizeof (expected_notations[0]);
       i++)
    {
      err = gpgme_sig_notation_add (ctx, expected_notations[i].name,
				    expected_notations[i].value,
				    expected_notations[i].flags);
      fail_if_err (err);
    }
  
  err = gpgme_op_sign (ctx, in, out, GPGME_SIG_MODE_NORMAL);
  fail_if_err (err);

  gpgme_data_release (in);
  err = gpgme_data_new (&in);
  fail_if_err (err);

  gpgme_data_seek (out, 0, SEEK_SET);

  err = gpgme_op_verify (ctx, out, NULL, in);
  fail_if_err (err);
  result = gpgme_op_verify_result (ctx);
  check_result (result);

  gpgme_data_release (in);
  gpgme_data_release (out);
  gpgme_release (ctx);
  return 0;
}
