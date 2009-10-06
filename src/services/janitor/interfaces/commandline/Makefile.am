# moves (janitor, rjanitor) to share/arc/perl/interfaces/commandline/interfaces/commandline/
# create softlink (sbin/janitor) => .../interfaces/commandline/rjanitor
# set suid bit for (rjanitor)
# create man page (janitor.8)

# Location discussed in WP 5 meeting. The sysAdmin will need this tool => ../sbin/
sbindir=$(exec_prefix)/sbin

# The interfaces are stored along with the janitor module!
data_janitorcommandlinedir = $(pkgdatadir)/perl/Janitor/interfaces/commandline
dist_data_janitorcommandline_DATA = janitor

# Wrapper executable (SUID root bit will be set later) for more privileged rights
data_janitorcommandline_PROGRAMS = rjanitor
rjanitor_SOURCES  = rjanitor.c

# Man pages needs to be proccessed by configure in order to have the latest ARC version.
EXTRA_DIST=janitor.8.in
man_MANS = janitor.8


install-data-hook:
#	The suid bit will set the permissions to “-rwsr-xr-x”, or similar 
#	(note the “s” replacing the usual “x”). That means that you can run 
#	certain commands with the permissions of a different user (typically root).
	chmod 744 $(DESTDIR)/$(data_janitorcommandlinedir)/janitor
	chmod u+s $(DESTDIR)/$(data_janitorcommandlinedir)/rjanitor
	[ -d $(DESTDIR)/$(sbindir) ] || mkdir -p $(DESTDIR)/$(sbindir)
	ln -sf $(data_janitorcommandlinedir)/rjanitor $(DESTDIR)/$(sbindir)/janitor
	[ -d $(DESTDIR)/$(pkglibexecdir) ] || mkdir -p $(DESTDIR)/$(pkglibexecdir)
	ln -sf $(data_janitorcommandlinedir)/rjanitor $(DESTDIR)/$(pkglibexecdir)/janitor

uninstall-local: 
	if [ -f $(DESTDIR)/$(data_janitorcommandlinedir)/rjanitor ] ; then \
	rm -rf $(DESTDIR)/$(sbindir)/janitor; \
	fi


# Target is processed before install is processed => not useful for my purposes
#install-exec-hook:
#	if [ -f $(data_janitorcommandlinedir)/rjanitor ] ; then \
#	touch install_data_hook_EXIST; \
#	else \
#	touch install_data_hook_NOTEXIST; \
#	fi

#  Overloads the install function => not useful for my purposes
#install::
