#
# Regular cron jobs for the nr-deckbuilder package
#
0 4	* * *	root	[ -x /usr/bin/nr-deckbuilder_maintenance ] && /usr/bin/nr-deckbuilder_maintenance
