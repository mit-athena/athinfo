/* Copyright 1998 by the Massachusetts Institute of Technology.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */

static const char rcsid[] = "$Id: athinfo.c,v 1.2 1999-09-15 23:56:19 danw Exp $";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>

#define ATHINFO_FALLBACK_PORT 49155

int main(int argc, char **argv)
{
  int s, count, pos, len;
  const char *host, *query;
  struct hostent *hostent;
  struct servent *servent;
  struct sockaddr_in sin;
  char buf[8192];

  if (argc != 3)
    {
      fprintf(stderr, "athinfo: usage: %s host query\n", argv[0]);
      return 1;
    }
  host = argv[1];
  query = argv[2];

  /* Get host address and port. */
  hostent = gethostbyname(host);
  if (!hostent || hostent->h_addrtype != AF_INET)
    {
      fprintf(stderr, "athinfo: host %s not found\n", host);
      return 1;
    }
  servent = getservbyname("athinfo", "tcp");

  /* Create the client socket. */
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1)
    {
      fprintf(stderr, "athinfo: socket: %s\n", strerror(errno));
      return 1;
    }

  /* Connect the socket. */
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  memcpy(&sin.sin_addr, hostent->h_addr, sizeof(sin.sin_addr));
  sin.sin_port = (servent) ? servent->s_port : htons(ATHINFO_FALLBACK_PORT);
  if (connect(s, (struct sockaddr *) &sin, sizeof(sin)) == -1)
    {
      fprintf(stderr, "athinfo: connect: %s\n", strerror(errno));
      return 1;
    }

  /* Write the query. */
  len = strlen(query);
  pos = 0;
  while (pos < len)
    {
      count = write(s, query + pos, len - pos);
      if (count == -1 && errno != EINTR)
	{
	  fprintf(stderr, "athinfo: write: %s\n", strerror(errno));
	  return 1;
	}
      else if (count > 0)
	pos += count;
    }
  do
    {
      count = write(s, "\n", 1);
      if (count == -1 && errno != EINTR)
	{
	  fprintf(stderr, "athinfo: write: %s\n", strerror(errno));
	  return 1;
	}
    }
  while (count != 1);

  /* Display the response. */
  while (1)
    {
      /* Read some data. */
      count = read(s, buf, sizeof(buf));
      if (count == -1 && errno == EINTR)
	continue;
      if (count == -1)
	{
	  fprintf(stderr, "athinfo: read: %s\n", strerror(errno));
	  return 1;
	}
      if (count == 0)
	break;

      /* Write it to stdout. */
      len = count;
      pos = 0;
      while (pos < len)
	{
	  count = write(STDOUT_FILENO, buf + pos, len - pos);
	  if (count == -1 && errno != EINTR)
	    {
	      fprintf(stderr, "athinfo: write: %s\n", strerror(errno));
	      return 1;
	    }
	  else if (count > 0)
	    pos += count;
	}
    }

  close(s);
  return 0;
}
