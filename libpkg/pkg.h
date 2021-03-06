/*
 * Copyright (c) 2011-2012 Baptiste Daroussin <bapt@FreeBSD.org>
 * Copyright (c) 2011-2012 Julien Laffaye <jlaffaye@FreeBSD.org>
 * Copyright (c) 2011 Will Andrews <will@FreeBSD.org>
 * Copyright (c) 2011 Philippe Pepiot <phil@philpep.org>
 * Copyright (c) 2011-2012 Marin Atanasov Nikolov <dnaeon@gmail.com>
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

#ifndef _PKG_H
#define _PKG_H

#include <sys/types.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/sbuf.h>
#include <openssl/pem.h>

#define PKGVERSION "1.0.90"
/* PORTVERSION equivalent for proper pkg-static->ports-mgmt/pkg version comparison
 * in pkgdb_query_newpkgversion() */
#define PKG_PORTVERSION "1.0.90"

struct pkg;
struct pkg_dep;
struct pkg_file;
struct pkg_dir;
struct pkg_category;
struct pkg_option;
struct pkg_license;
struct pkg_user;
struct pkg_group;
struct pkg_shlib;

struct pkgdb;
struct pkgdb_it;

struct pkg_jobs;

struct pkg_repos;
struct pkg_repos_entry;

struct pkg_config_kv;

typedef enum {
	/**
	 * The license logic is OR (dual in the ports)
	 */
	LICENSE_OR='|',
	/**
	 * The license logic is AND (multi in the ports)
	 */
	LICENSE_AND='&',
	/**
	 * The license logic un single (default in the ports)
	 */
	LICENSE_SINGLE=1
} lic_t;

typedef enum {
	PKGDB_DEFAULT=0,
	PKGDB_REMOTE
} pkgdb_t;

/**
 * Specify how an argument should be used by query functions.
 */
typedef enum {
	/**
	 * The argument does not matter, all items will be matched.
	 */
	MATCH_ALL,
	/**
	 * The argument is the exact pattern.
	 */
	MATCH_EXACT,
	/**
	 * The argument is a globbing expression.
	 */
	MATCH_GLOB,
	/**
	 * The argument is a basic regular expression.
	 */
	MATCH_REGEX,
	/**
	 * The argument is an extended regular expression.
	 */
	MATCH_EREGEX,
	/**
	 * The argument is a WHERE clause to use as condition
	 */
	MATCH_CONDITION
} match_t;

/**
 * Specify on which field the pattern will be matched uppon.
 */

typedef enum {
	FIELD_NONE,
	FIELD_ORIGIN,
	FIELD_NAME,
	FIELD_NAMEVER,
	FIELD_COMMENT,
	FIELD_DESC
} pkgdb_field;

/**
 * The type of package.
 */
typedef enum {
	/**
	 * The pkg type can not be determined.
	 */
	PKG_NONE = 0,

	/**
	 * The pkg refers to a local file archive.
	 */
	PKG_FILE = 1 << 0,
	/**
	 * The pkg refers to a package available on the remote repository.
	 * @todo Document which attributes are available.
	 */
	PKG_REMOTE = 1 << 1,
	/**
	 * The pkg refers to a localy installed package.
	 */
	PKG_INSTALLED = 1 << 2,
} pkg_t;

/**
 * Contains keys to refer to a string attribute.
 * Used by pkg_get() and pkg_set()
 */
typedef enum {
	PKG_ORIGIN = 1,
	PKG_NAME,
	PKG_VERSION,
	PKG_COMMENT,
	PKG_DESC,
	PKG_MTREE,
	PKG_MESSAGE,
	PKG_ARCH,
	PKG_MAINTAINER,
	PKG_WWW,
	PKG_PREFIX,
	PKG_INFOS,
	PKG_REPOPATH,
	PKG_CKSUM,
	PKG_NEWVERSION,
	PKG_REPONAME,
	PKG_REPOURL, /* end of fields */
	PKG_FLATSIZE=64,
	PKG_NEW_FLATSIZE,
	PKG_NEW_PKGSIZE,
	PKG_LICENSE_LOGIC,
	PKG_AUTOMATIC,
	PKG_ROWID,
	PKG_TIME,
} pkg_attr;

