#!/bin/sh
# $Id: edit_cfg.sh,v 1.4 1998/02/11 12:13:41 tom Exp $
##############################################################################
# Copyright (c) 1998 Free Software Foundation, Inc.                          #
#                                                                            #
# Permission is hereby granted, free of charge, to any person obtaining a    #
# copy of this software and associated documentation files (the "Software"), #
# to deal in the Software without restriction, including without limitation  #
# the rights to use, copy, modify, merge, publish, distribute, distribute    #
# with modifications, sublicense, and/or sell copies of the Software, and to #
# permit persons to whom the Software is furnished to do so, subject to the  #
# following conditions:                                                      #
#                                                                            #
# The above copyright notice and this permission notice shall be included in #
# all copies or substantial portions of the Software.                        #
#                                                                            #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    #
# THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        #
# DEALINGS IN THE SOFTWARE.                                                  #
#                                                                            #
# Except as contained in this notice, the name(s) of the above copyright     #
# holders shall not be used in advertising or otherwise to promote the sale, #
# use or other dealings in this Software without prior written               #
# authorization.                                                             #
##############################################################################
#
# Author: Thomas E. Dickey <dickey@clark.net> 1997
#
# Edit the default value of the etip.h file based on the autoconf-generated
# values:
#
#	$1 = ncurses_cfg.h
#	$2 = etip.h
#
for name in \
	HAVE_BUILTIN_H \
	HAVE_TYPEINFO \
	HAVE_VALUES_H
do
	mv $2 $2.bak
	if ( grep "[ 	]$name[ 	]1" $1 2>&1 >/dev/null)
	then
		sed -e 's/define '$name'.*$/  define '$name' 1/' $2.bak >$2
	else
		sed -e 's/define '$name'.*$/  define '$name' 0/' $2.bak >$2
	fi
	if (cmp -s $2 $2.bak)
	then
		echo '** same: '$name
		mv $2.bak $2
	else
		echo '** edit: '$name
		rm -f $2.bak
	fi
done
