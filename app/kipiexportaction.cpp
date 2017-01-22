// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2010 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "kipiexportaction.h"

// Qt
#include <QMenu>

// KDE
#include <KLocalizedString>

// Local
#include <lib/gwenviewconfig.h>
#include "kipiinterface.h"

namespace Gwenview
{

struct KIPIExportActionPrivate
{
    KIPIExportAction* q;
    KIPIInterface* mKIPIInterface;
    QAction* mDefaultAction;
    QList<QAction*> mExportActionList;

    void updateMenu()
    {
        QMenu* menu = static_cast<QMenu*>(q->menu());
        menu->clear();

        if (mDefaultAction) {
            menu->addSection(i18n("Last Used Plugin"));
            menu->addAction(mDefaultAction);
            menu->addSection(i18n("Other Plugins"));
        }
        Q_FOREACH(QAction * action, mExportActionList) {
            action->setIconVisibleInMenu(true);
            if (action != mDefaultAction) {
                menu->addAction(action);
            }
        }
        if (menu->isEmpty()) {
            QAction* action = new QAction(menu);
            action->setText(i18n("No Plugin Found"));
            action->setEnabled(false);
            menu->addAction(action);
        }
    }
};

KIPIExportAction::KIPIExportAction(QObject* parent)
: KToolBarPopupAction(QIcon::fromTheme("document-share"), i18nc("@action", "Share"), parent)
, d(new KIPIExportActionPrivate)
{
    d->q = this;
    d->mKIPIInterface = 0;
    d->mDefaultAction = 0;
    setToolTip(i18nc("@info:tooltip", "Share images using various services"));

    setDelayed(false);
    connect(menu(), SIGNAL(aboutToShow()), SLOT(init()));
    connect(menu(), SIGNAL(triggered(QAction*)), SLOT(slotPluginTriggered(QAction*)));
}

KIPIExportAction::~KIPIExportAction()
{
    delete d;
}

void KIPIExportAction::setKIPIInterface(KIPIInterface* interface)
{
    d->mKIPIInterface = interface;
}

void KIPIExportAction::init()
{
    d->mExportActionList = d->mKIPIInterface->pluginActions(KIPI::ExportPlugin);
    if (d->mKIPIInterface->isLoadingFinished()) {
        // Look for default action
        QString defaultActionText = GwenviewConfig::defaultExportPluginText();
        Q_FOREACH(QAction* action, d->mExportActionList) {
            if (action->text() == defaultActionText) {
                setDefaultAction(action);
                break;
            }
        }
        // We are done, don't come back next time menu is shown
        disconnect(menu(), SIGNAL(aboutToShow()), this, SLOT(init()));
    } else {
        // Loading is in progress, come back when it is done
        connect(d->mKIPIInterface, &KIPIInterface::loadingFinished, this, &KIPIExportAction::init);
    }
    d->updateMenu();
}

void KIPIExportAction::setDefaultAction(QAction* action)
{
    if (action == d->mDefaultAction) {
        return;
    }
    d->mDefaultAction = action;

    GwenviewConfig::setDefaultExportPluginText(action->text());
}

void KIPIExportAction::slotPluginTriggered(QAction* action)
{
    setDefaultAction(action);
    d->updateMenu();
}

} // namespace