typedef enum {
	PKG_SET_FLATSIZE = 1,
	PKG_SET_AUTOMATIC,
	PKG_SET_DEPORIGIN,
	PKG_SET_ORIGIN
} pkg_set_attr;

/**
 * contains keys to refer to a string attribute
 * Used by pkg_dep_get()
 */
typedef enum {
	PKG_DEP_NAME = 0,
	PKG_DEP_ORIGIN,
	PKG_DEP_VERSION
} pkg_dep_attr;

typedef enum {
	PKG_FILE_PATH = 0,
	PKG_FILE_SUM,
	PKG_FILE_UNAME,
	PKG_FILE_GNAME
} pkg_file_attr;

typedef enum {
	PKG_DEPS = 0,
	PKG_RDEPS,
	PKG_LICENSES,
	PKG_OPTIONS,
	PKG_CATEGORIES,
	PKG_FILES,
	PKG_DIRS,
	PKG_USERS,
	PKG_GROUPS,
	PKG_SHLIBS
} pkg_list;

/**
 * Determine the type of a pkg_script.
 */
typedef enum {
	PKG_SCRIPT_PRE_INSTALL = 0,
	PKG_SCRIPT_POST_INSTALL,
	PKG_SCRIPT_PRE_DEINSTALL,
	PKG_SCRIPT_POST_DEINSTALL,
	PKG_SCRIPT_PRE_UPGRADE,
	PKG_SCRIPT_POST_UPGRADE,
	PKG_SCRIPT_INSTALL,
	PKG_SCRIPT_DEINSTALL,
	PKG_SCRIPT_UPGRADE
} pkg_script;

typedef enum _pkg_jobs_t {
	PKG_JOBS_INSTALL,
	PKG_JOBS_DEINSTALL,
	PKG_JOBS_FETCH
} pkg_jobs_t;

typedef enum _pkg_config_key {
	PKG_CONFIG_REPO = 0,
	PKG_CONFIG_DBDIR = 1,
	PKG_CONFIG_CACHEDIR = 2,
	PKG_CONFIG_PORTSDIR = 3,
	PKG_CONFIG_REPOKEY = 4,
	PKG_CONFIG_MULTIREPOS = 5,
	PKG_CONFIG_HANDLE_RC_SCRIPTS = 6,
	PKG_CONFIG_ASSUME_ALWAYS_YES = 7,
	PKG_CONFIG_REPOS = 8,
	PKG_CONFIG_PLIST_KEYWORDS_DIR = 9,
	PKG_CONFIG_SYSLOG = 10,
	PKG_CONFIG_SHLIBS = 11,
	PKG_CONFIG_AUTODEPS = 12,
	PKG_CONFIG_ABI = 13,
	PKG_CONFIG_DEVELOPER_MODE = 14,
	PKG_CONFIG_PORTAUDIT_SITE = 15,
	PKG_CONFIG_SRV_MIRROR = 16,
	PKG_CONFIG_FETCH_RETRY = 17,
} pkg_config_key;

typedef enum {
	PKG_CONFIG_KV_KEY,
	PKG_CONFIG_KV_VALUE
} pkg_config_kv_t;

typedef enum _pkg_stats_t {
	PKG_STATS_LOCAL_COUNT = 0,
	PKG_STATS_LOCAL_SIZE,
	PKG_STATS_REMOTE_COUNT,
	PKG_STATS_REMOTE_UNIQUE,
	PKG_STATS_REMOTE_SIZE,
	PKG_STATS_REMOTE_REPOS,
} pkg_stats_t;

/**
 * Error type used everywhere by libpkg.
 */
typedef enum {
	EPKG_OK = 0,
	/**
	 * No more items available (end of the loop).
	 */
	EPKG_END,
	EPKG_WARN,
	/**
	 * The function encountered a fatal error.
	 */
	EPKG_FATAL,
	/**
	 * Can not delete the package because it is required by another package.
	 */
	EPKG_REQUIRED,
	/**
	 * Can not install the package because it is already installed.
	 */
	EPKG_INSTALLED,
	/**
	 * Can not install the package because some dependencies are unresolved.
	 */
	EPKG_DEPENDENCY,
	/**
	 * Can not create local database
	 */
	EPKG_ENODB,
	/**
	 * local file newer than remote
	 */
	EPKG_UPTODATE,
	/**
	 * unkown keyword
	 */
	EPKG_UNKNOWN,
	/**
	 * repo DB schema incompatible version
	 */
	EPKG_REPOSCHEMA,
} pkg_error_t;

