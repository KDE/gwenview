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
#include <klocale.h>
#include <kstandarddirs.h>
#include <kparts/browserextension.h>
#include <kparts/genericfactory.h>
#include <kio/job.h>

#include "gvdirpart.h"
#include <src/gvscrollpixmapview.h>
#include <src/gvfileviewstack.h>
#include <src/gvpixmap.h>
#include <src/gvslideshow.h>
#include <src/gvslideshowdialog.h>

//Factory Code
typedef KParts::GenericFactory<GVDirPart> GVDirFactory;
K_EXPORT_COMPONENT_FACTORY( libgvdirpart /*library name*/, GVDirFactory );

GVDirPart::GVDirPart(QWidget* parentWidget, const char* /*widgetName*/, QObject* parent, const char* name,
		     const QStringList &) : KParts::ReadOnlyPart( parent, name )  {
	kdDebug() << k_funcinfo << endl;

	setInstance( GVDirFactory::instance() );

	m_browserExtension = new GVDirPartBrowserExtension(this);
	m_browserExtension->updateActions();

	m_splitter = new QSplitter(Qt::Horizontal, parentWidget, "gwenview-kpart-splitter");

	// Create the widgets
	m_gvPixmap = new GVPixmap(this);
	m_filesView = new GVFileViewStack(m_splitter, actionCollection());
	m_pixmapView = new GVScrollPixmapView(m_splitter, m_gvPixmap, actionCollection());

	mSlideShow = new GVSlideShow(m_filesView->selectFirst(), m_filesView->selectNext());

	m_filesView->kpartConfig();
	m_pixmapView->kpartConfig();

	setWidget(m_splitter);

	connect(m_pixmapView, SIGNAL(contextMenu()),
		m_browserExtension, SLOT(contextMenu()) );
	connect(m_filesView, SIGNAL(urlChanged(const KURL&)),
		m_gvPixmap, SLOT(setURL(const KURL&)) );
	connect(m_filesView, SIGNAL(directoryChanged(const KURL&)),
		m_browserExtension, SLOT(directoryChanged(const KURL&)) );

	QValueList<int> splitterSizes;
	splitterSizes.append(20);
	m_splitter->setSizes(splitterSizes);

	mToggleSlideShow = new KToggleAction(i18n("Slide Show..."), "slideshow", 0, this, SLOT(toggleSlideShow()), actionCollection(), "slideshow");


	setXMLFile( "gvdirpart/gvdirpart.rc" );
}

GVDirPart::~GVDirPart() {
	kdDebug() << k_funcinfo << endl;
}

KAboutData* GVDirPart::createAboutData() {
	kdDebug() << k_funcinfo << endl;
	KAboutData* aboutData = new KAboutData( "gvdirpart", I18N_NOOP("GVDirPart"),
						"0.1", I18N_NOOP("Image Viewer"),
						KAboutData::License_GPL,
						"(c) 2001, Jonathan Riddell <jr@jriddell.org>");
	return aboutData;
}

bool GVDirPart::openFile() {
        //unused because openURL implemented
        kdDebug() << k_funcinfo << endl;

        //m_gvPixmap->setFilename(m_file);
        return true;
}

bool GVDirPart::openURL(const KURL& url) {
	kdDebug() << k_funcinfo << url.prettyURL() << "<--end" << endl;

	if (!url.isValid())  {
		return false;
	}

	m_url = url;

	m_gvPixmap->setDirURL(url);
	m_filesView->setURL(url, 0);
	emit setWindowCaption( url.prettyURL() );

	return true;
}

void GVDirPart::setKonquerorWindowCaption(const QString& url) {
	kdDebug() << k_funcinfo << url << endl;
	emit setWindowCaption(url);
	kdDebug() << k_funcinfo << "done" << endl;
}

KURL GVDirPart::pixmapURL() {
	return m_pixmapView->pixmapURL();
}

void GVDirPart::toggleSlideShow() {
	if (mToggleSlideShow->isChecked()) {
		GVSlideShowDialog dialog(m_splitter, mSlideShow);
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

/***** GVDirPartBrowserExtension *****/

GVDirPartBrowserExtension::GVDirPartBrowserExtension(GVDirPart* viewPart, const char* name)
	:KParts::BrowserExtension(viewPart, name) {
	m_gvDirPart = viewPart;
}

GVDirPartBrowserExtension::~GVDirPartBrowserExtension() {
}

void GVDirPartBrowserExtension::updateActions() {
	kdDebug() << k_funcinfo << endl;
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
	kdDebug() << k_funcinfo << endl;
	emit openURLRequest(dirURL);
}

void GVDirPartBrowserExtension::contextMenu() {
	kdDebug() << k_funcinfo << endl;
	/*FIXME Why is this KFileMetaInfo invalid?
	KFileMetaInfo metaInfo = KFileMetaInfo(m_gvImagePart->filePath());
	kdDebug() << k_funcinfo << "m_gvImagePart->filePath(): " << m_gvImagePart->filePath() << endl;
	kdDebug() << k_funcinfo << "metaInfo.isValid(): " << metaInfo.isValid() << endl;
	kdDebug() << k_funcinfo << "above" << endl;
	QString mimeType = metaInfo.mimeType();
	kdDebug() << k_funcinfo << "below" << endl;
	emit popupMenu(QCursor::pos(), m_gvImagePart->url(), mimeType);
	*/
	kdDebug() << k_funcinfo << "url: " << m_gvDirPart->url() << endl;
	kdDebug() << k_funcinfo << "filename: " << m_gvDirPart->pixmapURL() << endl;
	emit popupMenu(QCursor::pos(), m_gvDirPart->pixmapURL(), 0);
}

#include "gvdirpart.moc"
