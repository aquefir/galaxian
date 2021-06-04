/****************************************************************************\
 *                                 Galaxian                                 *
 *                                                                          *
 *                         Copyright © 2021 Aquefir                         *
 *                 Released under Artisan Software Licence.                 *
\****************************************************************************/

#include "ini2cfg.h"

#include <uni/err.h>
#include <uni/futils.h>
#include <uni/memory.h>
#include <uni/str.h>

enum
{
	PARSER_BLANK = 0,
	PARSER_SECTION,
	PARSER_KEY,
	PARSER_VALUE,
	BUF_SZ = 4096
};

enum
{
	LEXEME_MALFORMED,
	LEXEME_BLANK,
	LEXEME_SECTION,
	LEXEME_PAIR,
	LEXEME_COMMENT,
	MAX_LEXEME
};

enum
{
	STATE_MALFORMED,
	STATE_BLANK,
	STATE_SECTION,
	STATE_KEY,
	STATE_VALUE,
	STATE_COMMENT,
	STATE_POST_SECTION,
	MAX_STATE
};

static const int fstate2lexeme[MAX_STATE] = {
	LEXEME_MALFORMED,
	LEXEME_BLANK,
	LEXEME_MALFORMED,
	LEXEME_MALFORMED,
	LEXEME_PAIR,
	LEXEME_COMMENT,
	LEXEME_SECTION,
};

struct lexeme
{
	u8 type;
	char * value;
	char * key;
};

static int is_wspace( char c )
{
	return c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r';
}

static struct lexeme _parse_line( const char * line )
{
	ptri i, buf_i;
	struct lexeme ret, tmp;
	unsigned s = STATE_BLANK;
	char buf[BUF_SZ];

	uni_memset( &ret, 0, sizeof( ret ) );
	uni_memset( &tmp, 0, sizeof( tmp ) );
	uni_memset( buf, 0, BUF_SZ );

	ret.type = LEXEME_MALFORMED;

	for( i = 0; line[i] != '\0'; ++i )
	{
		char c = line[i];

		if( ( (u8)c ) > 0x7F )
		{
			return ret;
		}
		else if( s == STATE_BLANK )
		{
			if( is_wspace( c ) )
			{
			}
			else if( c == '[' )
			{
				s     = STATE_SECTION;
				buf_i = 0;
				uni_memset( buf, 0, BUF_SZ );
			}
			else if( c == ';' )
			{
				s     = STATE_COMMENT;
				buf_i = 0;
				uni_memset( buf, 0, BUF_SZ );
			}
			else
			{
				s     = STATE_KEY;
				buf_i = 0;
				uni_memset( buf, 0, BUF_SZ );
			}
		}
		else if( s == STATE_SECTION )
		{
			if( c == ']' )
			{
				s = STATE_POST_SECTION;
			}
			else
			{
				buf[buf_i] = c;
				buf_i++;
			}
		}
		else if( s == STATE_KEY )
		{
			if( c == '=' )
			{
				if( buf_i == 0 )
				{
					return ret;
				}
				else
				{
					const ptri buf_len = uni_strlen( buf );

					tmp.key = uni_alloc0( buf_len + 1 );
					uni_memcpy( tmp.key, buf, buf_len );

					buf_i = 0;
					uni_memset( buf, 0, BUF_SZ );

					s = STATE_VALUE;
				}
			}
			else
			{
				buf[buf_i] = c;
				buf_i++;
			}
		}
		else if( s == STATE_VALUE || s == STATE_COMMENT )
		{
			buf[buf_i] = c;
			buf_i++;
		}
		else if( s == STATE_POST_SECTION )
		{
			if( !is_wspace( c ) )
			{
				return ret;
			}
		}
	}

	tmp.type = fstate2lexeme[s];

