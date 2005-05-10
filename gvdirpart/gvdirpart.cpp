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
#include <qfile.h>
#include <qsplitter.h>
#include <qvaluelist.h>

#include <kdebug.h>
#include <kdeversion.h>
#include <kaction.h>
#include <kicontheme.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kparts/browserextension.h>
#include <kparts/genericfactory.h>
#include <kio/job.h>

#include "gvdirpart.h"
#include <src/fileoperation.h>
#include <src/gvarchive.h>
#include <src/gvcache.h>
#include <src/gvdocument.h>
#include <src/gvfileviewbase.h>
#include <src/gvfileviewstack.h>
#include <src/gvprintdialog.h>
#include <src/gvscrollpixmapview.h>
#include <src/gvslideshowdialog.h>
#include <src/gvslideshow.h>
#include <src/thumbnailloadjob.h>

#include "config.h"

// For now let's duplicate
const char CONFIG_CACHE_GROUP[]="cache";
const char CONFIG_FILEOPERATION_GROUP[]="file operations";
const char CONFIG_JPEGTRAN_GROUP[]="jpegtran";
const char CONFIG_SLIDESHOW_GROUP[]="slide show";
const char CONFIG_THUMBNAILLOADJOB_GROUP[]="thumbnail loading";
const char CONFIG_VIEW_GROUP[]="GwenviewPart View";


class GVDirPartView : public GVScrollPixmapView {
public:
	GVDirPartView(QWidget* parent, GVDocument* document, KActionCollection* actionCollection, GVDirPartBrowserExtension* browserExtension)
	: GVScrollPixmapView(parent, document, actionCollection), mBrowserExtension(browserExtension)
	{}

protected:
	void openContextMenu(const QPoint&) {
		mBrowserExtension->contextMenu();
	}

private:
	GVDirPartBrowserExtension* mBrowserExtension;
};


//Factory Code
typedef KParts::GenericFactory<GVDirPart> GVDirFactory;
K_EXPORT_COMPONENT_FACTORY( libgvdirpart /*library name*/, GVDirFactory )

GVDirPart::GVDirPart(QWidget* parentWidget, const char* /*widgetName*/, QObject* parent, const char* name,
		     const QStringList &) : KParts::ReadOnlyPart( parent, name )  {
	GVDirFactory::instance()->iconLoader()->addAppDir( "gwenview");
	setInstance( GVDirFactory::instance() );
	KGlobal::locale()->insertCatalogue( "gwenview" );

	mBrowserExtension = new GVDirPartBrowserExtension(this);
	mBrowserExtension->updateActions();

	mSplitter = new QSplitter(Qt::Horizontal, parentWidget, "gwenview-kpart-splitter");
	mSplitter->setFocusPolicy(QWidget::ClickFocus);

	// Create the widgets
	mDocument = new GVDocument(this);
	mFilesView = new GVFileViewStack(mSplitter, actionCollection());
	mPixmapView = new GVDirPartView(mSplitter, mDocument, actionCollection(), mBrowserExtension);

	mSlideShow = new GVSlideShow(mDocument);

	FileOperation::kpartConfig();
	mFilesView->kpartConfig();

	setWidget(mSplitter);

	KStdAction::saveAs( mDocument, SLOT(saveAs()), actionCollection(), "saveAs" );
	new KAction(i18n("Rotate &Right"), "rotate_cw", CTRL + Key_R, this, SLOT(rotateRight()), actionCollection(), "rotate_right");

	connect(mFilesView, SIGNAL(urlChanged(const KURL&)),
		this, SLOT(urlChanged(const KURL&)) );
	connect(mFilesView, SIGNAL(directoryChanged(const KURL&)),
		this, SLOT(directoryChanged(const KURL&)) );
	connect(mSlideShow, SIGNAL(nextURL(const KURL&)),
		this, SLOT(urlChanged(const KURL&)) );
	connect(mDocument, SIGNAL(loaded(const KURL&)),
		this, SLOT(loaded(const KURL&)) );

	QValueList<int> splitterSizes;
	splitterSizes.append(20);
	mSplitter->setSizes(splitterSizes);

	mToggleSlideShow = new KToggleAction(i18n("Slide Show..."), "slideshow", 0, this, SLOT(toggleSlideShow()), actionCollection(), "slideshow");
#if KDE_IS_VERSION(3, 3, 0)
	mToggleSlideShow->setCheckedState( i18n("Stop Slide Show" ));
#endif

	setXMLFile( "gvdirpart/gvdirpart.rc" );
}

GVDirPart::~GVDirPart() {
	delete mSlideShow;
}


void GVDirPart::partActivateEvent(KParts::PartActivateEvent* event) {
	KConfig* config=new KConfig("gwenviewrc");
	if (event->activated()) {
		FileOperation::readConfig(config, CONFIG_FILEOPERATION_GROUP);
		mSlideShow->readConfig(config, CONFIG_SLIDESHOW_GROUP);
		mPixmapView->readConfig(config, CONFIG_VIEW_GROUP);
		ThumbnailLoadJob::readConfig(config,CONFIG_THUMBNAILLOADJOB_GROUP);
		GVCache::instance()->readConfig(config,CONFIG_CACHE_GROUP);
	} else {
		mPixmapView->writeConfig(config, CONFIG_VIEW_GROUP);
	}
	delete config;
}


