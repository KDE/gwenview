// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#include "messageviewadapter.moc"

// Qt

// KDE
#include <klocale.h>

// Local
#include <lib/document/document.h>
#include <lib/ui_messageview.h>

namespace Gwenview {


struct MessageViewAdapterPrivate : Ui_MessageView {
	Document::Ptr mDocument;
	qreal mOpacity;
};


MessageViewAdapter::MessageViewAdapter(QWidget* parent)
: AbstractDocumentViewAdapter(parent)
, d(new MessageViewAdapterPrivate) {
	d->mOpacity = 1.;
	QWidget* widget = new QWidget(parent);
	d->setupUi(widget);
	d->mMessageWidget->setCloseButtonVisible(false);
	d->mMessageWidget->setWordWrap(true);

	setInfoMessage(i18n("No document selected"));

	widget->setAutoFillBackground(true);
	widget->setBackgroundRole(QPalette::Base);
	widget->setForegroundRole(QPalette::Text);

	setWidget(widget);
}


MessageViewAdapter::~MessageViewAdapter() {
	delete d;
}


void MessageViewAdapter::setErrorMessage(const QString& main, const QString& detail) {
	d->mMessageWidget->setMessageType(KMessageWidget::Error);
	QString message;
	if (detail.isEmpty()) {
		message = main;
	} else {
		message = QString("<b>%1</b><br>%2").arg(main).arg(detail);
	}
	d->mMessageWidget->setText(message);
}


void MessageViewAdapter::setInfoMessage(const QString& message) {
	d->mMessageWidget->setMessageType(KMessageWidget::Information);
	d->mMessageWidget->setText(message);
}


void MessageViewAdapter::installEventFilterOnViewWidgets(QObject* object) {
	widget()->installEventFilter(object);
}


Document::Ptr MessageViewAdapter::document() const {
	return d->mDocument;
}


void MessageViewAdapter::setDocument(Document::Ptr doc) {
	d->mDocument = doc;
}


qreal MessageViewAdapter::opacity() const {
	return d->mOpacity;
}


void MessageViewAdapter::setOpacity(qreal value) {
	d->mOpacity = value;
}



} // namespace
