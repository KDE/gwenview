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
#include <kconfig.h>
#include <kdebug.h>
#include <kfilemetainfo.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kparts/browserextension.h>
#include <kparts/genericfactory.h>

#include "gvimagepart.h"
#include <src/gvcache.h>
#include <src/gvdocument.h>
#include <src/gvprintdialog.h>
#include <src/gvscrollpixmapview.h>

#include "config.h"

// For now let's duplicate
const char CONFIG_VIEW_GROUP[]="GwenviewPart View";
const char CONFIG_JPEGTRAN_GROUP[]="jpegtran";
const char CONFIG_CACHE_GROUP[]="cache";


class GVImagePartView : public GVScrollPixmapView {
public:
	GVImagePartView(QWidget* parent, GVDocument* document, KActionCollection* actionCollection, GVImagePartBrowserExtension* browserExtension)
	: GVScrollPixmapView(parent, document, actionCollection), mBrowserExtension(browserExtension)
	{}

protected:
	void openContextMenu(const QPoint&) {
		mBrowserExtension->contextMenu();
	}

private:
	GVImagePartBrowserExtension* mBrowserExtension;
};


//Factory Code
typedef KParts::GenericFactory<GVImagePart> GVImageFactory;
K_EXPORT_COMPONENT_FACTORY( libgvimagepart /*library name*/, GVImageFactory )

GVImagePart::GVImagePart(QWidget* parentWidget, const char* /*widgetName*/, QObject* parent,
			 const char* name, const QStringList &) : KParts::ReadOnlyPart( parent, name )  {
	setInstance( GVImageFactory::instance() );
	KGlobal::locale()->insertCatalogue( "gwenview" );

	mBrowserExtension = new GVImagePartBrowserExtension(this);

	// Create the widgets
	mDocument = new GVDocument(this);
	connect( mDocument, SIGNAL( loaded(const KURL&)), SLOT( loaded(const KURL&)));
	mPixmapView = new GVImagePartView(parentWidget, mDocument, actionCollection(), mBrowserExtension);
	setWidget(mPixmapView);

	KIconLoader iconLoader = KIconLoader("gwenview");
	iconLoader.loadIconSet("rotate_right", KIcon::Toolbar);
	KStdAction::saveAs( mDocument, SLOT(saveAs()), actionCollection(), "saveAs" );
	new KAction(i18n("Rotate &Right"), "rotate_cw", CTRL + Key_R, this, SLOT(rotateRight()), actionCollection(), "rotate_right");

	setXMLFile( "gvimagepart/gvimagepart.rc" );
}

GVImagePart::~GVImagePart() {
}


void GVImagePart::partActivateEvent(KParts::PartActivateEvent* event) {
#ifdef GV_HACK_SUFFIX
	KConfig* config=new KConfig("gwenview_hackrc");
#else
	KConfig* config=new KConfig("gwenviewrc");
#endif
	if (event->activated()) {
		GVCache::instance()->readConfig(config,CONFIG_CACHE_GROUP);
		mPixmapView->readConfig(config, CONFIG_VIEW_GROUP);
	} else {
		mPixmapView->writeConfig(config, CONFIG_VIEW_GROUP);
	}
	delete config;
	KParts::ReadOnlyPart::partActivateEvent( event );
}


KAboutData* GVImagePart::createAboutData() {
	KAboutData* aboutData = new KAboutData( "gvimagepart", I18N_NOOP("GVImagePart"),
						"0.1", I18N_NOOP("Image Viewer"),
						KAboutData::License_GPL,
						"(c) 2004, Jonathan Riddell <jr@jriddell.org>");
	return aboutData;
}

bool GVImagePart::openURL(const KURL& url) {
	if (!url.isValid())  {
		return false;
	}
	m_url = url;
	emit started( 0 );
	if( mDocument->url() == url ) { // reload button in Konqy - setURL would return immediately
		mDocument->reload();
	} else {
		mDocument->setURL(url);
	}
	emit setWindowCaption(url.prettyURL());
	return true;
}

QString GVImagePart::filePath() {
	return m_file;
}

void GVImagePart::loaded(const KURL& url) {
	QString caption = QString("%1 - %2x%3").arg(url.filename()).arg(mDocument->width()).arg(mDocument->height());
	emit setWindowCaption(caption);
	emit completed();
	emit setStatusBarText(i18n("Done."));
}

void GVImagePart::print() {
	KPrinter printer;

	printer.setDocName( m_url.filename() );
	KPrinter::addDialogPage( new GVPrintDialogPage( mDocument, mPixmapView, "GV page"));

	if (printer.setup(mPixmapView, QString::null, true)) {
		mDocument->print(&printer);
	}
}

void GVImagePart::rotateRight() {
	mDocument->transform(GVImageUtils::ROT_90);
}

/***** GVImagePartBrowserExtension *****/

GVImagePartBrowserExtension::GVImagePartBrowserExtension(GVImagePart* viewPart, const char* name)
	:KParts::BrowserExtension(viewPart, name) {
	mGVImagePart = viewPart;
	emit enableAction("print", true );
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

void GVImagePartBrowserExtension::print() {
	mGVImagePart->print();
}

#include "gvimagepart.moc"
