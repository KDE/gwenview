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

// Qt
#include <QAction>
#include <QCompleter>
#include <QIcon>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QToolButton>

// KF
#include <KComboBox>
#include <KIconLoader>
#include <KLocalizedString>

// Local
#include "gwenview_app_debug.h"
#include <chrono>
#include <lib/flowlayout.h>
#include <lib/paintutils.h>

using namespace std::chrono_literals;

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
// KF

// Local
#endif

namespace Gwenview
{
NameFilterWidget::NameFilterWidget(SortedDirModel *model)
    : mFilter(new NameFilter(model))
{
    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Name contains"), QVariant(NameFilter::Contains));
    mModeComboBox->addItem(i18n("Name does not contain"), QVariant(NameFilter::DoesNotContain));

    mLineEdit = new QLineEdit;

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addWidget(mModeComboBox);
    layout->addWidget(mLineEdit);

    auto timer = new QTimer(this);
    timer->setInterval(350ms);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &NameFilterWidget::applyNameFilter);

    connect(mLineEdit, SIGNAL(textChanged(QString)), timer, SLOT(start()));

    connect(mModeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(applyNameFilter()));

    QTimer::singleShot(0, mLineEdit, SLOT(setFocus()));
}

NameFilterWidget::~NameFilterWidget()
{
    delete mFilter;
}

void NameFilterWidget::applyNameFilter()
{
    const QVariant data = mModeComboBox->itemData(mModeComboBox->currentIndex());
    mFilter->setMode(NameFilter::Mode(data.toInt()));
    mFilter->setText(mLineEdit->text());
}

DateFilterWidget::DateFilterWidget(SortedDirModel *model)
    : mFilter(new DateFilter(model))
{
    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Date >="), DateFilter::GreaterOrEqual);
    mModeComboBox->addItem(i18n("Date ="), DateFilter::Equal);
    mModeComboBox->addItem(i18n("Date <="), DateFilter::LessOrEqual);

    mDateWidget = new DateWidget;

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mModeComboBox);
    layout->addWidget(mDateWidget);

    connect(mDateWidget, &DateWidget::dateChanged, this, &DateFilterWidget::applyDateFilter);
    connect(mModeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(applyDateFilter()));

    applyDateFilter();
}

DateFilterWidget::~DateFilterWidget()
{
    delete mFilter;
}

void DateFilterWidget::applyDateFilter()
{
    const QVariant data = mModeComboBox->itemData(mModeComboBox->currentIndex());
    mFilter->setMode(DateFilter::Mode(data.toInt()));
    mFilter->setDate(mDateWidget->date());
}

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
RatingFilterWidget::RatingFilterWidget(SortedDirModel *model)
{
    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Rating >="), RatingFilter::GreaterOrEqual);
    mModeComboBox->addItem(i18n("Rating ="), RatingFilter::Equal);
    mModeComboBox->addItem(i18n("Rating <="), RatingFilter::LessOrEqual);

    mRatingWidget = new KRatingWidget;
    mRatingWidget->setHalfStepsEnabled(true);
    mRatingWidget->setMaxRating(10);

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mModeComboBox);
    layout->addWidget(mRatingWidget);

    mFilter = new RatingFilter(model);

    QObject::connect(mModeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateFilterMode()));

    QObject::connect(mRatingWidget, SIGNAL(ratingChanged(int)), SLOT(slotRatingChanged(int)));

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
    const QVariant data = mModeComboBox->itemData(mModeComboBox->currentIndex());
    mFilter->setMode(RatingFilter::Mode(data.toInt()));
}

TagFilterWidget::TagFilterWidget(SortedDirModel *model)
    : mFilter(new TagFilter(model))
{
    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Tagged"), QVariant(true));
    mModeComboBox->addItem(i18n("Not Tagged"), QVariant(false));

    mTagComboBox = new QComboBox;

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mModeComboBox);
    layout->addWidget(mTagComboBox);

    AbstractSemanticInfoBackEnd *backEnd = model->semanticInfoBackEnd();
    backEnd->refreshAllTags();
    TagModel *tagModel = TagModel::createAllTagsModel(this, backEnd);

    auto completer = new QCompleter(mTagComboBox);
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
        qCWarning(GWENVIEW_APP_LOG) << "Invalid index";
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

    void setFilterWidget(QWidget *widget)
    {
        auto closeButton = new QToolButton;
        closeButton->setIcon(QIcon::fromTheme(QStringLiteral("window-close")));
        closeButton->setAutoRaise(true);
        closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        int size = style()->pixelMetric(QStyle::PM_SmallIconSize);
        closeButton->setIconSize(QSize(size, size));
        connect(closeButton, &QAbstractButton::clicked, this, &QObject::deleteLater);
        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(2);
        layout->addWidget(widget);
        layout->addWidget(closeButton);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QPainterPath path = PaintUtils::roundedRectangle(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 6);

        const QColor color = palette().color(QPalette::Highlight);
        painter.fillPath(path, PaintUtils::alphaAdjustedF(color, 0.5));
        painter.setPen(color);
        painter.drawPath(path);
    }
};

FilterController::FilterController(QFrame *frame, SortedDirModel *dirModel)
    : QObject(frame)
{
    q = this;
    mFrame = frame;
    mDirModel = dirModel;
    mFilterWidgetCount = 0;

    mFrame->hide();
    auto layout = new FlowLayout(mFrame);
    layout->setSpacing(2);

    addAction(i18nc("@action:inmenu", "Filter by Name"), SLOT(addFilterByName()), QKeySequence(Qt::CTRL | Qt::Key_I));

    addAction(i18nc("@action:inmenu", "Filter by Date"), SLOT(addFilterByDate()), QKeySequence());

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    addAction(i18nc("@action:inmenu", "Filter by Rating"), SLOT(addFilterByRating()), QKeySequence());

    addAction(i18nc("@action:inmenu", "Filter by Tag"), SLOT(addFilterByTag()), QKeySequence());
#endif
}

QList<QAction *> FilterController::actionList() const
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

void FilterController::addAction(const QString &text, const char *slot, const QKeySequence &shortcut)
{
    auto action = new QAction(text, q);
    action->setShortcut(shortcut);
    QObject::connect(action, SIGNAL(triggered()), q, slot);
    mActionList << action;
}

void FilterController::addFilter(QWidget *widget)
{
    if (mFrame->isHidden()) {
        mFrame->show();
    }
    auto container = new FilterWidgetContainer;
    container->setFilterWidget(widget);
    mFrame->layout()->addWidget(container);

    mFilterWidgetCount++;
    QObject::connect(container, &QObject::destroyed, q, &FilterController::slotFilterWidgetClosed);
}

} // namespace
