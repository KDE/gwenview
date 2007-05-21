/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#include "documentview.moc"

// Qt
#include <QLabel>
#include <QVBoxLayout>

// KDE
#include <klocale.h>
#include <kstatusbar.h>


namespace Gwenview {

struct DocumentViewPrivate {
	QLabel* mNoDocumentLabel;
	QWidget* mViewContainer;
	QVBoxLayout* mViewContainerLayout;
	KStatusBar* mStatusBar;
};

DocumentView::DocumentView(QWidget* parent)
: QStackedWidget(parent)
, d(new DocumentViewPrivate)
{
	d->mNoDocumentLabel = new QLabel(this);
	addWidget(d->mNoDocumentLabel);
	d->mNoDocumentLabel->setText(i18n("No document selected"));
	d->mNoDocumentLabel->setAlignment(Qt::AlignCenter);
	d->mNoDocumentLabel->setAutoFillBackground(true);
	QPalette palette;
	palette.setColor(QPalette::Window, Qt::black);
	palette.setColor(QPalette::WindowText, Qt::white);
	d->mNoDocumentLabel->setPalette(palette);

	d->mViewContainer = new QWidget(this);
	addWidget(d->mViewContainer);
	d->mStatusBar = new KStatusBar(d->mViewContainer);
	d->mStatusBar->hide();

	d->mViewContainerLayout = new QVBoxLayout(d->mViewContainer);
	d->mViewContainerLayout->addWidget(d->mStatusBar);
	d->mViewContainerLayout->setMargin(0);
	d->mViewContainerLayout->setSpacing(0);
}

DocumentView::~DocumentView() {
	delete d;
}

void DocumentView::setView(QWidget* view) {
	if (view) {
		// Insert the widget above the status bar
		d->mViewContainerLayout->insertWidget(0 /* position */, view, 1 /* stretch */);
		setCurrentWidget(d->mViewContainer);
	} else {
		setCurrentWidget(d->mNoDocumentLabel);
	}
}

QWidget* DocumentView::viewContainer() const {
	return d->mViewContainer;
}

KStatusBar* DocumentView::statusBar() const {
	return d->mStatusBar;
}

QSize DocumentView::sizeHint() const {
	return QSize(400, 300);
}

} // namespace
