#!/bin/bash

source /etc/init.d/functions.sh
svcdir=/dev/shm/.init.d

myscript=${1}
if [ -L $1 ]
then
	myservice=`readlink ${1}`
else
	myservice=${1}
fi

myservice=${myservice##*/}
mylevel=`cat ${svcdir}/softlevel`

# Source configuration files.
# (1) Source /etc/conf.d/basic to get common configuration.
# (2) Source /etc/conf.d/${myservice} to get initscript-specific
#     configuration (if it exists).
# (3) Source /etc/rc.conf to pick up potentially overriding
#     configuration, if the system administrator chose to put it
#     there (if it exists).
[ -e /etc/conf.d/basic ]	&& source /etc/conf.d/basic
[ -e /etc/conf.d/${myservice} ] && source /etc/conf.d/${myservice}
[ -e /etc/rc.conf ]		&& source /etc/rc.conf

usage() {
	export IFS="|"
	myline="Usage: ${myservice} {$*"
	eerror "${myline}}"
}

stop() {
	#return success so the symlink gets removed
	return 0
}

start() {
	einfo "${myservice} does not have a start function."
	#return failure so the symlink doesn't get created
	return 1
}

svc_stop() {
	local x
	local stopfail="no"
	local mydeps
	local mydep
	if [ ! -L ${svcdir}/started/${myservice} ]
	then
		einfo "${myservice} has not yet been started."
		return 1
	fi
	if [ -L /etc/init.d/boot/${myservice} ]
	then
		einfo "Warning: you are stopping a boot service."
	fi
	if [ "${myservice##*.}" != "$myservice" ]
	then
		#net.* service
		if [ -L /etc/runlevels/boot/${myservice} ] || [ -L /etc/runlevels/${mylevel}/${myservice} ]
		then
			mydeps="net $myservice"
		else
			mydeps=$myservice
		fi
	else
		mydeps=$myservice
	fi
	for mydep in $mydeps
	do
		for mytype in need use
		do
			if [ -d ${svcdir}/${mytype}/${mydep} ]
			then
				for x in ${svcdir}/${mytype}/${mydep}/*
				do
					if [ ! -L ${x} ]
					then
						continue
					fi
					if [ ! -L ${svcdir}/started/${x##*/} ]
					then
						#service not currently running, continue
					# This breaks the need's, and since they do not get
					# regenerated, things will break when the services
					# gets started again.  We should keep the need's and
					# use's (when actually official) intact at all time
					# anyhow.
					#	rm ${x}
						continue
					fi
					${x} stop
					if [ $? -ne 0 ]
					then
						stopfail="yes"
						break
					# See above.  The need's and use's needs to stay intact.
					#else
					#	rm ${x}
					fi
				done
				if [ "$stopfail" = "yes" ]
				then
					einfo "Problems stopping dependent services.  ${myservice} still up."
					exit 1
				fi
			fi
		done
	done
	#now that deps are stopped, stop our service
	stop
	if [ $? -eq 0 ]
	then
		rm ${svcdir}/started/${myservice}
	else
		return $?
	fi
}

svc_start() {
	local retval
	local x
	local y
	local myserv
	if [ ! -L ${svcdir}/started/${myservice} ]
	then
		#link first to prevent possible recursion
		ln -s /etc/init.d/${myservice} ${svcdir}/started/${myservice}

		#start dependencies, if any
		for x in `ineed ${myservice}` `valid_iuse ${myservice}`
		do
			if [ "$x" = "net" ]
			then
				for y in /etc/runlevels/boot/net.* /etc/runlevels/${mylevel}/net.*
				do
					myserv=${y##*/}
					if [ ! -L ${svcdir}/started/${myserv} ]
					then
						/etc/init.d/${myserv} start
					fi
				done
			else
				if [ ! -L ${svcdir}/started/${x} ]
				then
					/etc/init.d/${x} start
				fi
			fi
		done
		#start service
		start
		retval=$?

		#remove link if service didn't start; but only if we're not booting
		#if we're booting, we need to continue and do our best to get the
		#system up.
		if [ "$SOFTLEVEL" = "boot" ]
		then
			return $retval
		elif [ $retval -ne 0 ]
		then
			rm ${svcdir}/started/${myservice}
			return $retval
		fi
	else
		einfo "${myservice} has already been started."
		return 1
	fi
}

source ${myscript}
if [ "$opts" = "" ]
then
	opts="start stop restart"
fi

needsme() {
	local x
	if [ -d ${svcdir}/need/${1} ]
	then
		for x in ${svcdir}/need/${1}/*
		do
			if [ ! -L $x ]
			then
				continue
			fi
			echo ${x##*/}
		done
	fi
}

