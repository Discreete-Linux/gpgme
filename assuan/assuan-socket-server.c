/* assuan-socket-server.c - Assuan socket based server
 *	Copyright (C) 2002 Free Software Foundation, Inc.
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
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "assuan-defs.h"

static int
accept_connection (ASSUAN_CONTEXT ctx)
{
  int fd;
  struct sockaddr_un clnt_addr;
  size_t len = sizeof clnt_addr;

  fd = accept (ctx->listen_fd, (struct sockaddr*)&clnt_addr, &len );
  if (fd == -1)
    {
      ctx->os_errno = errno;
      return ASSUAN_Accept_Failed;
    }

  ctx->inbound.fd = fd;
  ctx->inbound.eof = 0;
  ctx->inbound.linelen = 0;
  ctx->inbound.attic.linelen = 0;
  ctx->inbound.attic.pending = 0;

  ctx->outbound.fd = fd;
  ctx->outbound.data.linelen = 0;
  ctx->outbound.data.error = 0;
  
  ctx->confidential = 0;

  return 0;
}

static int
finish_connection (ASSUAN_CONTEXT ctx)
{
  if (ctx->inbound.fd != -1)
    {
      close (ctx->inbound.fd);
    }
  ctx->inbound.fd = -1;
  ctx->outbound.fd = -1;
  return 0;
}


static void
deinit_socket_server (ASSUAN_CONTEXT ctx)
{
  finish_connection (ctx);
}



/* Initialize a server for the socket LISTEN_FD which has already be
   put into listen mode */
int
assuan_init_socket_server (ASSUAN_CONTEXT *r_ctx, int listen_fd)
{
  ASSUAN_CONTEXT ctx;
  int rc;

  *r_ctx = NULL;
  ctx = xtrycalloc (1, sizeof *ctx);
  if (!ctx)
    return ASSUAN_Out_Of_Core;
  ctx->is_server = 1;
  ctx->input_fd = -1;
  ctx->output_fd = -1;

  ctx->inbound.fd = -1;
  ctx->outbound.fd = -1;

  ctx->listen_fd = listen_fd;
  ctx->deinit_handler = deinit_socket_server;
  ctx->accept_handler = accept_connection;
  ctx->finish_handler = finish_connection;

  rc = _assuan_register_std_commands (ctx);
  if (rc)
    xfree (ctx);
  else
    *r_ctx = ctx;
  return rc;
}










