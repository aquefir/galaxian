/****************************************************************************\
 *                                 Galaxian                                 *
 *                                                                          *
 *                         Copyright © 2021 Aquefir                         *
 *                 Released under Artisan Software Licence.                 *
\****************************************************************************/

#include "errors.h"

#include <errno.h>
#include <uni/err.h>

static const int errs[MAX_ERRNO] = { EACCES,
	ELOOP,
	ENAMETOOLONG,
	ENOENT,
	ENOTDIR,
	EROFS,
	EFAULT,
	EINVAL,
	EIO,
	ENOMEM,
	ETXTBSY };

static const char * const err_msgs[MAX_ERRNO] = { "Access denied",
	"Too many symbolic links",
	"Name too long",
	"Does not exist or dangling symlink",
	"Directory component is not a directory",
	"Cannot write to read-only file system",
	"Address space access denied",
	"Mode incorrectly specified",
	"I/O error",
	"Insufficient kernel memory",
	"Cannot write to executable file being executed" };

int get_errno( int e )
{
	if( e >= MAX_ERRNO )
	{
		uni_die( );
	}

	return errs[e];
}

const char * get_errmsg( int e )
{
	if( e >= MAX_ERRNO )
	{
		uni_die( );
	}

	return err_msgs[e];
}
