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
#include <kaction.h>
#include <kicontheme.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kparts/browserextension.h>
#include <kparts/genericfactory.h>
#include <kio/job.h>

#include "gvdirpart.h"
#include <src/gvscrollpixmapview.h>
#include <src/gvfileviewstack.h>
#include <src/gvdocument.h>
#include <src/gvprintdialog.h>
#include <src/gvslideshow.h>
#include <src/gvslideshowdialog.h>
#include <src/fileoperation.h>

//Factory Code
typedef KParts::GenericFactory<GVDirPart> GVDirFactory;
K_EXPORT_COMPONENT_FACTORY( libgvdirpart /*library name*/, GVDirFactory )

GVDirPart::GVDirPart(QWidget* parentWidget, const char* /*widgetName*/, QObject* parent, const char* name,
		     const QStringList &) : KParts::ReadOnlyPart( parent, name )  {
	setInstance( GVDirFactory::instance() );

	mBrowserExtension = new GVDirPartBrowserExtension(this);
	mBrowserExtension->updateActions();

	mSplitter = new QSplitter(Qt::Horizontal, parentWidget, "gwenview-kpart-splitter");

	// Create the widgets
	mDocument = new GVDocument(this);
	mFilesView = new GVFileViewStack(mSplitter, actionCollection());
	mPixmapView = new GVScrollPixmapView(mSplitter, mDocument, actionCollection());

	mSlideShow = new GVSlideShow(mFilesView->selectFirst(), mFilesView->selectNext());

	FileOperation::kpartConfig();
	mFilesView->kpartConfig();
	mPixmapView->kpartConfig();

	setWidget(mSplitter);

	KStdAction::saveAs( mDocument, SLOT(saveAs()), actionCollection(), "saveAs" );
	new KAction(i18n("Rotate &Right"), "rotate_cw", CTRL + Key_R, this, SLOT(rotateRight()), actionCollection(), "rotate_right");

	connect(mPixmapView, SIGNAL(contextMenu()),
		mBrowserExtension, SLOT(contextMenu()) );
	connect(mFilesView, SIGNAL(urlChanged(const KURL&)),
		mDocument, SLOT(setURL(const KURL&)) );
	connect(mFilesView, SIGNAL(directoryChanged(const KURL&)),
		mBrowserExtension, SLOT(directoryChanged(const KURL&)) );
	connect(mDocument, SIGNAL(loaded(const KURL&, const QString&)),
		this, SLOT(setKonquerorWindowCaption(const KURL&, const QString&)) );

	QValueList<int> splitterSizes;
	splitterSizes.append(20);
	mSplitter->setSizes(splitterSizes);

	// KIconLoader is weird.  If I preload them here it remembers about them for the following KAction.
	KIconLoader iconLoader = KIconLoader("gwenview");
	iconLoader.loadIconSet("slideshow", KIcon::Toolbar);

	mToggleSlideShow = new KToggleAction(i18n("Slide Show..."), "slideshow", 0, this, SLOT(toggleSlideShow()), actionCollection(), "slideshow");

	setXMLFile( "gvdirpart/gvdirpart.rc" );
}

GVDirPart::~GVDirPart() {
	delete mSlideShow;
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
	if (!url.isValid())  {
		return false;
	}

	m_url = url;

	mDocument->setDirURL(url);
	mFilesView->setURL(url, 0);
	emit setWindowCaption( url.prettyURL() );

	return true;
}

void GVDirPart::setKonquerorWindowCaption(const KURL& /*url*/, const QString& filename) {
	QString caption = QString(filename + " %1 x %2").arg(mDocument->width()).arg(mDocument->height());
	emit setWindowCaption(caption);
}

KURL GVDirPart::pixmapURL() {
	return mPixmapView->pixmapURL();
}

void GVDirPart::toggleSlideShow() {
	if (mToggleSlideShow->isChecked()) {
		GVSlideShowDialog dialog(mSplitter, mSlideShow);
		if (!dialog.exec()) {
			mToggleSlideShow->setChecked(false);
			return;
		}
		//FIXME turn on full screen here (anyone know how?)
		mSlideShow->start();
	} else {
		//FIXME turn off full screen here
		mSlideShow->stop();
	}
}

void GVDirPart::print() {
	KPrinter printer;
	if ( !mDocument->filename().isEmpty() ) {
		printer.setDocName( m_url.filename() );
		KPrinter::addDialogPage( new GVPrintDialogPage( mPixmapView, "GV page"));

		if (printer.setup(mPixmapView, QString::null, true)) {
			mDocument->print(&printer);
		}
	}
}

void GVDirPart::rotateRight() {
	mDocument->modify(GVImageUtils::ROT_90);
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

void GVDirPartBrowserExtension::directoryChanged(const KURL& dirURL) {
	emit openURLRequest(dirURL);
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
