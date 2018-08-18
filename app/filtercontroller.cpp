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
#include "filtercontroller.h"

#include <config-gwenview.h>

// Qt
#include <QAction>
#include <QCompleter>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QToolButton>
#include <QDebug>
#include <QIcon>

// KDE
#include <KComboBox>
#include <KIconLoader>
#include <KLocalizedString>

// Local
#include <lib/datewidget.h>
#include <lib/flowlayout.h>
#include <lib/paintutils.h>

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
// KDE
#include <kratingwidget.h>

// Local
#include <lib/semanticinfo/abstractsemanticinfobackend.h>
#include <lib/semanticinfo/tagmodel.h>
#endif

namespace Gwenview
{

NameFilterWidget::NameFilterWidget(SortedDirModel* model)
{
    mFilter = new NameFilter(model);

    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Name contains"), QVariant(NameFilter::Contains));
    mModeComboBox->addItem(i18n("Name does not contain"), QVariant(NameFilter::DoesNotContain));

    mLineEdit = new QLineEdit;

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);
    layout->addWidget(mModeComboBox);
    layout->addWidget(mLineEdit);

    QTimer* timer = new QTimer(this);
    timer->setInterval(350);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), SLOT(applyNameFilter()));

    connect(mLineEdit, SIGNAL(textChanged(QString)),
            timer, SLOT(start()));

    connect(mModeComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(applyNameFilter()));

    QTimer::singleShot(0, mLineEdit, SLOT(setFocus()));
}

NameFilterWidget::~NameFilterWidget()
{
    delete mFilter;
}

void NameFilterWidget::applyNameFilter()
{
    QVariant data = mModeComboBox->itemData(mModeComboBox->currentIndex());
    mFilter->setMode(NameFilter::Mode(data.toInt()));
    mFilter->setText(mLineEdit->text());
}

DateFilterWidget::DateFilterWidget(SortedDirModel* model)
{
    mFilter = new DateFilter(model);

    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Date >="), DateFilter::GreaterOrEqual);
    mModeComboBox->addItem(i18n("Date ="),  DateFilter::Equal);
    mModeComboBox->addItem(i18n("Date <="), DateFilter::LessOrEqual);

    mDateWidget = new DateWidget;

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(mModeComboBox);
    layout->addWidget(mDateWidget);

    connect(mDateWidget, SIGNAL(dateChanged(QDate)),
            SLOT(applyDateFilter()));
    connect(mModeComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(applyDateFilter()));

    applyDateFilter();
}

DateFilterWidget::~DateFilterWidget()
{
    delete mFilter;
}

void DateFilterWidget::applyDateFilter()
{
    QVariant data = mModeComboBox->itemData(mModeComboBox->currentIndex());
    mFilter->setMode(DateFilter::Mode(data.toInt()));
    mFilter->setDate(mDateWidget->date());
}

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
RatingFilterWidget::RatingFilterWidget(SortedDirModel* model)
{
    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Rating >="), RatingFilter::GreaterOrEqual);
    mModeComboBox->addItem(i18n("Rating =") , RatingFilter::Equal);
    mModeComboBox->addItem(i18n("Rating <="), RatingFilter::LessOrEqual);

    mRatingWidget = new KRatingWidget;
    mRatingWidget->setHalfStepsEnabled(true);
    mRatingWidget->setMaxRating(10);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(mModeComboBox);
    layout->addWidget(mRatingWidget);

    mFilter = new RatingFilter(model);

    QObject::connect(mModeComboBox, SIGNAL(currentIndexChanged(int)),
                     SLOT(updateFilterMode()));

    QObject::connect(mRatingWidget, SIGNAL(ratingChanged(int)),
                     SLOT(slotRatingChanged(int)));

    updateFilterMode();
}

RatingFilterWidget::~RatingFilterWidget()
{
    delete mFilter;
}

void RatingFilterWidget::slotRatingChanged(int value)
{
    mFilter->setRating(value);
}

void RatingFilterWidget::updateFilterMode()
{
    QVariant data = mModeComboBox->itemData(mModeComboBox->currentIndex());
    mFilter->setMode(RatingFilter::Mode(data.toInt()));
}


