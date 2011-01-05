#include <sys/param.h>

#include <archive.h>
#include <archive_entry.h>
#include <err.h>
#include <stdlib.h>
#include <glob.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>

#include "pkg.h"
#include "pkg_manifest.h"

#define METADATA_GLOB "+{DEINSTALL,INSTALL,MTREE_DIRS}"

static int pkg_create_from_dir(struct pkg_manifest *, const char *, struct archive *);
static const char * pkg_create_set_format(struct archive *, pkg_formats);

static int
pkg_create_from_dir(struct pkg_manifest *m, const char *root, struct archive *pkg_archive)
{
	struct archive_entry *entry;
#if 0
	char *path;
	char glob_pattern[MAXPATHLEN + sizeof(METADATA_GLOB)];
	glob_t g;
#endif
	int fd;
	size_t len/*, i = 0*/;
	char buf[BUFSIZ];
	char *buffer;
	size_t buffer_len;
	char fpath[MAXPATHLEN];
	const char *filepath;
	struct archive *ar;

	ar = archive_read_disk_new();
	archive_read_disk_set_standard_lookup(ar);

	entry = archive_entry_new();

/* TODO: Add metadatas */
#if 0	
	path = strdup(mpath);
	buffer = strrchr(path, '/');
	buffer[0] = '\0';
	/* Add the metadatas */
	snprintf(glob_pattern, sizeof(glob_pattern), "%s/"METADATA_GLOB, path);
	free(path);
	
	if (glob(glob_pattern, GLOB_NOSORT|GLOB_BRACE, NULL, &g) == 0) {
		for ( i = 0; i < g.gl_pathc; i++) {
			fd = open(g.gl_pathv[i], O_RDONLY);
			archive_entry_copy_sourcepath(entry, g.gl_pathv[i]);

			if (archive_read_disk_entry_from_file(ar, entry, -1, 0) != ARCHIVE_OK)
				warnx("archive_read_disk_entry_from_file(%s): %s", g.gl_pathv[i],
					  archive_error_string(ar));

			archive_entry_set_pathname(entry, basename(g.gl_pathv[i]));
			archive_write_header(pkg_archive, entry);
			while ( (len = read(fd, buf, sizeof(buf))) > 0 )
				archive_write_data(pkg_archive, buf, len);

			close(fd);
			archive_entry_clear(entry);
		}
	}
	globfree(&g);
#endif
	buffer = pkg_manifest_dump_buffer(m);
	buffer_len = strlen(buffer);

	archive_entry_set_filetype(entry, AE_IFREG);
	archive_entry_set_pathname(entry, "+MANIFEST");
	archive_entry_set_size(entry, buffer_len);
	archive_write_header(pkg_archive, entry);
	archive_write_data(pkg_archive, buffer, buffer_len);
	archive_entry_clear(entry);

	free(buffer);

	pkg_manifest_file_init(m);
	while (pkg_manifest_file_next(m) == 0) {
		filepath = pkg_manifest_file_path(m);
		snprintf(fpath, sizeof(MAXPATHLEN), "%s/%s", root, filepath);
		archive_entry_copy_sourcepath(entry, filepath);

		if (archive_read_disk_entry_from_file(ar, entry, -1, 0) != ARCHIVE_OK)
			warnx("archive_read_disk_entry_from_file(%s): %s", filepath,
				  archive_error_string(ar));

		archive_entry_set_pathname(entry, filepath);
		archive_write_header(pkg_archive, entry);
		fd = open(filepath, O_RDONLY);
		if (fd != -1) {
			while ( (len = read(fd, buf, sizeof(buf))) > 0 )
				archive_write_data(pkg_archive, buf, len);

			close(fd);
		}
		archive_entry_clear(entry);
	}

	archive_entry_free(entry);
	archive_read_finish(ar);

	return (0);
}

int
pkg_create(const char *mpath, pkg_formats format, const char *outdir, const char *rootdir, struct pkg *pkg)
{
	char namever[FILENAME_MAX];
	struct archive *pkg_archive;
	char archive_path[MAXPATHLEN];
	const char *ext;
	struct pkg_manifest *m;

	pkg_archive = archive_write_new();

	if ((ext = pkg_create_set_format(pkg_archive, format)) == NULL) {
		archive_write_finish(pkg_archive);
		warnx("Unsupport format");
		return (-1);
	}

	if (pkg == NULL && mpath != NULL) {
		m = pkg_manifest_load_file(mpath);
	} else if (pkg != NULL) {
		pkg_manifest_from_pkg(pkg, &m);
	} else {
		err(1, "mpath or pkg required");
	}

	snprintf(namever, sizeof(namever), "%s-%s", pkg_manifest_value(m, "name"),
			 pkg_manifest_value(m, "version"));
	snprintf(archive_path, sizeof(archive_path), "%s/%s.%s", outdir, namever, ext);

	archive_write_set_format_pax_restricted(pkg_archive);
	archive_write_open_filename(pkg_archive, archive_path);

	pkg_create_from_dir(m, rootdir, pkg_archive);

	pkg_manifest_free(m);
	archive_write_close(pkg_archive);
	archive_write_finish(pkg_archive);

	return (0);
}


static const char *
pkg_create_set_format(struct archive *pkg_archive, pkg_formats format)
{
	switch (format) {
		case TAR:
			archive_write_set_compression_none(pkg_archive);
			return ("tar");
		case TGZ:
			if (archive_write_set_compression_gzip(pkg_archive) != ARCHIVE_OK) {
				warnx("gzip compression is not supported, trying plain tar");
				return (pkg_create_set_format(pkg_archive, TAR));
			} else {
				return ("tgz");
			}
		case TBZ:
			if (archive_write_set_compression_bzip2(pkg_archive) != ARCHIVE_OK) {
				warnx("bzip2 compression is not supported, trying gzip");
				return (pkg_create_set_format(pkg_archive, TGZ));
			} else {
				return ("tbz");
			}
		case TXZ:
			if (archive_write_set_compression_xz(pkg_archive) != ARCHIVE_OK) {
				warnx("xz compression is not supported, trying bzip2");
				return (pkg_create_set_format(pkg_archive, TBZ));
			} else {
				return ("txz");
			}
	}
	return (NULL);
}
