/****************************************************************************\
 *                                 Galaxian                                 *
 *                                                                          *
 *                         Copyright © 2021 Aquefir                         *
 *                 Released under Artisan Software Licence.                 *
\****************************************************************************/

#ifndef INC__INI2CFG_H
#define INC__INI2CFG_H

#include <uni/types/int.h>

enum
{
	CFG_MAX_NAME  = 128,
	CFG_MAX_IDENT = 64,
	CFG_MAX_URL   = 32768,
	CFG_MAX_FPATH = 32768,
	CFG_MAX_KEY   = 256,
	CFG_MAX_VALUE = 4096
};

enum
{
	CFG_FLAG_LINUX,
	CFG_FLAG_DARWIN,
	CFG_FLAG_WIN32,
	CFG_FLAG_WIN64,
	CFG_FLAG_GBA,
	CFG_FLAG_DOS,
	CFG_FLAG_IBMPC,
	CFG_FLAG_APE,
	CFG_MAX_FLAG
};

enum
{
	CFG_MASK_LINUX  = 1 << CFG_FLAG_LINUX,
	CFG_MASK_DARWIN = 1 << CFG_FLAG_DARWIN,
	CFG_MASK_WIN32  = 1 << CFG_FLAG_WIN32,
	CFG_MASK_WIN64  = 1 << CFG_FLAG_WIN64,
	CFG_MASK_GBA    = 1 << CFG_FLAG_GBA,
	CFG_MASK_DOS    = 1 << CFG_FLAG_DOS,
	CFG_MASK_IBMPC  = 1 << CFG_FLAG_IBMPC,
	CFG_MASK_APE    = 1 << CFG_FLAG_APE
};

struct src
{
	char ident[CFG_MAX_IDENT];
	u8 sha256sum[32];
	char url[CFG_MAX_URL];
};

struct dst
{
	char ident[CFG_MAX_IDENT];
	char src_ident[CFG_MAX_IDENT];
	char cwd[CFG_MAX_FPATH];
	u32 slick : 1;
};

struct cfg
{
	char name[CFG_MAX_NAME];
	char ident[CFG_MAX_IDENT];
	u32 tps : 8;
	u32 ver_major : 8;
	u32 ver_minor : 8;
	u32 ver_patch : 8;
	struct src * src;
	struct dst * dst;
	ptri src_sz, dst_sz;
};

struct ini_pair
{
	char key[CFG_MAX_KEY];
	char val[CFG_MAX_VALUE];
};

struct ini_section
{
	char name[CFG_MAX_IDENT];
	struct ini_pair * pairs;
	ptri pairs_sz;
};

struct ini
{
	struct ini_pair * globlpairs;
	ptri globlpairs_sz;
	struct ini_section * sections;
	ptri sections_sz;
};

int ini_fromfile( const char *, struct ini * );

#endif /* INC__INI2CFG_H */
