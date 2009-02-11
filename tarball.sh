#!/bin/bash
[[ ${UID} -eq 0 ]] || exec sudo "$0" "$@"

export TMP="${TMP:-/tmp}"
export V="$1"
export NAME="baselayout"
export DEST="${TMP}/${NAME}-${V}"

if [[ -z $1 ]] ; then
	echo "Usage: ${0} <ver> [-f]" 1>&2
	exit 1
else
	echo "Make sure to update VERSION in Makefile"
	shift
fi

if [[ $1 != "-f" ]] ; then
	echo "Performing sanity checks (run with -f to skip) ..."

	# Check that we're updated
	svnfiles=$( svn status --no-ignore 2>&1 | egrep -v '^(U|P)' )
	if [[ -n ${svnfiles} ]] ; then
		echo "Refusing to package tarball until svn is in sync:"
		echo "$svnfiles"
		exit 1
	fi
fi

echo "Creating tarball ..."
rm -rf ${DEST}
install -d -m0755 ${DEST}

for x in bin etc init.d sbin src rc-lists man ; do
	cp -ax $x ${DEST}
done

# do not yet package src/core stuff
#rm -rf ${DEST}/src/core

# Make clean
(cd ${DEST}/src; make clean)
(cd ${DEST}/src/core; make clean)

# copy net-scripts and remove older stuff
install -d -m0755 ${DEST}/lib/rcscripts
cp -ax net-scripts/init.d ${DEST}
cp -ax net-scripts/net ${DEST}/lib/rcscripts
cp -ax net-scripts/conf.d ${DEST}/etc
ln -sfn net.lo ${DEST}/init.d/net.eth0

cp ChangeLog ${DEST}

( cd $TMP/${NAME}-${V} ; rm -rf `find -iname .svn` )
chown -R root:root ${DEST}
chmod 0755 ${DEST}/sbin/*
chmod 0755 ${DEST}/init.d/*
cd $TMP
tar cjf ${TMP}/${NAME}-${V}.tar.bz2 ${NAME}-${V}
rm -rf ${NAME}-${V}

echo
du -b ${TMP}/${NAME}-${V}.tar.bz2
