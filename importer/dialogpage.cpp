// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "dialogpage.h"

// Qt
#include <QEventLoop>
#include <QList>
#include <QSignalMapper>
#include <QVBoxLayout>

// KDE
#include <KPushButton>

// Local
#include <ui_dialogpage.h>

namespace Gwenview
{

struct DialogPagePrivate : public Ui_DialogPage
{
    QVBoxLayout* mLayout;
    QList<KPushButton*> mButtons;
    QSignalMapper* mMapper;
    QEventLoop* mEventLoop;
};

DialogPage::DialogPage(QWidget* parent)
: QWidget(parent)
, d(new DialogPagePrivate)
{
    d->setupUi(this);
    d->mLayout = new QVBoxLayout(d->mButtonContainer);
    d->mMapper = new QSignalMapper(this);
    connect(d->mMapper, SIGNAL(mapped(int)), SLOT(slotMapped(int)));
}

DialogPage::~DialogPage()
{
    delete d;
}

void DialogPage::removeButtons()
{
    qDeleteAll(d->mButtons);
    d->mButtons.clear();
}

void DialogPage::setText(const QString& text)
{
    d->mLabel->setText(text);
}

int DialogPage::addButton(const KGuiItem& item)
{
    int id = d->mButtons.size();
    KPushButton* button = new KPushButton(item);
    button->setFixedHeight(button->sizeHint().height() * 2);

    connect(button, SIGNAL(clicked()), d->mMapper, SLOT(map()));
    d->mLayout->addWidget(button);
    d->mMapper->setMapping(button, id);
    d->mButtons << button;
    return id;
}

int DialogPage::exec()
{
    QEventLoop loop;
    d->mEventLoop = &loop;
    return loop.exec();
}

void DialogPage::slotMapped(int value)
{
    d->mEventLoop->exit(value);
}

} // namespace
