/*
Gwenview: an image viewer
Copyright 2007-2008 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "gvpart.moc"

// Qt

// KDE
#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kmenu.h>
#include <kstandardaction.h>
#include <kparts/genericfactory.h>
#include <kpropertiesdialog.h>

// Local
#include "../lib/document/document.h"
#include "../lib/document/documentfactory.h"
#include "../lib/documentview/documentview.h"
#include "../lib/imageformats/imageformats.h"
#include "../lib/urlutils.h"
#include "../lib/version.h"
#include "../lib/zoomwidget.h"
#include "gvbrowserextension.h"


//Factory Code
typedef KParts::GenericFactory<Gwenview::GVPart> GVPartFactory;
K_EXPORT_COMPONENT_FACTORY( gvpart /*library name*/, GVPartFactory )

namespace Gwenview {


GVPart::GVPart(QWidget* parentWidget, QObject* parent, const QStringList& /*args*/)
: KParts::ReadOnlyPart(parent)
{
        KGlobal::locale()->insertCatalog("gwenview");
	mDocumentView = new DocumentView(parentWidget, 0 /* slideShow */, actionCollection());
	mDocumentView->setZoomWidgetVisible(false);
	setWidget(mDocumentView);

	connect(mDocumentView, SIGNAL(captionUpdateRequested(const QString&)),
		SIGNAL(setWindowCaption(const QString&)));
	connect(mDocumentView, SIGNAL(completed()),
		SIGNAL(completed()));

	mDocumentView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(mDocumentView, SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(showContextMenu()) );

	KAction* action = new KAction(actionCollection());
	action->setText(i18nc("@action", "Properties"));
	connect(action, SIGNAL(triggered()), SLOT(showProperties()));
	actionCollection()->addAction("file_show_properties", action);

	KStandardAction::saveAs(this, SLOT(saveAs()), actionCollection());

	Gwenview::ImageFormats::registerPlugins();
	new GVBrowserExtension(this);

	setXMLFile("gvpart/gvpart.rc");
}


void GVPart::showProperties() {
	KPropertiesDialog::showDialog(url(), mDocumentView);
}


bool GVPart::openFile() {
	return false;
}


bool GVPart::openUrl(const KUrl& url) {
	if (!url.isValid()) {
		return false;
	}
	setUrl(url);
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	if (arguments().reload()) {
		doc->reload();
	}
	if (!UrlUtils::urlIsFastLocalFile(url)) {
		// Keep raw data of remote files to avoid downloading them again in
		// saveAs()
		doc->setKeepRawData(true);
	}
	mDocumentView->openUrl(url);
	return true;
}


KAboutData* GVPart::createAboutData() {
	KAboutData* aboutData = new KAboutData(
		"gvpart",                /* appname */
		"gwenview",              /* catalogName */
		ki18n("Gwenview KPart"), /* programName */
		GWENVIEW_VERSION);       /* version */
	aboutData->setShortDescription(ki18n("An Image Viewer"));
	aboutData->setLicense(KAboutData::License_GPL);
	aboutData->setCopyrightStatement(ki18n("Copyright 2000-2010 Aurélien Gâteau"));
	aboutData->addAuthor(
		ki18n("Aurélien Gâteau"),
		ki18n("Main developer"),
		"agateau@kde.org");
	return aboutData;
}


inline void addActionToMenu(KMenu* menu, KActionCollection* actionCollection, const char* name) {
	QAction* action = actionCollection->action(name);
	if (action) {
		menu->addAction(action);
	}
}


void GVPart::showContextMenu() {
	KMenu menu(mDocumentView);
	addActionToMenu(&menu, actionCollection(), "file_save_as");
	menu.addSeparator();
	addActionToMenu(&menu, actionCollection(), "view_actual_size");
	addActionToMenu(&menu, actionCollection(), "view_zoom_to_fit");
	addActionToMenu(&menu, actionCollection(), "view_zoom_in");
	addActionToMenu(&menu, actionCollection(), "view_zoom_out");
	menu.addSeparator();
	addActionToMenu(&menu, actionCollection(), "file_show_properties");
	menu.exec(QCursor::pos());
}


void GVPart::saveAs() {
	KUrl srcUrl = url();
	KUrl dstUrl = KFileDialog::getSaveUrl(srcUrl.fileName(), QString(), widget());
	if (!dstUrl.isValid()) {
		return;
	}

	KIO::Job* job;
	Document::Ptr doc = DocumentFactory::instance()->load(srcUrl);
	QByteArray rawData = doc->rawData();
	if (rawData.length() > 0) {
		job = KIO::storedPut(rawData, dstUrl, -1);
	} else {
		job = KIO::file_copy(srcUrl, dstUrl);
	}
	connect(job, SIGNAL(result(KJob*)),
		this, SLOT(showJobError(KJob*)) );
}


void GVPart::showJobError(KJob* job) {
	if (job->error() != 0) {
		KIO::JobUiDelegate* ui = static_cast<KIO::Job*>(job)->ui();
		if (!ui) {
			kError() << "Saving failed. job->ui() is null.";
			return;
		}
		ui->setWindow(widget());
		ui->showErrorMessage();
	}
}


} // namespace
