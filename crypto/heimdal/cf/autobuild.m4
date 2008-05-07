# autobuild.m4 serial 2 (autobuild-3.3)
# Copyright (C) 2004 Simon Josefsson
#
# This file is free software, distributed under the terms of the GNU
# General Public License.  As a special exception to the GNU General
# Public License, this file may be distributed as part of a program
# that contains a configuration script generated by Autoconf, under
# the same distribution terms as the rest of that program.
#
# This file can can be used in projects which are not available under
# the GNU General Public License or the GNU Library General Public
# License but which still want to provide support for Autobuild.

# Usage: AB_INIT([MODE]).
AC_DEFUN([AB_INIT],
[
	AC_REQUIRE([AC_CANONICAL_BUILD])
	AC_REQUIRE([AC_CANONICAL_HOST])

	AC_MSG_NOTICE([autobuild project... ${PACKAGE_NAME:-$PACKAGE}])
	AC_MSG_NOTICE([autobuild revision... ${PACKAGE_VERSION:-$VERSION}])
	hostname=`hostname`
	if test "$hostname"; then
	   AC_MSG_NOTICE([autobuild hostname... $hostname])
	fi
	ifelse([$1],[],,[AC_MSG_NOTICE([autobuild mode... $1])])
	date=`date +%Y%m%d-%H%M%S`
	if test "$?" != 0; then
	   date=`date`
	fi
	if test "$date"; then
	   AC_MSG_NOTICE([autobuild timestamp... $date])
	fi
])
