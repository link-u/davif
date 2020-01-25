#
# Regular cron jobs for the davif package
#
0 4	* * *	root	[ -x /usr/bin/davif_maintenance ] && /usr/bin/davif_maintenance
