# 
# We can't use the KDE_ICON system here, because am_edit does not handle the
# "if GV_HACK_SUFFIX", so we resort to manually installing the files.
#
# If you know a better way to do this, please patch that hack :-)
# 
if GV_HACK_SUFFIX
gv_icon_name = gwenview_hack
else
gv_icon_name = gwenview
endif
gv_icon_sizes = 16 22 32 48 64 128

install-data-am: install-gv-icons
uninstall-am: uninstall-gv-icons

install-gv-icons:
	for size in $(gv_icon_sizes) ; do \
		size2=$${size}x$${size} ; \
		$(mkinstalldirs) $(DESTDIR)$(kde_icondir)/crystalsvg/$$size2/apps ; \
		$(INSTALL_DATA) $(srcdir)/cr$$size-app-$(gv_icon_name).png $(DESTDIR)$(kde_icondir)/crystalsvg/$$size2/apps/$(gv_icon_name).png ; \
	done

uninstall-gv-icons:
	-for size in $(gv_icon_sizes) ; do \
		size2=$${size}x$${size} ; \
		rm -f $(DESTDIR)$(kde_icondir)/crystalsvg/$$size2/apps/$(gv_icon_name).png ; \
	done

