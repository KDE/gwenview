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
#include <klocale.h>

// Local
#include <lib/gwenviewconfig.h>
#include "kipiinterface.h"

namespace Gwenview {


struct KIPIExportActionPrivate {
	KIPIExportAction* q;
	KIPIInterface* mKIPIInterface;
	QString mDefaultActionText;
	QAction* mDefaultAction;

	void initFromStoredDefaultAction() {
		mDefaultActionText = GwenviewConfig::defaultExportPluginText();
		if (!mDefaultActionText.isEmpty()) {
			q->setText(mDefaultActionText);
			q->setIcon(KIcon(GwenviewConfig::defaultExportPluginIconName()));
		}
		mDefaultAction = 0;
		updateButtonBehavior();
	}

	void updateButtonBehavior() {
		bool splitButton = !mDefaultActionText.isEmpty() || mDefaultAction != 0;
		q->setDelayed(splitButton);
		q->setStickyMenu(splitButton);
	}
};

/**
 * If no default action set: clicking the button shows the menu
 * If a default action is set: clicking the button triggers it, clicking the arrow shows the menu
 * When an action in the menu is triggered, it is set as the default action
 */
KIPIExportAction::KIPIExportAction(QObject* parent)
: KToolBarPopupAction(KIcon("document-export"), i18n("Export"), parent)
, d(new KIPIExportActionPrivate) {
	d->q = this;
	d->mKIPIInterface = 0;
	d->initFromStoredDefaultAction();

	connect(this, SIGNAL(triggered(bool)), SLOT(triggerDefaultAction()));
	connect(menu(), SIGNAL(aboutToShow()), SLOT(init()));
}


KIPIExportAction::~KIPIExportAction() {
	delete d;
}


void KIPIExportAction::setKIPIInterface(KIPIInterface* interface) {
	d->mKIPIInterface = interface;
}


void KIPIExportAction::triggerDefaultAction() {
	init();
	if (!d->mDefaultAction) {
		kWarning() << "No default action, this should not happen!";
		return;
	}
	d->mDefaultAction->trigger();
}


void KIPIExportAction::init() {
	if (!menu()->isEmpty()) {
		return;
	}
	// Fill the menu and init mDefaultAction if we find it
	d->mKIPIInterface->loadPlugins();
	connect(menu(), SIGNAL(triggered(QAction*)), SLOT(setDefaultAction(QAction*)));
	Q_FOREACH(QAction* action, d->mKIPIInterface->pluginActions(KIPI::ExportPlugin)) {
		menu()->addAction(action);
		if (action->text() == d->mDefaultActionText) {
			d->mDefaultAction = action;
			d->updateButtonBehavior();
		}
	}
}


void KIPIExportAction::setDefaultAction(QAction* action) {
	d->mDefaultAction = action;
	setIcon(action->icon());
	setText(action->text());

	GwenviewConfig::setDefaultExportPluginText(action->text());
	GwenviewConfig::setDefaultExportPluginIconName(action->icon().name());
}


} // namespace
