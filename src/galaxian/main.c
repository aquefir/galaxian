/****************************************************************************\
 *                                 Galaxian                                 *
 *                                                                          *
 *                         Copyright © 2021 Aquefir                         *
 *                 Released under Artisan Software Licence.                 *
\****************************************************************************/

#include <uni/err.h>
#include <uni/log.h>
#include <uni/str.h>
#include <uni/types/int.h>

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "errors.h"
#include "ini2cfg.h"

typedef u32 platforms_t;

enum
{
	PF_FLAG_LINUX = 0,
	PF_FLAG_DARWIN,
	PF_FLAG_FREEBSD,
	PF_FLAG_OPENBSD,
	PF_FLAG_NETBSD
};

enum
{
	PF_MASK_LINUX   = 1 << PF_FLAG_LINUX,
	PF_MASK_DARWIN  = 1 << PF_FLAG_DARWIN,
	PF_MASK_FREEBSD = 1 << PF_FLAG_FREEBSD,
	PF_MASK_OPENBSD = 1 << PF_FLAG_OPENBSD,
	PF_MASK_NETBSD  = 1 << PF_FLAG_NETBSD,
	PF_MASK_ALLBSDS = PF_MASK_FREEBSD | PF_MASK_OPENBSD | PF_MASK_NETBSD
};

struct srcfile
{
	u8 sha2_256sum[32];
	char * src_uri;
};

struct pkg
{
	struct srcfile * srcs;
	ptri srcs_sz;
};

static int check_rpath( const char * rpath )
{
	if( access( rpath, F_OK ) == -1 )
	{
		int e = errno;
		int i, set = 0;

		for( i = 0; i < MAX_ERRNO; ++i )
		{
			if( get_errno( i ) == e )
			{
				set = 1;
			}
		}

		if( set )
		{
			return 1;
		}
	}

	return 0;
}

static void ensure_xfile_exists(
	const char * path, platforms_t pf, const char * fail_msg )
{
	if( !path )
	{
		uni_die( );
	}

	if( !pf )
	{
		return;
	}

	{
		platforms_t mypf =
#if defined( CFG_LINUX )
			PF_MASK_LINUX
#elif defined( CFG_DARWIN )
			PF_MASK_DARWIN
#elif defined( CFG_APE )
			0 /* XXX: Replace with Cosmopolitan runtime check */
#else
			PF_MASK_ALLBSDS
#endif
			;

		if( !( pf & mypf ) )
		{
			return;
		}
	}

	if( !uni_strstr( "/", path ) )
	{
		/* scan PATH */
		const char * pathvar = getenv( "PATH" );
		const char ** paths =
			(const char **)uni_strsplit( pathvar, ":", -1 );
		int i, found = 0;

		for( i = 0; paths[i] != NULL; ++i )
		{
			const char * tpath =
				uni_strjoin( "/", paths[i], path, NULL );

			if( !check_rpath( tpath ) )
			{
				found = 1;

				break;
			}
		}

		if( found )
		{
			return;
		}
	}
	else if( check_rpath( path ) )
	{
		return;
	}

	uni_die( );
}

static void ensure_dir_exists(
	const char * path, platforms_t pf, const char * fail_msg )
{
	if( !path )
	{
		uni_die( );
	}

	if( !pf )
	{
		return;
	}

	{
		platforms_t mypf =
#if defined( CFG_LINUX )
			PF_MASK_LINUX
#elif defined( CFG_DARWIN )
			PF_MASK_DARWIN
#elif defined( CFG_APE )
			0 /* XXX: Replace with Cosmopolitan runtime check */
#else
			PF_MASK_ALLBSDS
#endif
			;

		if( !( pf & mypf ) )
		{
			return;
		}
	}

	{
		DIR * d = opendir( path );
		int e   = errno;

		if( d )
		{
			return;
		}
		else if( e == ENOENT )
		{
			if( fail_msg )
			{
				uni_perror( "Directory does not exist: %s\n",
					fail_msg );
				uni_die( );
			}
		}
		else
		{
			uni_perror( "opendir() failed. Errno was %i\n", e );
			uni_die( );
		}
	}
}

static const char * const darwin_elf_as_404 =
	"ELF-bearing GNU as was not found on your system.\nRun 'brew install "
	"x86_64-elf-binutils' to fix this. Exiting...";

static void ensure_elf_as( void )
{
	ensure_dir_exists( "/usr/local/opt/x86_64-elf-binutils/bin",
		PF_MASK_DARWIN,
		darwin_elf_as_404 );
	ensure_xfile_exists(
		"/usr/local/opt/x86_64-elf-binutils/bin/x86_64-elf-as",
		PF_MASK_DARWIN,
		darwin_elf_as_404 );
	ensure_xfile_exists( "as",
		PF_MASK_LINUX | PF_MASK_ALLBSDS,
		"GNU as was not found on your system.\nInstall binutils to fix this."
		"Exiting..." );
}

int main( int ac, char * av[] )
{
	struct ini cfg;
	int r;

	ensure_elf_as( );

	r = ini_fromfile( "etc/hinterlib.ini", &cfg );

	if( !r )
	{
		uni_print( "Succeeded.\n" );
	}

	return 0;
}