/**
 * Allocate a new pkg.
 * Allocated pkg must be deallocated by pkg_free().
 */
int pkg_new(struct pkg **, pkg_t type);

/**
 * Reset a pkg to its initial state.
 * Useful to avoid sequences of pkg_new() and pkg_free().
 */
void pkg_reset(struct pkg *, pkg_t type);

/**
 * Deallocate a pkg
 */
void pkg_free(struct pkg *);

/**
 * Check if a package is valid according to its type.
 */
int pkg_is_valid(struct pkg *);

/**
 * Open a package file archive and retrive informations.
 * @param p A pointer to pkg allocated by pkg_new(), or if it points to a
 * NULL pointer, the function allocate a new pkg using pkg_new().
 * @param path The path to the local package archive.
 */
int pkg_open(struct pkg **p, const char *path);

/**
 * @return the type of the package.
 * @warning returns PKG_NONE on error.
 */
pkg_t pkg_type(struct pkg const * const);

/**
 * Generic getter for simple attributes.
 * @return NULL-terminated string.
 * @warning May return a NULL pointer.
 */
int pkg_get2(struct pkg const *const, ...);
#define pkg_get(pkg, ...) pkg_get2(pkg, __VA_ARGS__, -1)

/**
 * Specific getters for simple attributes.
 * @return NULL-terminated string.
 */
const char *pkg_name(struct pkg const *const pkg);
const char *pkg_version(struct pkg const *const pkg);

int pkg_list_is_empty(struct pkg *, pkg_list);
/**
 * Iterates over the dependencies of the package.
 * @param dep Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_deps(struct pkg *, struct pkg_dep **dep);

/**
 * Iterates over the reverse dependencies of the package.
 * That is, the packages which require this package.
 * @param dep Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_rdeps(struct pkg *, struct pkg_dep **dep);

/**
 * Iterates over the files of the package.
 * @param file Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_files(struct pkg *, struct pkg_file **file);

/**
 * Iterates over the directories of the package.
 * @param Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_dirs(struct pkg *pkg, struct pkg_dir **dir);

/**
 * Iterates over the categories of the package.
 * @param Must be set to NULL for the first call.
 * @return An error code
 */
int pkg_categories(struct pkg *pkg, struct pkg_category **category);

/**
 * Iterates over the licenses of the package.
 * @param Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_licenses(struct pkg *pkg, struct pkg_license **license);

/**
 * Iterates over the users of the package.
 * @param Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_users(struct pkg *pkg, struct pkg_user **user);

/**
 * Iterates over the groups of the package.
 * @param Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_groups(struct pkg *pkg, struct pkg_group **group);

/**
 * Iterates over the options of the package.
 * @param  option Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_options(struct pkg *, struct pkg_option **option);

/**
 * Iterates over the shared libraries used by the package.
 * @param shlib must be set to NULL for the first call.
 * @return An error code
 */
int pkg_shlibs(struct pkg *pkg, struct pkg_shlib **shlib);

/**
 * Iterate over all of the files within the package pkg, ensuring the
 * dependency list contains all applicable packages providing the
 * shared objects used by pkg.
 * Also add all the shared object into the shlibs.
 * It respects the SHLIBS and AUTODEPS options from configuration
 * @return An error code
 */

 /* Don't conflict with PKG_LOAD_* q.v. */
#define PKG_CONTAINS_ELF_OBJECTS (1<<24)
#define PKG_CONTAINS_STATIC_LIBS (1<<25)
#define PKG_CONTAINS_H_OR_LA (1<<26)

int pkg_analyse_files(struct pkgdb *, struct pkg *);

/**
 * Suggest if a package could be marked architecture independent or
 * not.
 */
int pkg_suggest_arch(struct pkg *, const char *, bool);

/**
 * Generic setter for simple attributes.
 */
int pkg_set2(struct pkg *pkg, ...);
#define pkg_set(pkg, ...) pkg_set2(pkg, __VA_ARGS__, -1)

int pkgdb_set2(struct pkgdb *db, struct pkg *pkg, ...);
#define pkgdb_set(db, pkg, ...) pkgdb_set2(db, pkg, __VA_ARGS__, -1)

