%define	desktop_vendor	rockers

Name: gwenview
Summary: Gwenview is an image viewer for KDE.
Version: 0.16.2
Release: 0
License: GPL
Group: Amusements/Graphics
Source: http://prdownloads.sourceforge.net/gwenview/%{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-root
Url: http://gwenview.sourceforge.net/home/
Requires: libpng, kdebase >= 3.0
BuildRequires: libpng-devel, kdelibs-devel, arts-devel, libjpeg-devel
BuildRequires:  XFree86-devel, zlib-devel, qt-devel >= 3.0.2

%description
Gwenview is an image viewer for KDE.

It features a folder tree window and a file list window to 
provide easy navigation among your thousand images.

Image loading is done by the Qt library, giving you access 
to all image formats your Qt installation supports, but 
Gwenview can also browse GIMP files (*.xcf) thanks to the 
included QXCFI component developed by Lignum Computing.

Gwenview correctly displays images with alpha channel, 
using the traditionnal checker board as a background to
reveal transparency.

%prep
rm -rf %{buildroot}

%setup -q

%build
%configure --with-xinerama
make %{_smp_mflags}

%install
%makeinstall

desktop-file-install --vendor %{desktop_vendor} --delete-original \
  --dir %{buildroot}%{_datadir}/applications                      \
  --add-category X-Red-Hat-Extra                                  \
  --add-category Application                                      \
  --add-category Graphics					  \
  %{buildroot}%{_datadir}/applnk/Graphics/%{name}.desktop

rm -rf %{buildroot}%{_datadir}/applnk

mv %{buildroot}%{_datadir}/hicolor/16x16/actions/	\
%{buildroot}%{_datadir}/icons/hicolor/16x16

mkdir %{buildroot}%{_datadir}/icons/hicolor/22x22
mv %{buildroot}%{_datadir}/hicolor/22x22/actions/	\
%{buildroot}%{_datadir}/icons/hicolor/22x22

mv %{buildroot}%{_datadir}/hicolor/32x32/actions/	\
%{buildroot}%{_datadir}/icons/hicolor/32x32

rm -rf %{buildroot}%{_datadir}/hicolor

%clean
rm -rf %{buildroot}

%post

%postun

%files
%defattr(-,root,root,-)
%doc AUTHORS COPYING README INSTALL CREDITS TODO NEWS
%{_bindir}/%{name}*
%{_mandir}/man1/%{name}*
%{_datadir}/apps/konqueror/servicemenus/konqgwenview.desktop
%{_datadir}/applications/%{desktop_vendor}-%{name}.desktop
%{_datadir}/icons/*/*/*/*
%{_datadir}/locale/*/LC_MESSAGES/%{name}*

%changelog
* Fri Feb 14 2003 Robert Rockers <brockers at dps.state.ok.us> 0.16.2
- Recompiled for version 0.16.2

* Fri Feb 7 2003 Robert Rockers <brockers at dps.state.ok.us> 0.16.1
- Initial RedHat RPM release.
