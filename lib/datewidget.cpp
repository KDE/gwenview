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
#include "datewidget.h"

// Qt
#include <QDate>
#include <QHBoxLayout>

// KDE
#include <KDatePicker>
#include <KDebug>
#include <KIconLoader>
#include <KLocale>
#include <KGlobal>

// Local
#include <lib/statusbartoolbutton.h>

namespace Gwenview
{

struct DateWidgetPrivate
{
    DateWidget* q;

    QDate mDate;
    KDatePicker* mDatePicker;
    StatusBarToolButton* mPreviousButton;
    StatusBarToolButton* mDateButton;
    StatusBarToolButton* mNextButton;

    void setupDatePicker()
    {
        mDatePicker = new KDatePicker;
        /* Use Qt::Tool instead of Qt::Window so that the bubble does not appear in the task bar */
        //mDatePicker->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        mDatePicker->setWindowFlags(Qt::Popup);
        mDatePicker->hide();
        mDatePicker->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

        QObject::connect(mDatePicker, SIGNAL(dateEntered(QDate)),
                         q, SLOT(slotDatePickerModified(QDate)));
        QObject::connect(mDatePicker, SIGNAL(dateSelected(QDate)),
                         q, SLOT(slotDatePickerModified(QDate)));
    }

    void updateButton()
    {
        mDateButton->setText(KGlobal::locale()->formatDate(mDate, KLocale::ShortDate));
    }

    void adjustDate(int delta)
    {
        mDate = mDate.addDays(delta);
        updateButton();
        q->dateChanged(mDate);
    }
};

DateWidget::DateWidget(QWidget* parent)
: QWidget(parent)
, d(new DateWidgetPrivate)
{
    d->q = this;

    d->setupDatePicker();

    d->mPreviousButton = new StatusBarToolButton;
    d->mPreviousButton->setGroupPosition(StatusBarToolButton::GroupLeft);
    // FIXME: RTL
    d->mPreviousButton->setIcon(SmallIcon("go-previous"));
    connect(d->mPreviousButton, SIGNAL(clicked()), SLOT(goToPrevious()));

    d->mDateButton = new StatusBarToolButton;
    d->mDateButton->setGroupPosition(StatusBarToolButton::GroupCenter);
    connect(d->mDateButton, SIGNAL(clicked()), SLOT(showDatePicker()));

    d->mNextButton = new StatusBarToolButton;
    d->mNextButton->setGroupPosition(StatusBarToolButton::GroupRight);
    d->mNextButton->setIcon(SmallIcon("go-next"));
    connect(d->mNextButton, SIGNAL(clicked()), SLOT(goToNext()));

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(d->mPreviousButton);
    layout->addWidget(d->mDateButton);
    layout->addWidget(d->mNextButton);
}

DateWidget::~DateWidget()
{
    delete d->mDatePicker;
    delete d;
}

QDate DateWidget::date() const
{
    return d->mDate;
}

void DateWidget::showDatePicker()
{
    d->mDatePicker->setDate(d->mDate);
    d->mDatePicker->adjustSize();
    const QPoint pos = mapToGlobal(QPoint(0, -d->mDatePicker->height()));
    d->mDatePicker->move(pos);
    d->mDatePicker->show();
}

void DateWidget::slotDatePickerModified(const QDate& date)
{
    d->mDatePicker->hide();
    d->mDate = date;
    emit dateChanged(date);

    d->updateButton();
}

void DateWidget::goToPrevious()
{
    d->adjustDate(-1);
}

void DateWidget::goToNext()
{
    d->adjustDate(1);
}

} // namespace
