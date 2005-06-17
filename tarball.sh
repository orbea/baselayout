#!/bin/bash
export TMP="${TMP:-/tmp}"
export V="1.7.1"
export DEST="${TMP}/rc-scripts-${V}"

if [[ $1 != "-f" ]] ; then
	echo "Performing sanity checks (run with -f to skip) ..."

	# Check that we're updated
	svnfiles=$( svn status 2>&1 | egrep -v '^(U|P)' )
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
rm -rf ${DEST}/src/core

# copy net-scripts and remove older stuff
install -d -m0755 ${DEST}/lib/rcscripts
cp -ax net-scripts/init.d ${DEST}
cp -ax net-scripts/net.modules.d ${DEST}/lib/rcscripts
cp -ax net-scripts/conf.d ${DEST}/etc
ln -sfn net.lo ${DEST}/init.d/net.eth0

cp ChangeLog ${DEST}

chown -R root:root ${DEST}
chmod 0755 ${DEST}/sbin/*
chmod 0755 ${DEST}/init.d/*
( cd $TMP/rc-scripts-${V} ; rm -rf `find -iname .svn` )
cd $TMP
tar cjvf ${TMP}/rc-scripts-${V}.tar.bz2 rc-scripts-${V}
rm -rf rc-scripts-${V}

echo
du -b ${TMP}/rc-scripts-${V}.tar.bz2
