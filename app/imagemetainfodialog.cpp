// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "imagemetainfodialog.moc"

// Qt
#include <QTreeView>
#include <QHeaderView>

// KDE
#include <kdebug.h>
#include <klocale.h>

// STL
#include <memory>

// Local
#include <lib/preferredimagemetainfomodel.h>

namespace Gwenview {


/**
 * A tree view which is always fully expanded
 */
class ExpandedTreeView : public QTreeView {
public:
	ExpandedTreeView(QWidget* parent)
	: QTreeView(parent) {}

protected:
	virtual void rowsInserted(const QModelIndex& parent, int start, int end) {
		QTreeView::rowsInserted(parent, start, end);
		expand(parent);
	}

	virtual void reset() {
		QTreeView::reset();
		expandAll();
	}
};


struct ImageMetaInfoDialogPrivate {
	std::auto_ptr<PreferredImageMetaInfoModel> mModel;
	QTreeView* mTreeView;
};


ImageMetaInfoDialog::ImageMetaInfoDialog(QWidget* parent)
: KDialog(parent)
, d(new ImageMetaInfoDialogPrivate) {
	d->mTreeView = new ExpandedTreeView(this);
	d->mTreeView->setRootIsDecorated(false);
	d->mTreeView->setIndentation(0);
	setMainWidget(d->mTreeView);
	setCaption(i18n("Meta Information"));
	setButtons(KDialog::Close);
}


ImageMetaInfoDialog::~ImageMetaInfoDialog() {
	delete d;
}


void ImageMetaInfoDialog::setMetaInfo(ImageMetaInfoModel* model, const QStringList& list) {
	if (model) {
		d->mModel.reset(new PreferredImageMetaInfoModel(model, list));
		connect(d->mModel.get(), SIGNAL(preferredMetaInfoKeyListChanged(const QStringList&)),
			this, SIGNAL(preferredMetaInfoKeyListChanged(const QStringList&)) );
	} else {
		d->mModel.reset(0);
	}
	d->mTreeView->setModel(d->mModel.get());

	d->mTreeView->header()->resizeSection(0, sizeHint().width() / 2 - KDialog::marginHint()*2);
}


QSize ImageMetaInfoDialog::sizeHint() const {
	return QSize(400, 300);
}


} // namespace
