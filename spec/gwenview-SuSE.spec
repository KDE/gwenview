Summary:Simple image viewer for KDE
Name: gwenview
Version: 0.15.1
Release: 1
Copyright: GPL
Group: Application/Multimedia 
Source0: %{name}-%{version}.tar.bz2
URL: http://gwenview.sourceforge.net 

Packager: Dario Abatianni <eisfuchs@tigress.com>

BuildRoot: /tmp/build/%{name}-%{version}

%description
Gwenview is an image viewer for KDE 3.x.

It features a folder tree window and a file list window to provide easy 
navigation in your file hierarchy.  Image loading is done by the Qt library, 
so it supports all image formats your Qt installation supports. 

%prep
rm -rf $RPM_BUILD_ROOT

%setup

%build
./configure --prefix=$RPM_BUILD_ROOT/opt/kde3 --enable-final
make

%install
make install 

%files
%defattr(-,root,root,0755)
/opt/kde3/bin/gwenview
/opt/kde3/share/apps/gwenview/icons/*/*/actions/*.png
/opt/kde3/share/icons/*/*/apps/gwenview.png
/opt/kde3/share/applnk/Graphics/gwenview.desktop
/opt/kde3/share/apps/konqueror/servicemenus/konqgwenview.desktop
/opt/kde3/share/locale/*/LC_MESSAGES/gwenview.mo
/opt/kde3/man/man1/gwenview.1
%doc NEWS README TODO ChangeLog COPYING CREDITS

%clean
rm -rf $RPM_BUILD_ROOT
