Summary:Simple image viewer for KDE
Name: gwenview
Version: 0.16.0
Release: 1mdk
Copyright: GPL
Group: Application/Multimedia 
Source0: %{name}-%{version}.tar.bz2
URL: http://gwenview.sourceforge.net 
Packager: Aurélien Gâteau <aurelien.gateau@mail.dotcom.fr> 
BuildRoot: /tmp/%{name}-%{version}

%description
Gwenview is an image viewer for KDE. 

It features a folder tree window and a file list window to provide easy 
navigation in your file hierarchy.  Image loading is done by the Qt library, 
so it supports all image formats your Qt installation supports. 

%prep
rm -rf $RPM_BUILD_ROOT

%setup

%build
./configure --prefix=$RPM_BUILD_ROOT/usr --enable-final 
make

%install
make install 

%files
%defattr(-,root,root,0755)
/usr/bin/gwenview
/usr/share/apps/gwenview/icons/*/*/actions/*.png
/usr/share/icons/*/*/apps/gwenview.png
/usr/share/applnk/Graphics/gwenview.desktop
/usr/share/apps/konqueror/servicemenus/konqgwenview.desktop
/usr/share/locale/*/LC_MESSAGES/gwenview.mo
%doc NEWS README TODO ChangeLog COPYING CREDITS

%clean
rm -rf $RPM_BUILD_ROOT
