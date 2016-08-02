#!/bin/sh
#$Id$
#Copyright (c) 2012-2015 Pierre Pronchery <khorben@defora.org>
#This file is part of DeforaOS Desktop Mailer
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are
#met:
#
#1. Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
#2. Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
#3. Neither the name of the authors nor the names of the contributors may
#   be used to endorse or promote products derived from this software
#   without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS "AS IS" AND
#ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS
#BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#THE POSSIBILITY OF SUCH DAMAGE.


#variables
[ -n "$OBJDIR" ] || OBJDIR="./"
PROGNAME="tests.sh"
#executables
DATE="date"


#functions
#fail
_fail()
{
	_run "$@"
}


#run
_run()
{
	test="$1"

	shift
	echo -n "$test:" 1>&2
	(echo
	echo "Testing: $OBJDIR$test" "$@"
	LD_LIBRARY_PATH="$OBJDIR../src" "$OBJDIR$test" "$@") >> "$target" 2>&1
	res=$?
	if [ $res -ne 0 ]; then
		echo " FAIL (error $res)" 1>&2
	else
		echo " PASS" 1>&2
	fi
	return $res
}


#test
_test()
{
	test="$1"

	_run "$@"
	res=$?
	[ $res -eq 0 ] || FAILED="$FAILED $test(error $res)"
}


#usage
_usage()
{
	echo "Usage: $PROGNAME [-c][-P prefix] target" 1>&2
	return 1
}


#main
clean=0
while getopts "cP:" name; do
	case "$name" in
		c)
			clean=1
			;;
		P)
			#XXX ignored for compatibility
			;;
		?)
			_usage
			exit $?
			;;
	esac
done
shift $((OPTIND - 1))
if [ $# -ne 1 ]; then
	_usage
	exit $?
fi
target="$1"

[ "$clean" -ne 0 ]			&& exit 0

$DATE > "$target"
FAILED=
echo "Performing tests:" 1>&2
_test "date"
_test "email"
_test "imap4"
_test "pkgconfig.sh"
_test "plugins"
echo "Expected failures:" 1>&2
if [ -n "$FAILED" ]; then
	echo "Failed tests:$FAILED" 1>&2
	exit 2
fi
echo "All tests completed" 1>&2
