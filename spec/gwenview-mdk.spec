Summary:Simple image viewer for KDE
Name: gwenview
Version: 0.15.0
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
# Disabled, the documentation needs to be converted to docbook
#%doc NEWS README TODO ChangeLog COPYING CREDITS

%clean
rm -rf $RPM_BUILD_ROOT
