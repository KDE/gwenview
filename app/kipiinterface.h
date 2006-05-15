// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef KIPIINTERFACE_H
#define KIPIINTERFACE_H	

#include <config.h>
#ifdef GV_HAVE_KIPI

#include <libkipi/interface.h>
namespace Gwenview {

class KIPIInterfacePrivate;

class FileViewController;

class KIPIInterface :public KIPI::Interface {
	Q_OBJECT

public:
	KIPIInterface( QWidget* parent, FileViewController*);
	virtual ~KIPIInterface();

	KIPI::ImageCollection currentAlbum();
	KIPI::ImageCollection currentSelection();
	QValueList<KIPI::ImageCollection> allAlbums();
	KIPI::ImageInfo info( const KURL& );
	int features() const;
	virtual bool addImage(const KURL&, QString& err);
	virtual void delImage( const KURL& );
	virtual void refreshImages( const KURL::List& urls );

private:
	KIPIInterfacePrivate* d;

private slots:
	void slotSelectionChanged();
	void slotDirectoryChanged();
	void init();
};

#endif /* GV_HAVE_KIPI */
} // namespace
#endif /* KIPIINTERFACE_H */

