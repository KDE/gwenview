%define gvname gwenview
%define version 1.2.92
## Distribution Specific Release Tag
%{?!mkrel: %define mkrel(c:) %{-c:0.%{-c*}.}%{!?_with_unstable:%(perl -e '$_="%{1}";m/(.\*)(\\d+)$/;$rel=${2}-1;re;print "$1$rel";').%{?subrel:%subrel}%{!?subrel:1}.%{?distversion:%distversion}%{?!distversion:%(echo $[%{mdkversion}/10])}}%{?_with_unstable:%{1}}%{?distsuffix:%distsuffix}%{?!distsuffix:mdk}}
%define release %mkrel 1

%define gvcorename libgwenviewcore
%define gvdirpartname gvdirpart
%define gvimagepartname gvimagepart

%define name %gvname

%define major 1
%define libname %mklibname %{gvname} %major
%define libnamedev %mklibname %{gvname} %major -d

# building kipi version
%define build_kipi 1
%{?_with_nokipi: %global build_kipi 0}
%if %build_kipi
%define kipiopt --enable-kipi
%define kipireq  libkipi-devel
%endif


Summary: Fast and easy to use image viewer for KDE
Name: %name
Version: %version
Release: %release
License: GPL
Group: Graphics
Source0: http://prdownloads.sourceforge.net/gwenview/%{gvname}-%{version}.tar.bz2

URL: http://gwenview.sourceforge.net 
BuildRoot: %_tmppath/%{name}-%{version}
#  added automake1.7 requirement to patch Makefile(s).am
%if %build_kipi
BuildRequires: kdelibs-devel automake1.7 X11-devel %{kipireq}
%else
BuildRequires: kdelibs-devel automake1.7 X11-devel
%endif


%description
Gwenview is a fast and easy to use image viewer/browser for KDE.
All common image formats are supported, such as PNG(including transparency), 
JPEG(including EXIF tags and lossless transformations), GIF, XCF (Gimp 
image format), BMP, XPM and others. Standard features include slideshow, 
fullscreen view, image thumbnails, drag'n'drop, image zoom, full network 
transparency using the KIO framework, including basic file operations and 
browsing in compressed archives, non-blocking GUI with adjustable views. 
Gwenview also provides image and directory KParts components for use e.g. in 
Konqueror. Additional features, such as image renaming, comparing, 
converting, and batch processing, HTML gallery and others are provided by the 
KIPI image framework.

%package -n %libname
Summary: Libraries for gwenview image viewer
Group: System/Libraries
Requires:  %{name} = %{version}

%description -n %libname
Gwenview is a fast and easy to use image viewer/browser for KDE.
%{libname} contains the libraries needed to use %{gvname}

%package -n %libnamedev
Summary: Devel files (gwenview image viewer)
Group: Development/Other
Requires: %libname = %{version}
Provides: lib%{gvname}-devel = %{version}-%{release}
Provides: %{gvname}-devel = %{version}-%{release}

%description -n %libnamedev
Gwenview is a fast and easy to use image viewer/browser for KDE.
%{libnamedev} contains the libraries and header files needed to
develop programs which make use of %{libname}.

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{gvname}-%{version}

%build
make -f admin/Makefile.common cvs

export QTDIR=%_prefix/%_lib/qt3
export KDEDIR=%_prefix

export LD_LIBRARY_PATH=$QTDIR/%_lib:$KDEDIR/%_lib:$LD_LIBRARY_PATH
export PATH=$QTDIR/bin:$KDEDIR/bin:$PATH

# Search for qt/kde libraries in the right directories (avoid patch)
# NOTE: please don't regenerate configure scripts below
perl -pi -e "s@/lib(\"|\b[^/])@/%_lib\1@g if /(kde|qt)_(libdirs|libraries)=/" configure

./configure --disable-rpath \
%if %build_kipi
            %kipiopt \
%else
             \
%endif
            --prefix=%_prefix \
            --libdir=%_libdir \
            --mandir=%_mandir \
            --datadir=%_datadir 


%make 

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p %buildroot/%_datadir/applnk/Multimedia/Graphics
%makeinstall 

install -d %buildroot/%_menudir/
kdedesktop2mdkmenu.pl %{gvname} "Multimedia/Graphics" %buildroot/%_datadir/applications/kde/%{gvname}.desktop %buildroot/%_menudir/%{gvname}

#icons for rpmlint
mkdir -p %buildroot/{%_liconsdir,%_miconsdir,%_iconsdir}
ln -s %_datadir/icons/hicolor/64x64/apps/%{gvname}.png %buildroot/%_liconsdir
ln -s %_datadir/icons/hicolor/32x32/apps/%{gvname}.png %buildroot/%_iconsdir
ln -s %_datadir/icons/hicolor/16x16/apps/%{gvname}.png %buildroot/%_miconsdir

