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

// KF
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

        if (mDefaultAction && mExportActionList.contains(mDefaultAction)) {
            if (mExportActionList.count() > 1) {
                menu->addSection(i18n("Last Used Plugin"));
            }
            menu->addAction(mDefaultAction);
            menu->addSection(i18n("Other Plugins"));
        }
        for (QAction * action : qAsConst(mExportActionList)) {
            action->setIconVisibleInMenu(true);
            if (action != mDefaultAction) {
                menu->addAction(action);
            }
        }
    }
};

KIPIExportAction::KIPIExportAction(QObject* parent)
: KToolBarPopupAction(QIcon::fromTheme("document-share"), i18nc("@action", "Share"), parent)
, d(new KIPIExportActionPrivate)
{
    d->q = this;
    d->mKIPIInterface = nullptr;
    d->mDefaultAction = nullptr;
    setToolTip(i18nc("@info:tooltip", "Share images using various services"));

    setDelayed(false);
    connect(menu(), &QMenu::aboutToShow, this, &KIPIExportAction::init);
    connect(menu(), &QMenu::triggered, this, &KIPIExportAction::slotPluginTriggered);
}

KIPIExportAction::~KIPIExportAction()
{
    delete d;
}

void KIPIExportAction::setKIPIInterface(KIPIInterface* interface)
{
    d->mKIPIInterface = interface;
    connect(d->mKIPIInterface, &KIPIInterface::loadingFinished, this, &KIPIExportAction::init);
}

void KIPIExportAction::init()
{
    d->mExportActionList = d->mKIPIInterface->pluginActions(KIPI::ExportPlugin);
    if (d->mKIPIInterface->isLoadingFinished()) {
        // Look for default action
        QString defaultActionText = GwenviewConfig::defaultExportPluginText();
        for (QAction* action : qAsConst(d->mExportActionList)) {
            if (action->text() == defaultActionText) {
                setDefaultAction(action);
                break;
            }
        }
        // We are done, don't come back next time menu is shown
        disconnect(menu(), &QMenu::aboutToShow, this, &KIPIExportAction::init);
        // TODO: Temporary fix for the 'Share' menu not showing when updated
        // the second time. See Bug 395034 and D13312
        d->updateMenu();
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
