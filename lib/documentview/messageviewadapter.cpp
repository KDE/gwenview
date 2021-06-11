// vim: set tabstop=4 shiftwidth=4 expandtab:
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
#include "messageviewadapter.h"

// Qt
#include <QGraphicsProxyWidget>

// KF
#include <KLocalizedString>

// Local
#include "gwenview_lib_debug.h"
#include <lib/document/document.h>
#include <lib/ui_messageview.h>

namespace Gwenview
{

struct MessageViewAdapterPrivate : Ui_MessageView
{
    Document::Ptr mDocument;
};

MessageViewAdapter::MessageViewAdapter()
: d(new MessageViewAdapterPrivate)
{
    auto* widget = new QWidget;
    widget->installEventFilter(this);
    d->setupUi(widget);
    d->mMessageWidget->setCloseButtonVisible(false);
    d->mMessageWidget->setWordWrap(true);

    setInfoMessage(i18n("No document selected"));

    widget->setAutoFillBackground(true);
    widget->setBackgroundRole(QPalette::Base);
    widget->setForegroundRole(QPalette::Text);

    auto* proxy = new QGraphicsProxyWidget;
    proxy->setWidget(widget);
    setWidget(proxy);
}

MessageViewAdapter::~MessageViewAdapter()
{
    delete d;
}

void MessageViewAdapter::setErrorMessage(const QString& main, const QString& detail)
{
    if (main.isEmpty()) {
        d->mMessageWidget->hide();
        return;
    }
    d->mMessageWidget->show();
    d->mMessageWidget->setMessageType(KMessageWidget::Error);
    QString message;
    if (detail.isEmpty()) {
        message = main;
    } else {
        message = QStringLiteral("<b>%1</b><br>%2").arg(main, detail);
    }
    d->mMessageWidget->setText(message);
}

void MessageViewAdapter::setInfoMessage(const QString& message)
{
    if (message.isEmpty()) {
        d->mMessageWidget->hide();
        return;
    }
    d->mMessageWidget->show();
    d->mMessageWidget->setMessageType(KMessageWidget::Information);
    d->mMessageWidget->setText(message);
}

Document::Ptr MessageViewAdapter::document() const
{
    return d->mDocument;
}

void MessageViewAdapter::setDocument(const Document::Ptr &doc)
{
    d->mDocument = doc;
}

bool MessageViewAdapter::eventFilter(QObject*, QEvent* ev)
{
    if (ev->type() == QEvent::KeyPress) {
        auto* event = static_cast<QKeyEvent*>(ev);
        if (event->modifiers() != Qt::NoModifier) {
            return false;
        }

        switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_Up:
            emit previousImageRequested();
            break;
        case Qt::Key_Right:
        case Qt::Key_Down:
            emit nextImageRequested();
            break;
        default:
            break;
        }
    }
    return false;
}

} // namespace