	if( tmp.type != LEXEME_MALFORMED )
	{
		const ptri buf_len = uni_strlen( buf );

		tmp.value = uni_alloc0( buf_len + 1 );
		uni_memcpy( tmp.value, buf, buf_len );
	}
	else if( tmp.key != NULL )
	{
		/* key is only provided when .type == LEXEME_PAIR
		 * here it’s a line with just text */
		tmp.value = tmp.key;
		tmp.key   = NULL;
	}
	else if( s == STATE_SECTION )
	{
		const ptri buf_len = uni_strlen( buf );

		tmp.value = uni_alloc0( buf_len + 1 );
		uni_memcpy( tmp.value, buf, buf_len );
	}

	uni_memcpy( &ret, &tmp, sizeof( struct lexeme ) );

	return ret;
}

static int _get_lexemes( const char * fpath, struct lexeme ** out, ptri * sz )
{
	int r;
	u8 * bytes;
	char ** lines;
	ptri bytes_sz, lexemes_sz, lexemes_i, i;
	struct lexeme * lexemes;

	r = uni_loadfile( fpath, &bytes, &bytes_sz );

	if( r )
	{
		return 1;
	}

	if( uni_strlen( (const char *)bytes ) != bytes_sz ||
		!uni_isascii( (const char *)bytes ) )
	{
		return 2;
	}

	lines = uni_strsplit( (const char *)bytes, "\n", 0 );

	lexemes_i  = 0;
	lexemes_sz = 16;
	lexemes    = uni_alloc0( sizeof( struct lexeme ) * lexemes_sz );

	for( i = 0; lines[i] != NULL; ++i )
	{
		struct lexeme lexeme = _parse_line( lines[i] );

		if( lexeme.type == LEXEME_MALFORMED )
		{
			return 3;
		}

		if( lexemes_i >= lexemes_sz )
		{
			lexemes_sz <<= 1; /* *= 2 */
			lexemes = uni_realloc( lexemes,
				sizeof( struct lexeme ) * lexemes_sz );
		}

		uni_memcpy( &( lexemes[lexemes_i] ),
			&lexeme,
			sizeof( struct lexeme ) );
		lexemes_i++;
	}

	*out = lexemes;
	*sz  = lexemes_i;

	return 0;
}

