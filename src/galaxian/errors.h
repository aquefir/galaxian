/****************************************************************************\
 *                                 Galaxian                                 *
 *                                                                          *
 *                         Copyright © 2021 Aquefir                         *
 *                 Released under Artisan Software Licence.                 *
\****************************************************************************/

#ifndef INC__GALAXIAN_ERRORS_H
#define INC__GALAXIAN_ERRORS_H

enum
{
	ERRNO_EACCES = 0,
	ERRNO_ELOOP,
	ERRNO_ENAMETOOLONG,
	ERRNO_ENOENT,
	ERRNO_ENOTDIR,
	ERRNO_EROFS,
	ERRNO_EFAULT,
	ERRNO_EINVAL,
	ERRNO_EIO,
	ERRNO_ENOMEM,
	ERRNO_ETXTBSY,
	MAX_ERRNO
};

int get_errno( int );
const char * get_errmsg( int );

#endif /* INC__GALAXIAN_ERRORS_H */