%find_lang %{gvname}

%post
%update_menus

%postun
%clean_menus

%post -n %libname -p /sbin/ldconfig

%postun -n %libname -p /sbin/ldconfig

%files -f %{gvname}.lang
%defattr(-,root,root,0755)
%doc NEWS AUTHORS README TODO ChangeLog COPYING INSTALL
%_bindir/%{gvname}

%dir %_datadir/apps/%{gvdirpartname}/
%_datadir/apps/%{gvdirpartname}/gvdirpart.rc

%dir %_datadir/apps/%{gvimagepartname}/
%_datadir/apps/%{gvimagepartname}/gvimagepart.rc
%_datadir/services/%{gvdirpartname}.desktop
%_datadir/services/%{gvimagepartname}.desktop
%_menudir/*
%_datadir/apps/konqueror/servicemenus/*
%_datadir/apps/kconf_update/%{gvname}*
%dir %_datadir/apps/%{gvname}/
%_datadir/apps/%{gvname}/*
%_datadir/icons/crystalsvg/16x16/apps/*
%_datadir/icons/crystalsvg/22x22/apps/*
%_datadir/icons/hicolor/*
%_liconsdir/%{gvname}.png
%_iconsdir/%{gvname}.png
%_miconsdir/%{gvname}.png
%_datadir/applications/kde/%{gvname}.desktop
%_datadir/config.kcfg/gvconfig.kcfg
%dir %_datadir/doc/HTML/
%_datadir/doc/HTML/*
%_mandir/man1/%{gvname}.1.bz2

%_libdir/kde3/*
%_libdir/libkdeinit_%{gvname}.so
%_libdir/libkdeinit_%{gvname}.la

%files -n %libname
%defattr(-,root,root,0755)
%_libdir/*.so.*

%files -n %libnamedev
%defattr(-,root,root,0755)
%_includedir/libgwenview_export.h
%_libdir/%{gvcorename}.so
%_libdir/%{gvcorename}.la

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Sun Aug 21 2005 Angelo Naselli <anaselli@mandriva.org> 1.2.92-1mdk
- New release 1.2.92

* Wed Jul 20 2005 Angelo Naselli <anaselli@mandriva.org> 1.2.91-1mdk
- New release 1.2.91
- patched for fvisibility problem (aligned to svn)

* Mon May 09 2005 Laurent MONTEL <lmontel@mandriva.com> 1.2.0-4
- Real fix build on x86_64

* Sun May 08 2005 Angelo Naselli <anaselli@mandriva.org> 1.2.0-3mdk
- fix for x86_64 arch

* Sun May 08 2005 Angelo Naselli <anaselli@mandriva.org> 1.2.0-2mdk
- Rebuild

* Sun Apr 03 2005 Angelo Naselli <anaselli@mandrake.org> 1.2.0-1mdk
- really built new version

* Sun Mar 20 2005 Angelo Naselli <anaselli@mandrake.org> 1.2.0-0.pre4.2mdk
- really built new version

* Sun Mar 20 2005 Angelo Naselli <anaselli@mandrake.org> 1.2.0-0.pre4.1mdk
- new version

* Sat Mar 19 2005 Angelo Naselli <anaselli@mandrake.org> 1.2.0-0.pre3.2mdk
- fix bug #14731 

* Sun Feb 27 2005 Angelo Naselli <anaselli@mandrake.org> 1.2.0-0.pre3.1mdk
- new version

* Sun Feb 13 2005 Angelo Naselli <anaselli@mandrake.org> 1.2.0-0.pre2.2mdk
- define mkrel macro if not exist

* Sun Feb 13 2005 Angelo Naselli <anaselli@mandrake.org> 1.2.0-0.pre2.1mdk
- new version

* Sat Jan 29 2005 Angelo Naselli <anaselli@mandrake.org> 1.2.0-0.pre1.2mdk
- added patch to make it compile for mdk official 10.1
- added patch to fix zoom (from cvs)
- added patch to add missing files (from cvs)

* Mon Jan 24 2005 Angelo Naselli <anaselli@mandrake.org> 1.2.0-0.pre1.1mdk
- new version

* Wed Jan 19 2005 Angelo Naselli <anaselli@mandrake.org> 1.1.8-0.4mdk
- fix bug 13100

* Sun Jan 16 2005 Angelo Naselli <anaselli@mandrake.org> 1.1.8-0.3mdk
- better handling of symlink

* Thu Jan 13 2005 Angelo Naselli <anaselli@mandrake.org> 1.1.8-0.2mdk
- fix double click into kpart

* Sun Jan 09 2005 Angelo Naselli <anaselli@mandrake.org> 1.1.8-0.1mdk
- 1.1.8 
- fix Requires section to be compliant to the library policy

* Thu Dec 30 2004 Angelo Naselli <anaselli@mandrake.org> 1.1.7-0.5mdk
- added 1.1.7b patch (solved some build problems)

* Wed Dec 29 2004 Angelo Naselli <anaselli@mandrake.org> 1.1.7-0.4mdk
- description restyling

* Sun Dec 26 2004 Angelo Naselli <anaselli@mandrake.org> 1.1.7-0.3mdk
- fix Require and Provide section

* Sun Dec 26 2004 Angelo Naselli <anaselli@mandrake.org> 1.1.7-0.2mdk
- removed hack management
- added distro-specific release tag management 
  use option "--with official" to build mdk official package

* Mon Dec 20 2004 Laurent MONTEL <lmontel@mandrakesoft.com> 1.1.7-0.1mdk
- 1.1.7

* Fri Dec 10 2004 Laurent MONTEL <lmontel@mandrakesoft.com> 1.1.6-0.2mdk
- Fix spec file

* Sun Oct 24 2004 Angelo Naselli <anaselli@mandrake.org> 1.1.6-0.1mdk
- new version
  *  New features:
   o The application now has two modes: browse and view. Browse mode shows 
     all views: folder, file and image. View mode only shows the image. 
     Gwenview starts in browse mode except if an image URL is given as 
     an argument. You can switch between modes using the toolbar button, 
     or with the "View/Browse mode" menu item or with the Ctrl+Return shortcut.
   o JPEGTran code has been integrated into Gwenview, there's no need to install 
     it separately anymore.
  * Fixes:
   o Update the EXIF thumbnail when rotating a JPEG file.
   o In the folder view, folders now open with a single click (By Daniel Thaler).
   o Reworked coordinate conversions in order to avoid subtle paint errors.
   o Remember computed optimal repaint sizes in the config file, 
     so they are available immediately after next start.
   o Remember shown URL after session restore.
* Sat Oct 16 2004 Angelo Naselli <anaselli@mandrake.org> 1.1.5-0.3mdk
- rebuilt for new liblipi + fixing
* Sat Oct 09 2004 Angelo Naselli <anaselli@mandrake.org> 1.1.5-0.2mdk
- applied Lubos Lunak's patch to avoid printing crash using Konqueror
* Mon Sep 20 2004 Angelo Naselli <anaselli@mandrake.org> 1.1.5-0.1mdk
- new version
   *  New features:
    o The thumbnail progress bar and stop buttons are now embedded in the thumbnail view.
    o The location bar now shows the file names instead of the folders.
    o The thumbnails toolbar buttons have been moved to a specialized file view toolbar.
    o It's now possible to assign key shortcuts to KIPI plugins.
    o New manpage by Christopher Martin.
   * Fixes:
    o Do not display the folder name as an image in the status bar.
    o Make sure the folder KPart starts in the right folder.
    o Unbreak the saving of key shortcuts.
    o Remote urls are correctly bookmarked.
    o Do not try to overwrite the trash when trashing only one file.

* Sun Aug 29 2004 Angelo Naselli <random_lx@yahoo.com> 1.1.4-0.2mdk
- patch for russian language

* Tue Aug 24 2004 Angelo Naselli <anaselli@mandrake.org> 1.1.4-0.1mdk
- changed spec file to manage -with-hack option to build gwenview with 
  hack suffix (default is without hack)
- from Aurélien Gâteau:
- New features:
 - In the thumbnail view, It's now possible to sort images in reverse order.
 - Use EXIF-stored thumbnail if available.
 - Option to disable saving of generated thumbnails to cache.
 - In fullscreen mode, it's now possible to display the image comment or size
   in addition to the file path.
 - The fullscreen On-Screen-Display is more readable now.
 - The background color of the image view can be configured.
 - When printing, it's now possible to enlarge images so that they fill the
   page.
- Fixes:
 - In the folder view, pressing Enter now opens the selected folder.
 - Use icon list for the configuration dialog.
 - Avoid data loss if the JPEG images are saved while being rotated by
   JPEGTran.
 - The back button in Konqueror now works correctly with gvimagepart.
 - The default layout is more user-friendly.
 - Non-trivial URLs (e.g. http query URL) are correctly handled.
 - You can now drop images on the image view.

* Sat Jun 12 2004 Angelo Naselli <random_lx@yahoo.com> 1.1.3-0.1mdk
- new release: my wedding present :-)
   from Aurélien Gâteau:
   Gwenview codenamed "Hurry up, I'm getting married tomorrow"
   *  New features:
          o You can now define custom branches in the dir view (By Craig Drummond)
          o An image cache has been added to speedup image loading.
          o Gwenview now uses freedesktop.org thumbnail spec to store thumbnails.
          o A new option to automatically empty thumbnail cache on exit (By Angelo Naselli).
          o The image size is now displayed below file names in thumbnail view.
    * Fixes:
          o Don't crash when switching to fullscreen while generating thumbnails and coming back (By Lubos Lunak)
          o Faster thumbnail generation (By Lubos Lunak)
          o Faster image painting by dynamically determining suitable paint size (By Lubos Lunak)
          o Use the "Standard Background" color as the background for thumbnails and folders (By Craig Drummond).
          o Make sure the current image is reloaded if it has been modified outside Gwenview.

* Tue Jun 1 2004 Angelo Naselli <random_lx@yahoo.com> 1.1.2-0.3mdk
- hack suffix on kpart lib

* Thu May 13 2004 Lenny Cartier <lenny@mandrakesoft.com> 1.1.2-0.2mdk
- merge with changes from Angelo Naselli <random_lx@yahoo.com>

* Wed May 12 2004 Lenny Cartier <lenny@mandrakesoft.com> 1.1.2-0.1mdk
- 1.1.2
- from Angelo Naselli <random_lx@yahoo.com> : 
- mdk version of the development release (1.1.1) named gwenview_hack
    from Aurélien Gâteau:
    - New features:
     - Added KPart support, this installs in Konqueror a new file view mode and let you view 
       images in an embedded Gwenview (By Jonathan Riddell).
     - Asynchronous JPEG loading, based on Khtml loader.
     - Really asynchronous PNG loading (By Lubos Lunak).
     - Mouse wheel will now scroll the image by default. Holding Ctrl will scroll horizontally. 
       An option has been added to the setting dialog to toggle between scroll and browse 
       (By Jeroen Peters).
     - When holding shift over the image, right click will zoom out (By Jeroen Peters).
     - Image painting is now progressive (By Lubos Lunak).
    - Fixes:
     - The rotate and mirror functions can now work on multiple selection.
     - Make it possible to load another image or quit even if you can't save your changes.
     - Gwenview won't spawn multiple instances of jpegtran anymore.

* Sun Feb 01 2004 Angelo Naselli <random_lx@yahoo.com> 1.1.0-0.1mdk
- mdk version of the first development release (1.1.0)
    from Aurélien Gâteau:
    - New features:
     - New settings in print dialog to specify how the image must be printed.
     - Big thumbnails are really BIG now.
     - First implementation of asynchronous image loading. Only for PNG right now.
    - Fixes:
     - The move and copy dialogs now use a tree view.
     - In the thumbnail view, create thumbnails for the visible images first 

* Sun Jan 11 2004 Angelo Naselli <random_lx@yahoo.com> 1.0.1-0.1mdk
- built mdk version of Gwenview 1.0.0 release
- fix icons for rpmlint
    from Aurélien Gâteau:
	- New features:
	 - Double-clicking an image in the file view will open it in fullscreen.
	- Fixes:
	 - Gave contributors the credit they deserve in the about box.
	 - Updated to libexif 0.5.12 and applied patch from libexif CVS.
	 - When going to the parent folder, make sure the folder we were in before is selected.
	 - If there's no image in the current folder, select the first visible file.
	 - When holding down Shift to zoom, keep the same area of the image under the cursor.
	 - Nicer drag cursor.
	 - Hopefully fixed every cases where the image was not centered in the view.

* Mon Dec 08 2003 Angelo Naselli <random_lx@yahoo.com> 1.0.0-0.1mdk
- built mdk version of Gwenview 1.0.0 release
    from Aurélien Gâteau:
    - New features:
          - Show a wait icon for not-generated-yet thumbnails (inspired from Nautilus 
            thumbnail view).
          - Show a broken icon for broken images.
    - Fixes:
          - If auto-zoom is on, make sure the zoom is updated after rotating an image.
          - Fixed crash when loading XCF images if Gwenview was compiled with gcc 3.3.1.
          - Before running an external tool, change working directory to current folder.
          - When switching images in fullscreen, don't show the cursor.
          - Use standard KDE icons for zoom actions.
          - New icons for slideshow and image operations.
          - New magnifier cursor.

* Sun Nov 16 2003 Angelo Naselli <random_lx@yahoo.com> 1.0.0-0.pre4.1mdk
- built mdk version
    from Aurélien Gâteau:
   - New features 
     - Added a new option to hide the busy pointer when loading an 
       image in fullscreen.
     - Added a popup menu to select the sorting mode.
       Usefull in thumbnail view.
   - Fixes:
     - Use a KDE dialog for the configuration dialog.
     - Removed the image view mouse behavior configuration 
       options. The behavior is much simpler now: left button 
       to drag image,
     - middle button to toggle auto-zoom and mouse-wheel 
       to browse images. Ifrom Aurélien Gâteauef you want to zoom hold Shift
       and use either the mouse-wheel or the left button.

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

