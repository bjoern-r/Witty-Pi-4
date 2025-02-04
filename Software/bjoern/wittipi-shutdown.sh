#!/bin/sh

## "poweroff", "halt", "reboot", or "kexec",

echo "wittyPi shutdown handler mode: $1"

case $1 in
	poweroff|halt )
		echo "preparing wittypi for poweroff"
		echo /usr/sbin/i2cset -y 0 8 14 4
		;;
	reboot )
		## enable watchdog for reboot?
		# i2cset -y 0 8 14 8
		;;
	kexec )
		## enable watchdog for reboot?
		# i2cset -y 0 8 14 8
		;;
	*)
		echo "unknown mode: $1 "
        ;;
esac