usesme() {
	local x
	if [ -d ${svcdir}/use/${1} ]
	then
		for x in ${svcdir}/use/${1}/*
		do
			if [ ! -L $x ]
			then
				continue
			fi
			echo ${x##*/}
		done
	fi
}

ineed() {
	local x
	local z
	for x in ${svcdir}/need/*/${1}
	do
		if [ ! -L ${x} ]
		then
			continue
		fi
		z=${x%/*}
		echo ${z##*/}
	done
}

#this will give all the use's of the service, even if not in current or boot
#runlevels
iuse() {
    local x
    local z
    for x in ${svcdir}/use/*/${1}
    do
	if [ ! -L ${x} ]
	then
	    continue
	fi
	z=${x%/*}
	echo ${z##*/}
    done
}

#this will only give the valid use's for the service (they must be in the boot
#or current runlevel)
valid_iuse() {
	local x
	local y
	for x in `iuse ${1}`
	do
		if [ -e /etc/runlevels/boot/${x} ] || [ -e /etc/runlevels/${mylevel}/${x} ]
		then
			z=${x%/*}
			echo ${z##*/}
		fi
	done
}

#call this with "needsme", "ineed", "usesme" or "iuse" as first arg
query() {
	local deps
	local x
	install -d -m0755 ${svcdir}/depcheck/$$
	if [ "$1" = "ineed" ] && [ ! -L ${svcdir}/started/${myservice} ]
	then
		einfo "Warning: ${myservice} not running. need info may not be accurate."
	fi
	if [ "$1" = "iuse" ] && [ ! -L ${svcdir}/started/${myservice} ]
	then
		einfo "Warning: ${myservice} not running. use info may not be accurate."
	fi

	deps="${myservice}"
	while [ "$deps" != "" ]
	do
		deps=`${1} ${deps}`
		for x in $deps
		do
			if [ ! -e ${svcdir}/depcheck/$$/${x} ]
			then
				touch ${svcdir}/depcheck/$$/${x}
			fi
		done
	done
	for x in ${svcdir}/depcheck/$$/*
	do
		if [ ! -e $x ]
		then
			continue
		fi
		echo ${x##*/}
	done
	rm -rf ${svcdir}/depcheck/
}

svc_homegrown() {
	local arg="$1" x
	# Walk through the list of available options, looking for the
	# requested one.
	for x in $opts; do
		if [ $x = "$arg" ]; then
			if typeset -F $x &>/dev/null; then
				# Run the homegrown function
		$x
		return $?
			else
				# This is a weak error message
				echo "Function $x doesn't exist."
				usage $opts
				exit 1
			fi
		fi
	done
	# If we're here, then the function wasn't in $opts.  This is
	# the same error message that used to be in the case statement
	# before homegrown functions were supported.
	echo "wrong args. (  $arg / $* )"
	usage $opts
	exit 1
}

shift
if [ $# -lt 1 ]
then
	echo "not enough args."
	usage $opts
	exit 1
fi
for arg in ${*}
do
	case $arg in
	stop)
		svc_stop
		;;
	start)
		svc_start
		;;
	needsme|ineed|usesme|iuse)
		query $arg
		;;
	zap)
		if [ -e ${svcdir}/started/${myservice} ]
		then
			einfo "Manually resetting ${myservice} to stopped state."
			rm ${svcdir}/started/${myservice}
		fi
		;;
	restart)
		#add snapshot support here so any dependent services that
		#are stopped are restarted after the svc_start
		if [ -e ${svcdir}/started/${myservice} ]
		then
			svc_stop
			sleep 1
		fi
		svc_start
		;;
	*)
		# Allow for homegrown functions
		svc_homegrown $arg
		;;
	esac
done

# vim:ts=4
