/*
Copyright 2004 Jonathan Riddell <jr@jriddell.org>

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
#include <qcursor.h>
#include <qpoint.h>

#include <kaction.h>
#include <kdebug.h>
#include <kfilemetainfo.h>
#include <klocale.h>
#include <kparts/browserextension.h>
#include <kparts/genericfactory.h>

#include "gvimagepart.h"
#include <src/gvscrollpixmapview.h>
#include <src/gvpixmap.h>

//Factory Code
typedef KParts::GenericFactory<GVImagePart> GVImageFactory;
K_EXPORT_COMPONENT_FACTORY( libgvimagepart /*library name*/, GVImageFactory );

GVImagePart::GVImagePart(QWidget* parentWidget, const char* /*widgetName*/, QObject* parent,
			 const char* name, const QStringList &) : KParts::ReadOnlyPart( parent, name )  {
	kdDebug() << k_funcinfo << endl;

	setInstance( GVImageFactory::instance() );

	mBrowserExtension = new GVImagePartBrowserExtension(this);

	// Create the widgets
	mGVPixmap = new GVPixmap(this);
	mPixmapView = new GVScrollPixmapView(parentWidget, mGVPixmap, actionCollection());
	mPixmapView->kpartConfig();
	setWidget(mPixmapView);

	connect(mPixmapView, SIGNAL(contextMenu()),
		mBrowserExtension, SLOT(contextMenu()) );
	connect(mGVPixmap, SIGNAL(loaded(const KURL&, const QString&)),
		this, SLOT(setKonquerorWindowCaption(const KURL&, const QString&)) );

	setXMLFile( "gvimagepart/gvimagepart.rc" );
}

GVImagePart::~GVImagePart() {
}

KAboutData* GVImagePart::createAboutData() {
	KAboutData* aboutData = new KAboutData( "gvdirpart", I18N_NOOP("GVDirPart"),
						"0.1", I18N_NOOP("Image Viewer"),
						KAboutData::License_GPL,
						"(c) 2004, Jonathan Riddell <jr@jriddell.org>");
	return aboutData;
}

bool GVImagePart::openFile() {
	//m_file is inherited from super-class
	//it is a QString with the path of the file
	//remote files are first downloaded and saved in /tmp/kde-user/
	KURL url(m_file);

	if (!url.isValid())  {
		return false;
	}
	if (!url.isLocalFile())  {
		return false;
	}

	mGVPixmap->setURL(url);
	return true;
}

QString GVImagePart::filePath() {
	return m_file;
}

void GVImagePart::setKonquerorWindowCaption(const KURL& /*url*/, const QString& /*filename*/) {
	QString caption = QString(m_url.filename() + " %1 x %2").arg(mGVPixmap->width()).arg(mGVPixmap->height());
	emit setWindowCaption(caption);
}

/***** GVImagePartBrowserExtension *****/

GVImagePartBrowserExtension::GVImagePartBrowserExtension(GVImagePart* viewPart, const char* name)
	:KParts::BrowserExtension(viewPart, name) {
	mGVImagePart = viewPart;
}

GVImagePartBrowserExtension::~GVImagePartBrowserExtension() {
}

void GVImagePartBrowserExtension::contextMenu() {
	/*FIXME Why is this KFileMetaInfo invalid?
	KFileMetaInfo metaInfo = KFileMetaInfo(mGVImagePart->filePath());
	kdDebug() << k_funcinfo << "mGVImagePart->filePath(): " << mGVImagePart->filePath() << endl;
	kdDebug() << k_funcinfo << "metaInfo.isValid(): " << metaInfo.isValid() << endl;
	kdDebug() << k_funcinfo << "above" << endl;
	QString mimeType = metaInfo.mimeType();
	kdDebug() << k_funcinfo << "below" << endl;
	emit popupMenu(QCursor::pos(), mGVImagePart->url(), mimeType);
	*/
	emit popupMenu(QCursor::pos(), mGVImagePart->url(), 0);
}

#include "gvimagepart.moc"
