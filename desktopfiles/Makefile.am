# this is where the kdelnk file will go
if GV_HACK_SUFFIX
xdg_apps_DATA = gwenview_hack.desktop
else
xdg_apps_DATA = gwenview.desktop
endif

# file for konqueror integration
konqservicedir = $(kde_datadir)/konqueror/servicemenus
if GV_HACK_SUFFIX
konqservice_DATA = konqgwenview_hack.desktop
else
konqservice_DATA = konqgwenview.desktop
endif


EXTRA_DIST = $(xdg_apps_DATA) $(konqservice_DATA)