int _ini_fromfile( struct lexeme * lexemes, ptri lexemes_sz, struct ini * ini )
{
	ptri i, globlpairs_i = 0, globlpairs_sz = 16, sections_i = 0,
		sections_sz = 16, pairs_i = 0;
	unsigned globl = 1;
	struct ini_pair * globlpairs;
	struct ini_section * sections;
	struct ini ret;

	if( !lexemes || !ini )
	{
		uni_die( );
	}

	globlpairs = uni_alloc0( sizeof( struct ini_pair ) * globlpairs_sz );
	sections   = uni_alloc0( sizeof( struct ini_section ) * sections_sz );

	uni_memset( &ret, 0, sizeof( ret ) );

	for( i = 0; i < lexemes_sz; ++i )
	{
		if( lexemes[i].type == LEXEME_MALFORMED )
		{
			return 1;
		}
		else if( lexemes[i].type == LEXEME_PAIR )
		{
			ptri key_sz   = uni_strlen( lexemes[i].key ),
			     value_sz = uni_strlen( lexemes[i].value );

			if( globl )
			{
				uni_memcpy( globlpairs[globlpairs_i].key,
					lexemes[i].key,
					key_sz <= CFG_MAX_KEY ? key_sz
							      : CFG_MAX_KEY );
				globlpairs_i++;
			}
			else
			{
				if( sections[sections_i].pairs_sz == 0 )
				{
					sections[sections_i].pairs_sz = 16;
					sections[sections_i]
						.pairs = uni_alloc0(
						sizeof( struct ini_pair ) *
						sections[sections_i]
							.pairs_sz );
				}
				else if( pairs_i >=
					sections[sections_i].pairs_sz )
				{
					sections[sections_i].pairs_sz <<=
						1; /* *= 2 */
					sections[sections_i]
						.pairs = uni_realloc(
						sections[sections_i].pairs,
						sections[sections_i]
							.pairs_sz );
				}

				uni_memcpy( sections[sections_i]
						    .pairs[pairs_i]
						    .key,
					lexemes[i].key,
					key_sz <= CFG_MAX_KEY ? key_sz
							      : CFG_MAX_KEY );
				uni_memcpy( sections[sections_i]
						    .pairs[pairs_i]
						    .val,
					lexemes[i].value,
					value_sz <= CFG_MAX_VALUE
						? value_sz
						: CFG_MAX_VALUE );
				pairs_i++;
			}
		}
		else if( lexemes[i].type == LEXEME_SECTION )
		{
			ptri name_sz = uni_strlen( lexemes[i].value );

			if( globl )
			{
				/* finalise the global section */
				if( globlpairs_i == 0 )
				{
					uni_free( globlpairs );
					globlpairs    = NULL;
					globlpairs_sz = 0;
				}
				else
				{
					struct ini_pair * pairs;
					ptri pairs_sz = globlpairs_i;

					pairs = uni_alloc(
						sizeof( struct ini_pair ) *
						pairs_sz );

					uni_memcpy( pairs,
						globlpairs,
						sizeof( struct ini_pair ) *
							pairs_sz );
					uni_free( globlpairs );

					globlpairs    = pairs;
					globlpairs_sz = pairs_sz;
				}
			}
			else
			{
				/* finalise the previous section */
				if( pairs_i == 0 )
				{
					uni_free( sections[sections_i].pairs );
					sections[sections_i].pairs    = NULL;
					sections[sections_i].pairs_sz = 0;
				}
				else
				{
					struct ini_pair * pairs;
					ptri pairs_sz = pairs_i;

					pairs = uni_alloc(
						sizeof( struct ini_pair ) *
						pairs_sz );

					uni_memcpy( pairs,
						sections[sections_i].pairs,
						sizeof( struct ini_pair ) *
							pairs_sz );
					uni_free( sections[sections_i].pairs );

					sections[sections_i].pairs = pairs;
					sections[sections_i].pairs_sz =
						pairs_sz;
				}

				sections_i++;
				globl = 0;
			}

			if( sections_i >= sections_sz )
			{
				sections_sz <<= 1; /* *= 2 */
				sections =
					uni_realloc( sections, sections_sz );
			}

			uni_memcpy( sections[sections_i].name,
				lexemes[i].value,
				name_sz <= CFG_MAX_NAME ? name_sz
							: CFG_MAX_NAME );
			sections[sections_i].pairs_sz = 16;
			sections[sections_i].pairs =
				uni_alloc0( sizeof( struct ini_pair ) *
					sections[sections_i].pairs_sz );
			pairs_i = 0;
		}
		/* else:
		 * LEXEME_COMMENT, LEXEME_BLANK => continue */
	}

	ret.globlpairs_sz = globlpairs_i;

	if( ret.globlpairs_sz == 0 )
	{
		ret.globlpairs = NULL;
	}
	else
	{
		ret.globlpairs = uni_alloc(
			sizeof( struct ini_pair ) * ret.globlpairs_sz );
		uni_memcpy( ret.globlpairs,
			globlpairs,
			sizeof( struct ini_pair ) * ret.globlpairs_sz );
		uni_free( globlpairs );
	}

	ret.sections_sz = sections_i;

	if( ret.sections_sz == 0 )
	{
		ret.sections = NULL;
	}
	else
	{
		ret.sections = uni_alloc(
			sizeof( struct ini_section ) * ret.sections_sz );
		uni_memcpy( ret.sections,
			sections,
			sizeof( struct ini_section ) * ret.sections_sz );
		uni_free( sections );
	}

	uni_memcpy( ini, &ret, sizeof( struct ini ) );

	return 0;
}

int ini_fromfile( const char * fpath, struct ini * ini )
{
	int r;
	struct lexeme * lexemes;
	ptri lexemes_sz;

	if( !fpath || !ini )
	{
		uni_die( );
	}

	r = _get_lexemes( fpath, &lexemes, &lexemes_sz );

	if( r )
	{
		return r;
	}

	return _ini_fromfile( lexemes, lexemes_sz, ini );
}