/**
 * update the checksum of a file in the database (and the object)
 * XXX I don't think this function should be part of a public API.
 * @param db A pointer to a struct pkgdb object
 * @param file A pointer to a struct pkg_file object
 * @param sha256 The new checksum
 * @return An error code
 */
int pkgdb_file_set_cksum(struct pkgdb *db, struct pkg_file *file, const char *sha256);

/**
 * Read the content of a file into a buffer, then call pkg_set().
 */
int pkg_set_from_file(struct pkg *pkg, pkg_attr attr, const char *file);

/**
 * Allocate a new struct pkg and add it to the deps of pkg.
 * @return An error code.
 */
int pkg_adddep(struct pkg *pkg, const char *name, const char *origin, const
			   char *version);
int pkg_addrdep(struct pkg *pkg, const char *name, const char *origin, const
			   char *version);

//int pkg_addfile(struct pkg *, const char *fmt);
/**
 * Allocate a new struct pkg_file and add it to the files of pkg.
 * @param sha256 The ascii representation of the sha256 or a NULL pointer.
 * @param check_duplicates ensure no duplicate files are added to the pkg?
 * @return An error code.
 */
int pkg_addfile(struct pkg *pkg, const char *path, const char *sha256, bool check_duplicates);

/**
 * Allocate a new struct pkg_file and add it to the files of pkg;
 * @param path path of the file
 * @param sha256 The ascii representation of the sha256 or a NULL pointer.
 * @param uname the user name of the file
 * @param gname the group name of the file
 * @param perm the permission
 * @param check_duplicates ensure no duplicate files are added to the pkg?
 * @return An error code.
 */
int pkg_addfile_attr(struct pkg *pkg, const char *path, const char *sha256, const char *uname, const char *gname, mode_t perm, bool check_duplicates);

/**
 * Add a path
 * @return An error code.
 */
int pkg_adddir(struct pkg *pkg, const char *path, bool try);

/**
 * Allocate a new struct pkg_file and add it to the files of pkg;
 * @param path path of the file
 * @param uname the user name of the file
 * @param gname the group name of the file
 * @param perm the permission
 * @return An error code.
 */
int pkg_adddir_attr(struct pkg *pkg, const char *path, const char *uname, const char *gname, mode_t perm, bool try);

/**
 * Add a category
 * @return An error code.
 */
int pkg_addcategory(struct pkg *pkg, const char *name);

/**
 * Add a license
 * @return An error code.
 */
int pkg_addlicense(struct pkg *pkg, const char *name);

/**
 * Add a user
 * @return An error code.
 */
int pkg_adduser(struct pkg *pkg, const char *name);

/**
 * Add a group
 * @return An error code.
 */
int pkg_addgroup(struct pkg *pkg, const char *group);

/**
 * Add a user
 * @return An error code.
 */
int pkg_adduid(struct pkg *pkg, const char *name, const char *uidstr);

/**
 * Add a gid
 * @return an error code
 */
int pkg_addgid(struct pkg *pkg, const char *group, const char *gidstr);

/**
 * Allocate a new struct pkg_script and add it to the scripts of pkg.
 * @param path The path to the script on disk.
 @ @return An error code.
 */
int pkg_addscript(struct pkg *pkg, const char *data, pkg_script type);

/**
 * Helper which call pkg_addscript() with the content of the file and
 * with the correct type.
 */
int pkg_addscript_file(struct pkg *pkg, const char *path);
int pkg_appendscript(struct pkg *pkg, const char *cmd, pkg_script type);

/**
 * Allocate a new struct pkg_option and add it to the options of pkg.
 * @return An error code.
 */
int pkg_addoption(struct pkg *pkg, const char *name, const char *value);

/**
 * Add a shared library
 * @return An error code.
 */
int pkg_addshlib(struct pkg *pkg, const char *name);

/**
 * Parse a manifest and set the attributes of pkg accordingly.
 * @param buf An NULL-terminated buffer containing the manifest data.
 * @return An error code.
 */
int pkg_parse_manifest(struct pkg *pkg, char *buf);
int pkg_load_manifest_file(struct pkg *pkg, const char *fpath);

/**
 * Emit a manifest according to the attributes of pkg.
 * @param buf A pointer which will hold the allocated buffer containing the
 * manifest. To be free'ed.
 * @return An error code.
 */
int pkg_emit_manifest(struct pkg *pkg, char **buf);