KAboutData* GVDirPart::createAboutData() {
	KAboutData* aboutData = new KAboutData( "gvdirpart", I18N_NOOP("GVDirPart"),
						"0.1", I18N_NOOP("Image Browser"),
						KAboutData::License_GPL,
						"(c) 2004, Jonathan Riddell <jr@jriddell.org>");
	return aboutData;
}

bool GVDirPart::openFile() {
	//unused because openURL implemented

	//mDocument->setFilename(mFile);
	return true;
}

bool GVDirPart::openURL(const KURL& url) {
	if (!url.isValid()) {
		return false;
	}

	emit started( 0 );
	m_url = url;
	m_url.adjustPath(1);

	emit setWindowCaption( m_url.prettyURL() );
	mFilesView->setDirURL(m_url);

	return true;
}

void GVDirPart::loaded(const KURL& url) {
	QString caption = url.filename();
	if( !mDocument->image().isNull())
		caption += QString(" %1 x %2").arg(mDocument->width()).arg(mDocument->height());
	emit setWindowCaption(caption);
	emit completed();
}

KURL GVDirPart::pixmapURL() {
	return mDocument->url();
}

void GVDirPart::toggleSlideShow() {
	if (mToggleSlideShow->isChecked()) {
		GVSlideShowDialog dialog(mSplitter, mSlideShow);
		if (!dialog.exec()) {
			mToggleSlideShow->setChecked(false);
			return;
		}
        KURL::List list;
        KFileItemListIterator it( *mFilesView->currentFileView()->items() );
        for ( ; it.current(); ++it ) {
            KFileItem* item=it.current();
            if (!item->isDir() && !GVArchive::fileItemIsArchive(item)) {
                list.append(item->url());
            }
        }
        if (list.count()==0) {
			mToggleSlideShow->setChecked(false);
            return;
        }
		//FIXME turn on full screen here (anyone know how?)
		mSlideShow->start(list);
	} else {
		//FIXME turn off full screen here
		mSlideShow->stop();
	}
}

void GVDirPart::print() {
	KPrinter printer;
	if ( !mDocument->filename().isEmpty() ) {
		printer.setDocName( m_url.filename() );
		KPrinter::addDialogPage( new GVPrintDialogPage( mDocument, mPixmapView, "GV page"));

		if (printer.setup(mPixmapView, QString::null, true)) {
			mDocument->print(&printer);
		}
	}
}

void GVDirPart::rotateRight() {
	mDocument->transform(GVImageUtils::ROT_90);
}

void GVDirPart::directoryChanged(const KURL& dirURL) {
	if( dirURL == m_url ) return;
	emit mBrowserExtension->openURLRequest(dirURL);
}

void GVDirPart::urlChanged(const KURL& url) {
	mDocument->setURL( url );
	mFilesView->setFileNameToSelect( url.filename());
}


/***** GVDirPartBrowserExtension *****/

GVDirPartBrowserExtension::GVDirPartBrowserExtension(GVDirPart* viewPart, const char* name)
	:KParts::BrowserExtension(viewPart, name) {
	mGVDirPart = viewPart;
	emit enableAction("print", true );
}

GVDirPartBrowserExtension::~GVDirPartBrowserExtension() {
}

void GVDirPartBrowserExtension::updateActions() {
	/*
	emit enableAction( "copy", true );
	emit enableAction( "cut", true );
	emit enableAction( "trash", true);
	emit enableAction( "del", true );
	emit enableAction( "editMimeType", true );
	*/
}

void GVDirPartBrowserExtension::del() {
	kdDebug() << k_funcinfo << endl;
}

void GVDirPartBrowserExtension::trash() {
	kdDebug() << k_funcinfo << endl;
}

void GVDirPartBrowserExtension::editMimeType() {
	kdDebug() << k_funcinfo << endl;
}

void GVDirPartBrowserExtension::refresh() {
	kdDebug() << k_funcinfo << endl;
}

void GVDirPartBrowserExtension::copy() {
	kdDebug() << k_funcinfo << endl;
}
void GVDirPartBrowserExtension::cut() {
	kdDebug() << k_funcinfo << endl;
}

void GVDirPartBrowserExtension::contextMenu() {
	/*FIXME Why is this KFileMetaInfo invalid?
	KFileMetaInfo metaInfo = KFileMetaInfo(m_gvImagePart->filePath());
	kdDebug() << k_funcinfo << "m_gvImagePart->filePath(): " << m_gvImagePart->filePath() << endl;
	kdDebug() << k_funcinfo << "metaInfo.isValid(): " << metaInfo.isValid() << endl;
	kdDebug() << k_funcinfo << "above" << endl;
	QString mimeType = metaInfo.mimeType();
	kdDebug() << k_funcinfo << "below" << endl;
	emit popupMenu(QCursor::pos(), m_gvImagePart->url(), mimeType);
	*/
	emit popupMenu(QCursor::pos(), mGVDirPart->pixmapURL(), 0);
}

void GVDirPartBrowserExtension::print() {
	mGVDirPart->print();
}

#include "gvdirpart.moc"
