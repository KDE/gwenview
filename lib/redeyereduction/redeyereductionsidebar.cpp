// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include "redeyereductionsidebar.moc"

// Qt
#include <QPointer>
#include <QPushButton>

// KDE
#include "klocale.h"

// Local
#include "redeyereductionimageoperation.h"
#include "redeyereductiontool.h"
#include "imageview.h"
#include "ui_redeyereductionsidebar.h"

namespace Gwenview {


struct RedEyeReductionSideBarPrivate : public Ui_RedEyeReductionSideBar {
	Document::Ptr mDocument;
	QWidget* mWidget;
	AbstractImageViewTool* mPreviousTool;
	QPointer<RedEyeReductionTool> mRedEyeReductionTool;
};


RedEyeReductionSideBar::RedEyeReductionSideBar(QWidget* parent, ImageView* imageView, Document::Ptr document)
: QWidget(parent)
, d(new RedEyeReductionSideBarPrivate) {
	d->mDocument = document;
	d->mRedEyeReductionTool = new RedEyeReductionTool(imageView);
	d->mPreviousTool = imageView->currentTool();
	imageView->setCurrentTool(d->mRedEyeReductionTool);
	d->mWidget = new QWidget(this);
	d->setupUi(d->mWidget);
	d->mWidget->layout()->setMargin(0);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(d->mWidget);

	QPushButton* ok = d->buttonBox->button(QDialogButtonBox::Ok);
	Q_ASSERT(ok);
	ok->setText(i18n("Apply"));

	d->radiusSpinBox->setValue(d->mRedEyeReductionTool->radius());
	connect(d->radiusSpinBox, SIGNAL(valueChanged(int)),
		d->mRedEyeReductionTool, SLOT(setRadius(int)) );

	connect(d->buttonBox, SIGNAL(accepted()),
		SLOT(slotAccepted()) );

	connect(d->buttonBox, SIGNAL(rejected()),
		SIGNAL(done()) );
}


RedEyeReductionSideBar::~RedEyeReductionSideBar() {
	// The redEyeReduction tool is owned by the image view, so it may already have been
	// deleted, for example if we quit while the dialog is still opened.
	if (d->mRedEyeReductionTool) {
		d->mRedEyeReductionTool->imageView()->setCurrentTool(d->mPreviousTool);
	}
	delete d;
}


QRect RedEyeReductionSideBar::redEyeReductionRect() const {
	QRect rect;
	// FIXME
	return rect;
}


void RedEyeReductionSideBar::slotAccepted() {
	RedEyeReductionImageOperation* op = new RedEyeReductionImageOperation(redEyeReductionRect());
	emit imageOperationRequested(op);
	emit done();
}


} // namespace
