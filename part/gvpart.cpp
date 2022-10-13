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
#include <QAction>
#include <QDebug>
#include <QFileDialog>
#include <QMenu>
#include <QUrl>

// KF
#include <KAboutData>
#include <KActionCollection>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KIconLoader>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KPropertiesDialog>
#include <KStandardAction>

// Local
#include "../lib/document/document.h"
#include "../lib/document/documentfactory.h"
#include "../lib/documentview/documentview.h"
#include "../lib/documentview/documentviewcontainer.h"
#include "../lib/documentview/documentviewcontroller.h"
#include "../lib/urlutils.h"
#include "gvbrowserextension.h"
#include "gvpart.h"

namespace Gwenview
{
K_PLUGIN_CLASS_WITH_JSON(GVPart, "gvpart.json")

GVPart::GVPart(QWidget *parentWidget, QObject *parent, const KPluginMetaData &metaData, const QVariantList & /*args*/)
    : KParts::ReadOnlyPart(parent)
{
    setMetaData(metaData);

    auto container = new DocumentViewContainer(parentWidget);
    setWidget(container);
    mDocumentView = container->createView();

    connect(mDocumentView, &DocumentView::captionUpdateRequested, this, &KParts::Part::setWindowCaption);
    connect(mDocumentView, &DocumentView::completed, this, QOverload<>::of(&KParts::ReadOnlyPart::completed));

    connect(mDocumentView, &DocumentView::contextMenuRequested, this, &GVPart::showContextMenu);

    // Necessary to have zoom actions
    auto documentViewController = new DocumentViewController(actionCollection(), this);
    documentViewController->setView(mDocumentView);

    auto action = new QAction(actionCollection());
    action->setText(i18nc("@action", "Properties"));
    action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Return));
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

bool GVPart::openUrl(const QUrl &url)
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

inline void addActionToMenu(QMenu *menu, KActionCollection *actionCollection, const char *name)
{
    QAction *action = actionCollection->action(name);
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

    KIO::Job *job;
    Document::Ptr doc = DocumentFactory::instance()->load(srcUrl);
    const QByteArray rawData = doc->rawData();
    if (!rawData.isEmpty()) {
        job = KIO::storedPut(rawData, dstUrl, -1);
    } else {
        job = KIO::file_copy(srcUrl, dstUrl);
    }
    connect(job, &KJob::result, this, &GVPart::showJobError);
}

void GVPart::showJobError(KJob *job)
{
    if (job->error() != 0) {
        KJobUiDelegate *ui = static_cast<KIO::Job *>(job)->uiDelegate();
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