/* pkg_dep */
const char *pkg_dep_get(struct pkg_dep const * const , const pkg_dep_attr);

const char *pkg_dep_name(struct pkg_dep const * const);
const char *pkg_dep_origin(struct pkg_dep const * const);
const char *pkg_dep_version(struct pkg_dep const * const);

/* pkg_file */
const char *pkg_file_get(struct pkg_file const * const, const pkg_file_attr);

const char *pkg_file_path(struct pkg_file const * const);
const char *pkg_file_cksum(struct pkg_file const * const);
const char *pkg_file_uname(struct pkg_file const * const);
const char *pkg_file_gname(struct pkg_file const * const);
mode_t pkg_file_mode(struct pkg_file const * const);

/* pkg_dir */
const char *pkg_dir_path(struct pkg_dir const * const);
const char *pkg_dir_uname(struct pkg_dir const * const);
const char *pkg_dir_gname(struct pkg_dir const * const);
mode_t pkg_dir_mode(struct pkg_dir const * const);
bool pkg_dir_try(struct pkg_dir const * const);

/* pkg_category */
const char *pkg_category_name(struct pkg_category const * const);

/* pkg_license */
const char *pkg_license_name(struct pkg_license const * const);

/* pkg_user */
const char *pkg_user_name(struct pkg_user const * const);
const char *pkg_user_uidstr(struct pkg_user const * const);

/* pkg_group */
const char *pkg_group_name(struct pkg_group const * const);
const char *pkg_group_gidstr(struct pkg_group const * const);

/* pkg_script */
const char *pkg_script_get(struct pkg const * const, pkg_script);

/* pkg_option */
const char *pkg_option_opt(struct pkg_option const * const);
const char *pkg_option_value(struct pkg_option const * const);

/* pkg_shlib */
const char *pkg_shlib_name(struct pkg_shlib const * const);

/**
 * @param db A pointer to a struct pkgdb object
 * @param origin Package origin
 * @return EPKG_OK if the package is installed,
 * and != EPKG_OK if the package is not installed or an error occurred
 */
int pkg_is_installed(struct pkgdb *db, const char *origin);

/**
 * Create a repository database.
 * @param path The path where the repository live.
 * @param force If true, rebuild the repository catalogue from scratch
 * @param callback A function which is called at every step of the process.
 * @param data A pointer which is passed to the callback.
 * @param sum An 65 long char array to receive the sha256 sum
 */
int pkg_create_repo(char *path, bool force, void (*callback)(struct pkg *, void *), void *);
int pkg_finish_repo(char *path, pem_password_cb *cb, char *rsa_key_path);

/**
 * Open the local package database.
 * The db must be free'ed with pkgdb_close().
 * @return An error code.
 */
int pkgdb_open(struct pkgdb **db, pkgdb_t type);

/**
 * Close and free the struct pkgdb.
 */
void pkgdb_close(struct pkgdb *db);

/**
 * Initialize the local cache of the remote database with indicies
 */
int pkgdb_remote_init(struct pkgdb *db, const char *reponame);

/**
 * Dump to or load from a backup copy of the main database file
 * (local.sqlite)
 */

int pkgdb_dump(struct pkgdb *db, const char *dest);
int pkgdb_load(struct pkgdb *db, const char *src);

/**
 * Register a ports to the database.
 * @return An error code.
 */

int pkgdb_register_ports(struct pkgdb *db, struct pkg *pkg);

/**
 * Unregister a package from the database
 * @return An error code.
 */
int pkgdb_unregister_pkg(struct pkgdb *pkg, const char *origin);

/**
 * Query the local package database.
 * @param type Describe how pattern should be used.
 * @warning Returns NULL on failure.
 */
struct pkgdb_it * pkgdb_query(struct pkgdb *db, const char *pattern,
    match_t type);
struct pkgdb_it * pkgdb_rquery(struct pkgdb *db, const char *pattern,
    match_t type, const char *reponame);
struct pkgdb_it * pkgdb_search(struct pkgdb *db, const char *pattern,
    match_t type, pkgdb_field field, pkgdb_field sort, const char *reponame);

/**
 *
 */
