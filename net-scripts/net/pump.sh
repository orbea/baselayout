# Copyright (c) 2004-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# Contributed by Roy Marples (uberlord@gentoo.org)

# Fix any potential localisation problems
# Note that LC_ALL trumps LC_anything_else according to locale(7)
pump() {
	LC_ALL=C /sbin/pump "$@"
}

# void pump_depend(void)
#
# Sets up the dependancies for the module
pump_depend() {
	after interface
	provide dhcp
	functions interface_exists interface_get_address
}

# void pump_expose(void)
#
# Expose variables that can be configured
pump_expose() {
	variables pump dhcp
}

# bool pump_check_installed(void)
#
# Returns 1 if pump is installed, otherwise 0
pump_check_installed() {
	[[ -x /sbin/pump ]] && return 0
	${1:-false} && eerror "For DHCP (pump) support, emerge net-misc/pump"
	return 1
}

# bool pump_stop(char *iface)
#
# Stop pump on an interface
# Return 0 if pump is not running or we stop it successfully
# Otherwise 1
pump_stop() {
	local iface="$1" count= e=

	# We check for a pump process first as querying for status
	# causes pump to spawn a process
	pidof /sbin/pump &>/dev/null || return 0

	# Check that pump is running on the interface
	pump --status --interface "${iface}" 2>/dev/null \
		| grep -q "^Device ${iface}" || return 0

	# Pump always releases the lease
	ebegin "Stopping pump on ${iface}"
	pump --release --interface "${iface}"
	eend $? "Failed to stop pump"
}

# bool pump_start(char *iface)
#
# Start pump on an interface by calling pumpcd $iface $options
#
# Returns 0 (true) when a dhcp address is obtained, otherwise
# the return value from pump
pump_start() {
	local iface="$1" opts= d= ifvar=$(bash_variable "$1") search=

	interface_exists "${iface}" true || return 1

	opts="pump_${ifvar}"
	opts="${pump} ${!opts}"

	# Map some generic options to pump
	d="dhcp_${ifvar}"
	d=" ${!d} "
	[[ ${d} == "  " ]] && d=" ${dhcp} "
	[[ ${d} == *" nodns "* ]] && opts="${opts} --no-dns"
	[[ ${d} == *" nogateway "* ]] && opts="${opts} --no-gateway"
	[[ ${d} == *" nontp "* ]] && opts="${opts} --no-ntp"

	search="dns_search_${ifvar}"
	[[ -n ${!search} ]] && opts="${opts} --search-path='"${!search}"'"

	# Add our route metric
	metric="metric_${ifvar}"
	[[ -n ${!metric} ]] && opts="${opts} --route-metric ${!metric}"

	opts="${opts} --win-client-ident"
	opts="${opts} --keep-up --interface ${iface}"

	# Bring up DHCP for this interface (or alias)
	ebegin "Running pump"
	eval pump "${opts}"
	eend $? || return $?

	# pump succeeded, show address retrieved
	local addr=$(interface_get_address "${iface}")
	einfo "${iface} received address ${addr}"

	return 0
}

# vim: set ts=4 :