TagFilterWidget::TagFilterWidget(SortedDirModel* model)
{
    mFilter = new TagFilter(model);

    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Tagged"), QVariant(true));
    mModeComboBox->addItem(i18n("Not Tagged"), QVariant(false));

    mTagComboBox = new QComboBox;

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(mModeComboBox);
    layout->addWidget(mTagComboBox);

    AbstractSemanticInfoBackEnd* backEnd = model->semanticInfoBackEnd();
    backEnd->refreshAllTags();
    TagModel* tagModel = TagModel::createAllTagsModel(this, backEnd);

    QCompleter* completer = new QCompleter(mTagComboBox);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setModel(tagModel);
    mTagComboBox->setCompleter(completer);
    mTagComboBox->setInsertPolicy(QComboBox::NoInsert);
    mTagComboBox->setEditable(true);
    mTagComboBox->setModel(tagModel);
    mTagComboBox->setCurrentIndex(-1);

    connect(mTagComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateTagSetFilter()));

    connect(mModeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateTagSetFilter()));

    QTimer::singleShot(0, mTagComboBox, SLOT(setFocus()));
}

TagFilterWidget::~TagFilterWidget()
{
    delete mFilter;
}

void TagFilterWidget::updateTagSetFilter()
{
    QModelIndex index = mTagComboBox->model()->index(mTagComboBox->currentIndex(), 0);
    if (!index.isValid()) {
        qWarning() << "Invalid index";
        return;
    }
    SemanticInfoTag tag = index.data(TagModel::TagRole).toString();
    mFilter->setTag(tag);

    bool wantMatchingTag = mModeComboBox->itemData(mModeComboBox->currentIndex()).toBool();
    mFilter->setWantMatchingTag(wantMatchingTag);
}
#endif

/**
 * A container for all filter widgets. It features a close button on the right.
 */
class FilterWidgetContainer : public QFrame
{
public:
    FilterWidgetContainer()
    {
        QPalette pal = palette();
        pal.setColor(QPalette::Window, pal.color(QPalette::Highlight));
        setPalette(pal);
    }

    void setFilterWidget(QWidget* widget)
    {
        QToolButton* closeButton = new QToolButton;
        closeButton->setIcon(QIcon::fromTheme("window-close"));
        closeButton->setAutoRaise(true);
        closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        int size = IconSize(KIconLoader::Small);
        closeButton->setIconSize(QSize(size, size));
        connect(closeButton, SIGNAL(clicked()), SLOT(deleteLater()));
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setMargin(2);
        layout->setSpacing(2);
        layout->addWidget(widget);
        layout->addWidget(closeButton);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path = PaintUtils::roundedRectangle(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 6);

        QColor color = palette().color(QPalette::Highlight);
        painter.fillPath(path, PaintUtils::alphaAdjustedF(color, 0.5));
        painter.setPen(color);
        painter.drawPath(path);
    }
};


FilterController::FilterController(QFrame* frame, SortedDirModel* dirModel)
: QObject(frame)
{
    q = this;
    mFrame = frame;
    mDirModel = dirModel;
    mFilterWidgetCount = 0;

    mFrame->hide();
    FlowLayout* layout = new FlowLayout(mFrame);
    layout->setSpacing(2);

    addAction(i18nc("@action:inmenu", "Filter by Name"), SLOT(addFilterByName()));
    addAction(i18nc("@action:inmenu", "Filter by Date"), SLOT(addFilterByDate()));
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    addAction(i18nc("@action:inmenu", "Filter by Rating"), SLOT(addFilterByRating()));
    addAction(i18nc("@action:inmenu", "Filter by Tag"), SLOT(addFilterByTag()));
#endif
}

QList<QAction*> FilterController::actionList() const
{
    return mActionList;
}

void FilterController::addFilterByName()
{
    addFilter(new NameFilterWidget(mDirModel));
}

void FilterController::addFilterByDate()
{
    addFilter(new DateFilterWidget(mDirModel));
}

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
void FilterController::addFilterByRating()
{
    addFilter(new RatingFilterWidget(mDirModel));
}

void FilterController::addFilterByTag()
{
    addFilter(new TagFilterWidget(mDirModel));
}
#endif

void FilterController::slotFilterWidgetClosed()
{
    mFilterWidgetCount--;
    if (mFilterWidgetCount == 0) {
        mFrame->hide();
    }
}


void FilterController::addAction(const QString& text, const char* slot)
{
    QAction* action = new QAction(text, q);
    QObject::connect(action, SIGNAL(triggered()), q, slot);
    mActionList << action;
}


void FilterController::addFilter(QWidget* widget)
{
    if (mFrame->isHidden()) {
        mFrame->show();
    }
    FilterWidgetContainer* container = new FilterWidgetContainer;
    container->setFilterWidget(widget);
    mFrame->layout()->addWidget(container);

    mFilterWidgetCount++;
    QObject::connect(container, SIGNAL(destroyed()), q, SLOT(slotFilterWidgetClosed()));
}

} // namespace
