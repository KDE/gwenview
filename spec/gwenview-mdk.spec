%define name gwenview
%define version 1.0.0
%define release 0.pre3.2mdk

Summary: Simple image viewer for KDE.
Name: %name
Version: %version
Release: %release
License: GPL
Group: Graphics
Source0: %{name}-%{version}pre3.tar.bz2
URL: http://gwenview.sourceforge.net 
BuildRoot: %_tmppath/%{name}-%{version}

%description
Gwenview is an image viewer for KDE. 

It features a folder tree window and a file list window to provide easy 
navigation in your file hierarchy.  Image loading is done by the Qt library, 
so it supports all image formats your Qt installation supports. 

%prep
%setup -q -n %{name}-%{version}pre3

%build
./configure --disable-rpath \
            --prefix=$RPM_BUILD_ROOT/%_prefix \
	    --libdir=$RPM_BUILD_ROOT%_libdir \
	    --mandir=$RPM_BUILD_ROOT%_mandir \
	    --datadir=$RPM_BUILD_ROOT%_datadir \
	    --enable-final

%make

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall 

install -d %buildroot/%_menudir/
kdedesktop2mdkmenu.pl %{name} "Multimedia/Graphics" %buildroot/%_datadir/applications/kde/%{name}.desktop %buildroot/%_menudir/%{name}

%find_lang %name

%post
%update_menus

%postun
%clean_menus

%files -f %name.lang
%defattr(-,root,root,0755)
%doc NEWS README TODO ChangeLog COPYING CREDITS
%_bindir/%{name}
%_menudir/*
%_datadir/apps/konqueror/servicemenus/*
%dir %_datadir/apps/%{name}/
%_datadir/apps/%{name}/*
%_datadir/icons/locolor/16x16/apps/*
%_datadir/icons/locolor/32x32/apps/*
%_datadir/icons/hicolor/16x16/apps/*
%_datadir/icons/hicolor/32x32/apps/*
%_datadir/icons/hicolor/48x48/apps/*
%_datadir/applications/kde/%{name}.desktop
%_mandir/man1/gwenview.1.bz2

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Wed Nov 05 2003 Marcel Pol <mpol@gmx.net> 1.0.0-0.pre3.2mdk
- redo changelog
- rm -rf $RPM_BUILD_ROOT in %%install instead of %%prep

* Tue Nov 04 2003 Angelo Naselli <random_lx@yahoo.com> 1.0.0-0.pre3.1mdk
- built mdk version
      - New features from Aurélien Gâteau:
       - Added a "don't ask me again" check box to the save prompt dialog.
       - Added a reload button.
       - Added a "Go" button to the location toolbar.
      - Fixes:
        - Really fixed saving of external tools.
	- Make sure the folder view is updated when a folder is renamed.
	- The mouse-wheel behaviors are not messed anymore by dialogs or by 
	  showing the popup menu.

* Mon Nov 03 2003 Marcel Pol <mpol@gmx.net> 1.0.0-0.pre2.2mdk
- buildrequires
- quiet setup

* Wed Oct 22 2003 Angelo Naselli <random_lx@yahoo.com> 1.0.0-0.pre2.1mdk
- added some changes on spec file imported from 
  Lenny Cartier <lenny@mandrakesoft.com> 1.0.0-0.pre1.1mdk

* Mon Oct 20 2003 Lenny Cartier <lenny@mandrakesoft.com> 1.0.0-0.pre1.1mdk
- from Angelo Naselli <random_lx@yahoo.com> :
       - built mdk version

* Mon Oct 12 2003 Angelo Naselli <random_lx@yahoo.com> 1.0.0pre2-1mdk
- fixed some bugs on spec file
- built mdk version

* Mon Oct 6 2003 Angelo Naselli <random_lx@yahoo.com> 1.0.0pre1-2mdk
- Added print button patch

* Mon Sep 29 2003 Angelo Naselli <random_lx@yahoo.com> 1.0.0pre1-1mdk
- built mdk version

* Fri Aug 08 2003 Angelo N. <random_lx@yahoo.com>  0.17.1a-1mdk
- built mdk version

