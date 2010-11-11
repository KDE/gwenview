// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#include "kipiexportaction.moc"

// Qt
#include <QMenu>

// KDE
#include <kdebug.h>
#include <kmenu.h>
#include <klocale.h>

// Local
#include <lib/gwenviewconfig.h>
#include "kipiinterface.h"

namespace Gwenview {


struct KIPIExportActionPrivate {
	KIPIExportAction* q;
	KIPIInterface* mKIPIInterface;
	KMenu* mMenu;
	QAction* mDefaultAction;
	QList<QAction*> mExportActionList;

	void updateMenu() {
		mMenu->clear();

		if (mDefaultAction) {
			mMenu->addTitle(i18n("Last Used Plugin"));
			mMenu->addAction(mDefaultAction);
			mMenu->addTitle(i18n("Other Plugins"));
		}
		Q_FOREACH(QAction* action, mExportActionList) {
			action->setIconVisibleInMenu(true);
			if (action != mDefaultAction) {
				mMenu->addAction(action);
			}
		}
		if (mMenu->isEmpty()) {
			QAction* action = new QAction(mMenu);
			action->setText(i18n("No Plugin Found"));
			action->setEnabled(false);
			mMenu->addAction(action);
		}
	}
};

/**
 * If no default action set: clicking the button shows the menu
 * If a default action is set: clicking the button triggers it, clicking the arrow shows the menu
 * When an action in the menu is triggered, it is set as the default action
 */
KIPIExportAction::KIPIExportAction(QObject* parent)
: KToolBarPopupAction(KIcon("document-export"), i18n("Share"), parent)
, d(new KIPIExportActionPrivate) {
	d->q = this;
	d->mKIPIInterface = 0;
	d->mMenu = new KMenu;
	d->mDefaultAction = 0;

	setDelayed(false);
	setMenu(d->mMenu);
	connect(d->mMenu, SIGNAL(aboutToShow()), SLOT(init()));
	connect(d->mMenu, SIGNAL(triggered(QAction*)), SLOT(setDefaultAction(QAction*)));
}


KIPIExportAction::~KIPIExportAction() {
	delete d->mMenu;
	delete d;
}


void KIPIExportAction::setKIPIInterface(KIPIInterface* interface) {
	d->mKIPIInterface = interface;
}


void KIPIExportAction::init() {
	if (!d->mMenu->isEmpty()) {
		return;
	}
	d->mKIPIInterface->loadPlugins();
	d->mExportActionList = d->mKIPIInterface->pluginActions(KIPI::ExportPlugin);

	// Look for default action
	QString defaultActionText = GwenviewConfig::defaultExportPluginText();
	Q_FOREACH(QAction* action, d->mExportActionList) {
		if (action->text() == defaultActionText) {
			setDefaultAction(action);
			break;
		}
	}

	d->updateMenu();
}


void KIPIExportAction::setDefaultAction(QAction* action) {
	if (action == d->mDefaultAction) {
		return;
	}
	d->mDefaultAction = action;

	GwenviewConfig::setDefaultExportPluginText(action->text());
	d->updateMenu();
}


} // namespace
