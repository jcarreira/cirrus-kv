#!/bin/sh
#
# The KADM5 unit tests were developed to work under gmake.  As a
# result, they expect to inherit a number of environment variables.
# Rather than rewrite the tests, we simply use this script as an
# execution wrapper that sets all the necessary environment variables
# before running the program specified on its command line.
#
# The variable settings all came from OV's config.mk.
#
# Usage: env-setup.sh <command line>
#

TOP=/ebs/yangryan/cirrus/third_party/krb5-1.16/src/kadmin
STOP=/ebs/yangryan/cirrus/third_party/krb5-1.16/src/./kadmin
export TOP
export STOP
# These two may be needed in case $libdir references them.
prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib ; eval "libdir=$libdir"; export libdir

# The shared library run time setup
TOPLIBD=/ebs/yangryan/cirrus/third_party/krb5-1.16/src/lib
PROG_LIBPATH=-L/ebs/yangryan/cirrus/third_party/krb5-1.16/src/lib
BUILDTOP=/ebs/yangryan/cirrus/third_party/krb5-1.16/src
# XXX kludge!
PROG_RPATH=/ebs/yangryan/cirrus/third_party/krb5-1.16/src/lib
# This converts $(TOPLIBD) to $TOPLIBD
cat > /tmp/env_setup$$ <<\EOF
LD_LIBRARY_PATH=`echo $(PROG_LIBPATH) | sed -e "s/-L//g" -e "s/ /:/g"`
EOF

foo=`sed -e 's/(//g' -e 's/)//g' -e 's/\\\$\\\$/\$/g' /tmp/env_setup$$`
eval $foo
export LD_LIBRARY_PATH

# This will get put in setup.csh for convenience
KRB5_RUN_ENV_CSH=`eval echo "$foo" | \
	sed -e 's/\([^=]*\)=\(.*\)/setenv \1 \2/g'`
export KRB5_RUN_ENV_CSH
rm /tmp/env_setup$$

TESTDIR=$TOP/testing; export TESTDIR
STESTDIR=$STOP/testing; export STESTDIR
if [ "$K5ROOT" = "" ]; then
	K5ROOT="`cd $TESTDIR; pwd`/krb5-test-root"
	export K5ROOT
fi

# If $VERBOSE_TEST is non-null, enter verbose mode.  Set $VERBOSE to
# true or false so its exit status identifies the mode.
if test x$VERBOSE_TEST = x; then
	VERBOSE=false
else
	VERBOSE=true
fi
export VERBOSE

REALM=SECURE-TEST.OV.COM; export REALM

if test x$EXPECT = x; then
    EXPECT=; export EXPECT
fi

COMPARE_DUMP=$TESTDIR/scripts/compare_dump.pl; export COMPARE_DUMP
INITDB=$STESTDIR/scripts/init_db; export INITDB
MAKE_KEYTAB=$TESTDIR/scripts/make-host-keytab.pl; export MAKE_KEYTAB
LOCAL_MAKE_KEYTAB=$TESTDIR/scripts/make-host-keytab.pl
export LOCAL_MAKE_KEYTAB
SIMPLE_DUMP=$TESTDIR/scripts/simple_dump.pl; export SIMPLE_DUMP
QUALNAME=$TESTDIR/scripts/qualname.pl; export QUALNAME
TCLUTIL=$STESTDIR/tcl/util.t; export TCLUTIL
BSDDB_DUMP=$TESTDIR/util/bsddb_dump; export BSDDB_DUMP
CLNTTCL=$TESTDIR/util/kadm5_clnt_tcl; export CLNTTCL
SRVTCL=$TESTDIR/util/kadm5_srv_tcl; export SRVTCL

KRB5_CONFIG=$K5ROOT/krb5.conf; export KRB5_CONFIG
KRB5_KDC_PROFILE=$K5ROOT/kdc.conf; export KRB5_KDC_PROFILE
KRB5_KTNAME=$K5ROOT/ovsec_adm.srvtab; export KRB5_KTNAME
KRB5_CLIENT_KTNAME=$K5ROOT/client_keytab; export KRB5_CLIENT_KTNAME
KRB5CCNAME=$K5ROOT/krb5cc_unit-test; export KRB5CCNAME

# Make sure we don't get confused by translated messages
# or localized times.
LC_ALL=C; export LC_ALL

if [ "$TEST_SERVER" != "" ]; then
	MAKE_KEYTAB="$MAKE_KEYTAB -server $TEST_SERVER"
fi
if [ "$TEST_PATH" != "" ]; then
	MAKE_KEYTAB="$MAKE_KEYTAB -top $TEST_PATH"
fi

if [ "x$PS_ALL" = "x" ]; then
	if ps auxww >/dev/null 2>&1; then
		PS_ALL="ps auxww"
		PS_PID="ps uwwp"
	elif ps -ef >/dev/null 2>&1; then
		PS_ALL="ps -ef"
		PS_PID="ps -fp"
	else
		PS_ALL="ps auxww"
		PS_PID="ps uwwp"
		echo "WARNING!  Cannot auto-detect ps type, assuming BSD."
	fi

	export PS_ALL PS_PID
fi

exec ${1+"$@"}