struct pkgdb_it *pkgdb_query_installs(struct pkgdb *db, match_t type, int nbpkgs, char **pkgs, const char *reponame, bool force, bool recursive);
struct pkgdb_it *pkgdb_query_upgrades(struct pkgdb *db, const char *reponame, bool all);
struct pkgdb_it *pkgdb_query_downgrades(struct pkgdb *db, const char *reponame);
struct pkgdb_it *pkgdb_query_delete(struct pkgdb *db, match_t type, int nbpkgs, char **pkgs, int recursive);
struct pkgdb_it *pkgdb_query_autoremove(struct pkgdb *db);
struct pkgdb_it *pkgdb_query_fetch(struct pkgdb *db, match_t type, int nbpkgs, char **pkgs, const char *reponame, int flags);

/**
 * @todo Return directly the struct pkg?
 */
struct pkgdb_it * pkgdb_query_which(struct pkgdb *db, const char *path);

struct pkgdb_it * pkgdb_query_shlib(struct pkgdb *db, const char *shlib);

#define PKG_LOAD_BASIC 0
#define PKG_LOAD_DEPS (1<<0)
#define PKG_LOAD_RDEPS (1<<1)
#define PKG_LOAD_FILES (1<<2)
#define PKG_LOAD_SCRIPTS (1<<3)
#define PKG_LOAD_OPTIONS (1<<4)
#define PKG_LOAD_MTREE (1<<5)
#define PKG_LOAD_DIRS (1<<6)
#define PKG_LOAD_CATEGORIES (1<<7)
#define PKG_LOAD_LICENSES (1<<8)
#define PKG_LOAD_USERS (1<<9)
#define PKG_LOAD_GROUPS (1<<10)
#define PKG_LOAD_SHLIBS (1<<11)
/* Make sure new PKG_LOAD don't conflict with PKG_CONTAINS_* */

/**
 * Get the next pkg.
 * @param pkg An allocated struct pkg or a pointer to a NULL pointer. In the
 * last case, the function take care of the allocation.
 * @param flags OR'ed PKG_LOAD_*
 * @return An error code.
 */
int pkgdb_it_next(struct pkgdb_it *, struct pkg **pkg, int flags);

/**
 * Free a struct pkgdb_it.
 */
void pkgdb_it_free(struct pkgdb_it *);

/**
 * Compact the database to save space.
 * Note that the function will really compact the database only if some
 * internal criterias are met.
 * @return An error code.
 */
int pkgdb_compact(struct pkgdb *db);

/**
 * Install and register a new package.
 * @param db An opened pkgdb
 * @param path The path to the package archive file on the local disk
 * @return An error code.
 */
int pkg_add(struct pkgdb *db, const char *path, int flags);

#define PKG_ADD_UPGRADE (1 << 0)
#define PKG_ADD_USE_UPGRADE_SCRIPTS (1 << 1)
#define PKG_ADD_AUTOMATIC (1 << 2)
#define PKG_ADD_FORCE (1 << 3)

/**
 * Allocate a new pkg_jobs.
 * @param db A pkgdb open with PKGDB_REMOTE.
 * @return An error code.
 */
int pkg_jobs_new(struct pkg_jobs **jobs, pkg_jobs_t type, struct pkgdb *db);

/**
 * Free a pkg_jobs
 */
void pkg_jobs_free(struct pkg_jobs *jobs);

/**
 * Add a pkg to the jobs queue.
 * @return An error code.
 */
int pkg_jobs_add(struct pkg_jobs *jobs, struct pkg *pkg);

/**
 * Returns true if there are no jobs.
 */
int pkg_jobs_is_empty(struct pkg_jobs *jobs);

/**
 * Iterates over the packages in the jobs queue.
 * @param pkg Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_jobs(struct pkg_jobs *jobs, struct pkg **pkg);

/**
 * Apply the jobs in the queue (fetch and install).
 * @return An error code.
 */
int pkg_jobs_apply(struct pkg_jobs *jobs, int force);

/**
 * Archive formats options.
 */
typedef enum pkg_formats { TAR, TGZ, TBZ, TXZ } pkg_formats;

/**
 * Create package from an installed & registered package
 */
int pkg_create_installed(const char *, pkg_formats, const char *, struct pkg *);

/**
 * Create package from stage install with a metadata directory
 */
int pkg_create_staged(const char *, pkg_formats, const char *, const char *, char *);

/**
 * Download the latest repo db file and checks its signature if any
 * @param force Always download the repo catalogue
 */
int pkg_update(const char *name, const char *packagesite, bool force);

