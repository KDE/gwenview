// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include "gvcore.h"

// Qt
#include <QApplication>
#include <QList>
#include <QProgressDialog>
#include <QWidget>

// KDE
#include <klocale.h>
#include <kurl.h>

// Local
#include <lib/document/documentfactory.h>

namespace Gwenview {


struct GvCorePrivate {
	QWidget* mParent;
};


GvCore::GvCore(QWidget* parent)
: QObject(parent)
, d(new GvCorePrivate) {
	d->mParent = parent;
}


GvCore::~GvCore() {
	delete d;
}


void GvCore::saveAll() {
	QList<KUrl> lst = DocumentFactory::instance()->modifiedDocumentList();

	QProgressDialog progress(d->mParent);
	progress.setLabelText(i18nc("@info:progress saving all image changes", "Saving..."));
	progress.setCancelButtonText(i18n("&Stop"));
	progress.setMinimum(0);
	progress.setMaximum(lst.size());
	progress.setWindowModality(Qt::WindowModal);
	progress.show();

	// FIXME: Save in a separate thread
	Q_FOREACH(const KUrl& url, lst) {
		Document::Ptr doc = DocumentFactory::instance()->load(url);
		Document::SaveResult saveResult = doc->save(url, doc->format());
		if (saveResult != Document::SR_OK) {
			// FIXME: Message
			return;
		}
		progress.setValue(progress.value() + 1);
		if (progress.wasCanceled()) {
			return;
		}
		qApp->processEvents();
	}
}


} // namespace
