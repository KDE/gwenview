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
#include <kdebug.h>
#include <kaction.h>
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

	// Create the widgets
	m_gvPixmap = new GVPixmap(this);
	m_pixmapView = new GVScrollPixmapView(parentWidget, m_gvPixmap, actionCollection());
	setWidget(m_pixmapView);

	// Example action creation code
	KAction* m_openCloseAll = new KAction( i18n("Open All"), 0, this, SLOT( slotExample() ), actionCollection(), "openCloseAll" );

	//FIXME why won't this work?
	KAction* m_exampleAction = KStdAction::zoomIn(this, SLOT(slotExample()), actionCollection(), "zoomin");

	//FIXME why does this not work without path relative
	setXMLFile( "gvimagepart/gvimagepart.rc" );
}

GVImagePart::~GVImagePart() {
	kdDebug() << k_funcinfo << endl;
}

KAboutData* GVImagePart::createAboutData() {
	kdDebug() << k_funcinfo << endl;
	KAboutData* aboutData = new KAboutData( "gvdirpart", I18N_NOOP("GVDirPart"),
						"0.1", I18N_NOOP("Image Viewer"),
						KAboutData::License_GPL,
						"(c) 2001, Jonathan Riddell <jr@jriddell.org>");
	return aboutData;
}

bool GVImagePart::openURL(const KURL& url) {
	kdDebug() << k_funcinfo << endl;

	if (!url.isValid())  {
		return false;
	}
	if (!url.isLocalFile())  {
		return false;
	}

	m_gvPixmap->setURL(url);
	emit setWindowCaption( url.prettyURL() );

	return true;
}

bool GVImagePart::openFile() {
	//unused because openURL implemented
	kdDebug() << k_funcinfo << endl;
	m_gvPixmap->setFilename(m_file);
	return true;
}

void GVImagePart::slotExample() {
	kdDebug() << k_funcinfo << endl;
	//Example KAction
}

#include "gvimagepart.moc"