/**
 * Get statistics information from the package database(s)
 * @param db A valid database object as returned by pkgdb_open()
 * @param type Type of statistics to be returned
 * @return The statistic information requested
 */
int64_t pkgdb_stats(struct pkgdb *db, pkg_stats_t type);

/**
 * Get the value of a configuration key
 */
int pkg_config_string(pkg_config_key key, const char **value);
int pkg_config_bool(pkg_config_key key, bool *value);
int pkg_config_list(pkg_config_key key, struct pkg_config_kv **kv);
const char *pkg_config_kv_get(struct pkg_config_kv *kv, pkg_config_kv_t type);
int pkg_config_int64(pkg_config_key key, int64_t *value);

/**
 * @todo Document
 */
int pkg_version_cmp(const char * const , const char * const);

/**
 * Fetch a file.
 * @return An error code.
 */
int pkg_fetch_file(const char *url, const char *dest, time_t t);

/* glue to deal with ports */
int ports_parse_plist(struct pkg *, char *, const char *);

/**
 * @todo Document
 */
int pkg_copy_tree(struct pkg *, const char *src, const char *dest);

/**
 * Event type used to report progress or problems.
 */
typedef enum {
	/* informational */
	PKG_EVENT_INSTALL_BEGIN = 0,
	PKG_EVENT_INSTALL_FINISHED,
	PKG_EVENT_DEINSTALL_BEGIN,
	PKG_EVENT_DEINSTALL_FINISHED,
	PKG_EVENT_UPGRADE_BEGIN,
	PKG_EVENT_UPGRADE_FINISHED,
	PKG_EVENT_FETCHING,
	PKG_EVENT_INTEGRITYCHECK_BEGIN,
	PKG_EVENT_INTEGRITYCHECK_FINISHED,
	PKG_EVENT_NEWPKGVERSION,
	/* errors */
	PKG_EVENT_ERROR,
	PKG_EVENT_ERRNO,
	PKG_EVENT_ARCHIVE_COMP_UNSUP = 65536,
	PKG_EVENT_ALREADY_INSTALLED,
	PKG_EVENT_FAILED_CKSUM,
	PKG_EVENT_CREATE_DB_ERROR,
	PKG_EVENT_REQUIRED,
	PKG_EVENT_MISSING_DEP,
	PKG_EVENT_NOREMOTEDB,
	PKG_EVENT_NOLOCALDB,
	PKG_EVENT_FILE_MISMATCH,
} pkg_event_t;

struct pkg_event {
	pkg_event_t type;
	union {
		struct {
			const char *func;
			const char *arg;
		} e_errno;
		struct {
			char *msg;
		} e_pkg_error;
		struct {
			const char *url;
			off_t total;
			off_t done;
			time_t elapsed;
		} e_fetching;
		struct {
			struct pkg *pkg;
		} e_already_installed;
		struct {
			struct pkg *pkg;
		} e_install_begin;
		struct {
			struct pkg *pkg;
		} e_install_finished;
		struct {
			struct pkg *pkg;
		} e_deinstall_begin;
		struct {
			struct pkg *pkg;
		} e_deinstall_finished;
		struct {
			struct pkg *pkg;
		} e_upgrade_begin;
		struct {
			struct pkg *pkg;
		} e_upgrade_finished;
		struct {
			struct pkg *pkg;
			struct pkg_dep *dep;
		} e_missing_dep;
		struct {
			struct pkg *pkg;
			int force;
		} e_required;
		struct {
			const char *repo;
		} e_remotedb;
		struct {
			struct pkg *pkg;
			struct pkg_file *file;
			const char *newsum;
		} e_file_mismatch;
	};
};

/**
 * Event callback mechanism.  Events will be reported using this callback,
 * providing an event identifier and up to two event-specific pointers.
 */
typedef int(*pkg_event_cb)(void *, struct pkg_event *);

void pkg_event_register(pkg_event_cb cb, void *data);

int pkg_init(const char *);
int pkg_shutdown(void);

void pkg_test_filesum(struct pkg *);
void pkg_recompute(struct pkgdb *, struct pkg *);
int pkgdb_reanalyse_shlibs(struct pkgdb *, struct pkg *);

int pkg_get_myarch(char *pkgarch, size_t sz);

void pkgdb_cmd(int argc, char **argv);
#endif
