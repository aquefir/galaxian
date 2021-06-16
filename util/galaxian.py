#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import hashlib
import shutil
import sys
import tempfile
import urllib.request

def run(command):
	import subprocess
	import sys
	try:
		return subprocess.check_output(command).decode()
	except subprocess.CalledProcessError as e:
		print(e.output.decode(), file=sys.stderr)
		sys.exit(1)

def ini_parse(text):
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

def cfg_parse(ini):
	ret = {
		'sources': [],
		'projects': [],
		'targets': ini['']['tp'].upper().split(','),
		'ver': ini['']['ver']
	}
	for section in ini:
		if section.endswith('.src'):
			ret['sources'][section[:-4]] = {
				'url': ini[section]['url'],
				'sha2_256sum': ini[section]['sha2_256sum']
			}
		elif section.endswith('.proj'):
			ret['projects'][section[:-5]] = {
				'srcs':
			}
