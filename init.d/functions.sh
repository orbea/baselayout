SVCDIR=/var/lib/supervise

getcols() {
	echo $2
}

COLS=`stty size`
COLS=`getcols $COLS`
COLS=$(( $COLS - 7 ))
ENDCOL=$'\e[A\e['$COLS'G'
#now, ${ENDCOL} will move us to the end of the column; irregardless of character width

NORMAL="\033[0m"
GOOD=$'\e[32;01m'
BAD=$'\e[31;01m'
NORMAL=$'\e[0m'
HILITE=$'\e[36;01m'

ebegin() {
    echo -e " ${GOOD}*${NORMAL} ${*}..."
}

eerror() {
    echo -e ">>$BAD ${*}$NORMAL"
}

einfo() {
    echo -e "$HILITE${*}$NORMAL"
}

einfon() {
    echo -ne "$HILITE${*}$NORMAL"
}

eend() {
    if [ $# -eq 0 ] || [ $1 -eq 0 ] 
    then
        echo -e "$ENDCOL  \e[34;01m[ ${GOOD}ok \e[34;01m]${NORMAL}"
    else
		local returnme
		returnme=$1
		if [ $# -ge 2 ]
		then
	     	shift
			eerror $*
		fi
        echo -e "$ENDCOL  \e[34;01m[ ${BAD}!! \e[34;01m]${NORMAL}"
    	echo
		#extra spacing makes it easier to read
		return $returnme
    fi
}
