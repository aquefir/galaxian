#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import urllib.request
import hashlib
from shutil import move, rmtree, unpack_archive
from sys import stderr
from tempfile import mkdtemp, mkstemp

def run(command):
	import subprocess
	from sys import exit, stderr
	try:
		return subprocess.check_output(command).decode()
	except subprocess.CalledProcessError as e:
		print(e.output.decode(), file=stderr)
		exit(1)

def parse_ini(text):
	ret = {'': {}}
	cur_s = '' # Empty string is the global section
	text = text.split('\n')
	for i, ln in enumerate(text):
		# Remove comments, anywhere they appear
		ln = ln.split('#', 1)[0]
		if '[' in ln and ']' in ln:
			s = ln.lstrip(' \t\v\f\r').rstrip(' \t\v\f\r')
			if s[0] != '[' or s[-1] != ']':
				# bad section formatting
				raise Exception('Malformed section heading on line ' \
					+ str(i + 1) + '\n\n' + ln)
			else:
				cur_s = s[1:-1]
				ret[cur_s] = {}
			# done parsing, don’t try to find a keypair that isn’t here
			continue
		elif '=' not in ln:
			# Not dealing with a keypair, check it
			for ch in ln:
				# only whitespace is permitted outside of comments and keypairs
				if ch != ' ' and ch != '\t' and ch != '\v' and ch != '\f' \
				and ch != '\r':
					raise Exception('Malformed syntax on line ' + str(i + 1) \
						+ '\n\n' + ln)
			# don’t try to parse this, there’s nothing here
			continue
		# NOTE: no whitespace stripping is done. this is deliberate
		p = ln.split('=', 1)
		k = p[0]
		v = p[1]
		if k == '' or v == '':
			# either the key name or value (or both) are empty
			raise Exception('Bad key/value pair on line ' + str(i + 1) \
				+ '\n\n' + ln)
		ret[cur_s][k] = v
	return ret

def parse_cfg(ini):
	ret = {
		'projs': [],
		'tps': ini['']['tp'].split(','),
		'ident': ini['']['ident'],
		'ver': ini['']['ver'],
		'outdir': ''
	}
	outdir = mkdtemp(suffix='-galaxian')
	ret['outdir'] = outdir
	print('Output directory: ' + outdir, file=stderr)
	for s in ini:
		if s.endswith('.src'):
			fname, headers = urllib.request.urlretrieve(ini[s]['url'])
			urltail = ini[s]['url'].split('/')[-1]
			newfname = mkstemp(suffix='.' + urltail)[1]
			os.rename(fname, newfname)
			fname = newfname
			f = open(fname, 'rb')
			data_hash = hashlib.sha256()
			# Read and update hash string value in blocks of 4K
			for block in iter(lambda: f.read(4096), b''):
				data_hash.update(block)
			digest = data_hash.hexdigest()
			expected = ini[s]['sha2_256sum']
			if digest != expected:
				os.remove(fname)
				raise Exception('SHA2-256 checksum mismatch! Server file hash was %s, INI file expected %s' % (digest, expected))
			unpack_archive(fname, os.path.join(outdir, s[:-4]))
			os.remove(fname)
		elif s.endswith('.dst'):
			if ini[s]['slick'] != '1':
				continue
			ret['projs'] += [os.path.join(ini[s]['src'], ini[s]['cwd'])]
	return ret

HELP_TEXT = '''
Galaxian Package Management
Copyright © 2021 Aquefir
Released under Artisan Software Licence.

Usage:-
\tgalaxian -h|--help                  Print this text.
\tgalaxian -c <file>|--config=<file>  Read <file> as INI config.
\t         [-p <TP>|--platform=<TP>]  Build package for <TP>.

\tIf no -p arguments are given, none are passed to Slick, which defaults
\tto a native build only.
'''

def main(args):
	argc = len(args)
	if argc == 1 or '-h' in args or '--help' in args:
		print(HELP_TEXT, file=stderr)
		return 0
	cfg = ''
	tp = []
	i = 1
	while i < argc:
		arg = args[i]
		if arg == '-c' or arg == '--config':
			if i + 1 >= argc:
				print('Dangling -c argument. Exiting...', file=stderr)
				return 127
			if cfg != '':
				print('WARNING: Config file specified multiple times.\n' +
					'Using the last value given...', file=stderr)
			cfg = args[i + 1]
			i += 1
		elif arg.startswith('--config='):
			if cfg != '':
				print('WARNING: Config file specified multiple times.\n' +
					'Using the last value given...', file=stderr)
			cfg = arg[9:]
		elif arg == '-p' or arg == '--platform':
			if i + 1 >= argc:
				print('Dangling -p argument. Exiting...', file=stderr)
				return 127
			cur_tp = args[i + 1].upper()
			i += 1
			if cur_tp not in tp:
				tp.push(cur_tp)
		else:
			print('WARNING: Unknown argument %s. Ignoring...' % arg, file=stderr)
		i += 1
	print(cfg, file=stderr)
	return 0
	f = open('etc/hinterlib.ini', 'r')
	ini = parse_ini(f.read())
	f.close()
	cfg = parse_cfg(ini)
	o_cwd = os.getcwd()
	os.chdir()
	print(cfg)
	rmtree(cfg['outdir'])
	i = 0
	projs_sz = len(cfg['projs'])
	while i < projs_sz:
		i += 1
	return 0

if __name__ == '__main__':
	from sys import argv, exit
	exit(main(argv))
