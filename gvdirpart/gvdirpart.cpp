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
#include <qfile.h>
#include <qsplitter.h>
#include <qvaluelist.h>

#include <kdebug.h>
#include <kaction.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kparts/browserextension.h>
#include <kparts/genericfactory.h>

#include "gvdirpart.h"
#include <libgwenview/gvscrollpixmapview.h>
#include <libgwenview/gvfileviewstack.h>
#include <libgwenview/gvpixmap.h>

//Factory Code
typedef KParts::GenericFactory<GVDirPart> GVDirFactory;
K_EXPORT_COMPONENT_FACTORY( libgvdirpart /*library name*/, GVDirFactory );

GVDirPart::GVDirPart(QWidget* parentWidget, const char* /*widgetName*/, QObject* parent, const char* name,
		     const QStringList &) : KParts::ReadOnlyPart( parent, name )  {
	kdDebug() << k_funcinfo << endl;

	setInstance( GVDirFactory::instance() );

	m_splitter = new QSplitter(Qt::Horizontal, parentWidget, "splitter");

	// Create the widgets
	m_gvPixmap = new GVPixmap(this);
	m_filesView = new GVFileViewStack(m_splitter, actionCollection());
	m_pixmapView = new GVScrollPixmapView(m_splitter, m_gvPixmap, actionCollection());
	setWidget(m_splitter);

	connect(m_filesView, SIGNAL(urlChanged(const KURL&)),
		m_gvPixmap, SLOT(setURL(const KURL&)) );

	QValueList<int> splitterSizes;
	splitterSizes.append(10);
	m_splitter->setSizes(splitterSizes);

	// Example action creation code
	m_exampleAction = new KAction( i18n("Example"), 0, this, SLOT( slotExample() ), actionCollection(), "openCloseAll" );

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

bool GVDirPart::openURL(const KURL& url) {
	kdDebug() << k_funcinfo << endl;

	if (!url.isValid())  {
		return false;
	}
	if (!url.isLocalFile())  {
		return false;
	}

	m_gvPixmap->setDirURL(url);
	m_filesView->setURL(url, 0);
	emit setWindowCaption( url.prettyURL() );

	return true;
}


bool GVDirPart::openFile() {
	//unused because openURL implemented
	kdDebug() << k_funcinfo << endl;
	m_gvPixmap->setFilename(m_file);

	return true;
}

void GVDirPart::slotExample() {
	//Example KAction
	m_filesView->setMode(GVFileViewStack::Thumbnail);
}

#include "gvdirpart.moc"
