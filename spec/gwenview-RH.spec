Summary:Simple image viewer for KDE
Name: gwenview
Version: 0.12.0
Release: 1_RedHat
Copyright: GPL
Group: Application/Multimedia 
Source0: %{name}-%{version}.tar.bz2
URL: http://gwenview.sourceforge.net 

Packager: Ian Koenig <iguy@ionsphere.org> 

BuildRoot: /tmp/build/%{name}-%{version}

%description
Gwenview is an image viewer for KDE 2.x. 

It features a folder tree window and a file list window to provide easy 
navigation in your file hierarchy.  Image loading is done by the Qt library, 
so it supports all image formats your Qt installation supports. 

%prep
rm -rf $RPM_BUILD_ROOT

%setup

%build
./configure --prefix=$RPM_BUILD_ROOT/usr/ --enable-final 
make

%install
make install 

%files
%defattr(-,root,root,0755)
/usr/bin/gwenview
/usr/share/apps/gwenview
/usr/share/apps/gwenview/icons/hicolor/22x22/actions/actualsize.png
/usr/share/apps/gwenview/icons/hicolor/22x22/actions/smallthumbnails.png
/usr/share/apps/gwenview/icons/hicolor/22x22/actions/medthumbnails.png
/usr/share/apps/gwenview/icons/hicolor/22x22/actions/largethumbnails.png
/usr/share/apps/gwenview/icons/hicolor/32x32/actions/actualsize.png
/usr/share/apps/gwenview/icons/hicolor/32x32/actions/smallthumbnails.png
/usr/share/apps/gwenview/icons/hicolor/32x32/actions/medthumbnails.png
/usr/share/apps/gwenview/icons/hicolor/32x32/actions/largethumbnails.png
/usr/share/apps/gwenview/icons/locolor/16x16/actions/actualsize.png
/usr/share/apps/gwenview/icons/locolor/16x16/actions/smallthumbnails.png
/usr/share/apps/gwenview/icons/locolor/16x16/actions/medthumbnails.png
/usr/share/apps/gwenview/icons/locolor/16x16/actions/largethumbnails.png
/usr/share/icons/hicolor/16x16/apps/gwenview.png
/usr/share/icons/hicolor/32x32/apps/gwenview.png
/usr/share/icons/locolor/16x16/apps/gwenview.png
/usr/share/icons/locolor/32x32/apps/gwenview.png
/usr/share/applnk/Graphics/gwenview.desktop
/usr/share/apps/konqueror/servicemenus/konqgwenview.desktop
/usr/share/locale/*/LC_MESSAGES/gwenview.mo
/usr/man/man1/gwenview.1.gz
%doc NEWS README TODO ChangeLog COPYING CREDITS

%clean
rm -rf $RPM_BUILD_ROOT
