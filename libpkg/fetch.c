/*
 * Copyright (c) 2011-2012 Julien Laffaye <jlaffaye@FreeBSD.org>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fetch.h>

#include "pkg.h"
#include "private/event.h"
#include "private/utils.h"

int
pkg_fetch_file(const char *url, const char *dest, time_t t, int open_flags)
{
	int fd = -1;
	FILE *remote = NULL;
	struct url *u;
	struct url_stat st;
	off_t done = 0;
	off_t r;

	int64_t max_retry, retry;
	time_t begin_dl;
	time_t now;
	time_t last = 0;
	char buf[10240];
	int retcode = EPKG_OK;
	bool srv = false;
	char zone[MAXHOSTNAMELEN + 12];
	struct dns_srvinfo *mirrors, *current;
    int default_oflags = O_WRONLY|O_CREAT|O_TRUNC|O_EXCL;
    
    if (!open_flags) {
        open_flags = default_oflags;
    }

	current = mirrors = NULL;

	fetchTimeout = 30;

	if (pkg_config_int64(PKG_CONFIG_FETCH_RETRY, &max_retry) == EPKG_FATAL)
		max_retry = 3;

	retry = max_retry;

	if ((fd = open(dest, open_flags, 0600)) == -1) {
		pkg_emit_errno("open", dest);
		return(EPKG_FATAL);
	}

	u = fetchParseURL(url);
	while (remote == NULL) {
		if (retry == max_retry) {
			pkg_config_bool(PKG_CONFIG_SRV_MIRROR, &srv);
			if (srv) {
				if (strcmp(u->scheme, "file") != 0) {
					snprintf(zone, sizeof(zone),
					    "_%s._tcp.%s", u->scheme, u->host);
					mirrors = dns_getsrvinfo(zone);
					current = mirrors;
				}
			}
		}

		if (mirrors != NULL)
			strlcpy(u->host, current->host, sizeof(u->host));

		remote = fetchXGet(u, &st, "");
		if (remote == NULL) {
			--retry;
			if (retry <= 0) {
				pkg_emit_error("%s: %s", url,
				    fetchLastErrString);
				retcode = EPKG_FATAL;
				goto cleanup;
			}
			if (mirrors == NULL) {
				sleep(1);
			} else {
				current = current->next;
				if (current == NULL)
					current = mirrors;
			}
		}
	}
	if (t != 0) {
		if (st.mtime <= t) {
			retcode = EPKG_UPTODATE;
			goto cleanup;
		}
	}

	begin_dl = time(NULL);
	while (done < st.size) {
		if ((r = fread(buf, 1, sizeof(buf), remote)) < 1)
			break;

		if (write(fd, buf, r) != r) {
			pkg_emit_errno("write", dest);
			retcode = EPKG_FATAL;
			goto cleanup;
		}

		done += r;
		now = time(NULL);
		/* Only call the callback every second */
		if (now > last || done == st.size) {
			pkg_emit_fetching(url, st.size, done, (now - begin_dl));
			last = now;
		}
	}

	if (ferror(remote)) {
		pkg_emit_error("%s: %s", url, fetchLastErrString);
		retcode = EPKG_FATAL;
		goto cleanup;
	}

	cleanup:

	if (fd > 0)
		close(fd);

	if (remote != NULL)
		fclose(remote);

	fetchFreeURL(u);

	/* Remove local file if fetch failed */
	if (retcode != EPKG_OK)
		unlink(dest);

	return (retcode);
}
