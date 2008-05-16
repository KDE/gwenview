// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE

Copyright  2008      Angelo Naselli

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

// Qt includes.

// #include <QHeaderView>
// // #include <QTreeWidget>
// #include <QHBoxLayout>

// KDE includes.

// #include <klocale.h>
// #include <kdialog.h>
#include <kurlrequester.h>
#include <kdirlister.h>

// Local includes.
#include "kipiinterface.h"
#include "kipiuploadwidget.h"
#include "kipiuploadwidget.moc"

namespace Gwenview {

class KipiUploadWidgetPriv
{
public:
	KipiUploadWidgetPriv()
	{
		pDirView       = 0;
		pKipiInterface = 0;
	}

	KUrlRequester *pDirView;
	KIPIInterface *pKipiInterface;
};

KipiUploadWidget::KipiUploadWidget(KIPIInterface* interface, QWidget *parent)
                : KIPI::UploadWidget(parent)
{
	d = new KipiUploadWidgetPriv();
	d->pKipiInterface = interface;

	// Fetch the current album, so we can start out there.
	KIPI::ImageCollection album = interface->currentAlbum();

	// If no current album selected, get the first album in the list.
	if ( !album.isValid() || !album.isDirectory() )
		album = interface->allAlbums().first();

	d->pDirView = new KUrlRequester(album.uploadRoot().path(),this);

	// ------------------------------------------------------------------------------------

	connect(d->pDirView, SIGNAL(urlSelected (const KUrl &)),
			this, SLOT(selectUrl(const KUrl &)));
//             this, SIGNAL(selectionChanged()));

}

KipiUploadWidget::~KipiUploadWidget() 
{
	delete d;
}

KIPI::ImageCollection KipiUploadWidget::selectedImageCollection() const
{
	KUrl url = d->pDirView->url();
	KDirLister dl;
	dl.openUrl(url);
	KFileItemList fileList = dl.itemsForDir(url);
	KUrl::List list = fileList.urlList();

	return KIPI::ImageCollection(new ImageCollection(url, url.fileName(), list));
}



// SLOT
void KipiUploadWidget::selectUrl(const KUrl &)
{
  //TODO

  emit selectionChanged();
}

}// namespace
