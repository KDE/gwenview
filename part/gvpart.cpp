/*
Gwenview: an image viewer
Copyright 2007-2012 Aurélien Gâteau <agateau@kde.org>

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
// Qt
#include <QUrl>
#include <QAction>
#include <QDebug>
#include <QMenu>

// KDE
#include <KAboutData>
#include <KActionCollection>
#include <QFileDialog>
#include <KIconLoader>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KLocalizedString>
#include <KStandardAction>
#include <KPluginFactory>
#include <KPropertiesDialog>
#include <KJobWidgets>

// Local
#include "../lib/about.h"
#include "../lib/document/document.h"
#include "../lib/document/documentfactory.h"
#include "../lib/documentview/documentview.h"
#include "../lib/documentview/documentviewcontainer.h"
#include "../lib/documentview/documentviewcontroller.h"
#include "../lib/urlutils.h"
#include "../lib/zoomwidget.h"
#include "gvbrowserextension.h"
#include "gvpart.h"

//Factory Code
K_PLUGIN_FACTORY_WITH_JSON(GVPartFactory, "gvpart.json", registerPlugin<Gwenview::GVPart>();)

namespace Gwenview
{

GVPart::GVPart(QWidget* parentWidget, QObject* parent, const QVariantList& /*args*/)
: KParts::ReadOnlyPart(parent)
{
    QScopedPointer<KAboutData> aboutData(createAboutData());
    setComponentData(*aboutData, false);

    DocumentViewContainer* container = new DocumentViewContainer(parentWidget);
    setWidget(container);
    mDocumentView = container->createView();

    connect(mDocumentView, &DocumentView::captionUpdateRequested,
            this, &KParts::Part::setWindowCaption);
    connect(mDocumentView, &DocumentView::completed,
            this, QOverload<>::of(&KParts::ReadOnlyPart::completed));

    connect(mDocumentView, &DocumentView::contextMenuRequested,
            this, &GVPart::showContextMenu);

    // Necessary to have zoom actions
    DocumentViewController* documentViewController = new DocumentViewController(actionCollection(), this);
    documentViewController->setView(mDocumentView);

    QAction * action = new QAction(actionCollection());
    action->setText(i18nc("@action", "Properties"));
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Return));
    connect(action, &QAction::triggered, this, &GVPart::showProperties);
    actionCollection()->addAction(QStringLiteral("file_show_properties"), action);


    KStandardAction::saveAs(this, SLOT(saveAs()), actionCollection());

    new GVBrowserExtension(this);

    setXMLFile(QStringLiteral("gvpart.rc"), true);
}

void GVPart::showProperties()
{
    KPropertiesDialog::showDialog(url(), widget());
}

bool GVPart::openUrl(const QUrl& url)
{
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
    DocumentView::Setup setup;
    setup.zoomToFit = true;
    mDocumentView->openUrl(url, setup);
    mDocumentView->setCurrent(true);
    return true;
}

KAboutData* GVPart::createAboutData()
{
    KAboutData* aboutData = Gwenview::createAboutData(
        QStringLiteral("gvpart"), /* appname */
        i18n("Gwenview KPart")    /* programName */
        );
    aboutData->setShortDescription(i18n("An Image Viewer"));
    return aboutData;
}

inline void addActionToMenu(QMenu* menu, KActionCollection* actionCollection, const char* name)
{
    QAction* action = actionCollection->action(name);
    if (action) {
        menu->addAction(action);
    }
}

void GVPart::showContextMenu()
{
    QMenu menu(widget());
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

void GVPart::saveAs()
{
    const QUrl srcUrl = url();
    const QUrl dstUrl = QFileDialog::getSaveFileUrl(widget(), QString(), srcUrl);
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
    connect(job, &KJob::result,
            this, &GVPart::showJobError);
}

void GVPart::showJobError(KJob* job)
{
    if (job->error() != 0) {
        KJobUiDelegate* ui = static_cast<KIO::Job*>(job)->uiDelegate();
        if (!ui) {
            qCritical() << "Saving failed. job->ui() is null.";
            return;
        }
        KJobWidgets::setWindow(job, widget());
        ui->showErrorMessage();
    }
}

} // namespace

#include "gvpart.moc"
