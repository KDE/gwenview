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
#include "semanticinfocontextmanageritem.h"

// Qt
#include <QEvent>
#include <QPainter>
#include <QShortcut>
#include <QStyle>
#include <QTimer>
#include <QAction>
#include <QVBoxLayout>
#include <QDialog>

// KF
#include <KActionCategory>
#include <KActionCollection>
#include <KLocalizedString>
#include <KRatingPainter>
#include <KIconLoader>
#include <KSharedConfig>
#include <KWindowConfig>

// Local
#include "viewmainpage.h"
#include "sidebar.h"
#include "ui_semanticinfosidebaritem.h"
#include "ui_semanticinfodialog.h"
#include <lib/contextmanager.h>
#include <lib/decoratedtag/decoratedtag.h>
#include <lib/documentview/documentview.h>
#include <lib/eventwatcher.h>
#include <lib/flowlayout.h>
#include <lib/hud/hudwidget.h>
#include <lib/signalblocker.h>
#include <lib/widgetfloater.h>
#include <lib/semanticinfo/semanticinfodirmodel.h>
#include <lib/semanticinfo/sorteddirmodel.h>

namespace Gwenview
{

static const int RATING_INDICATOR_HIDE_DELAY = 2000;

struct SemanticInfoDialog : public QDialog, public Ui_SemanticInfoDialog
{
    SemanticInfoDialog(QWidget* parent)
    : QDialog(parent)
    {
        setLayout(new QVBoxLayout);
        auto* mainWidget = new QWidget;
        layout()->addWidget(mainWidget);
        setupUi(mainWidget);
        mainWidget->layout()->setContentsMargins(0, 0, 0, 0);
        setWindowTitle(mainWidget->windowTitle());

        KWindowConfig::restoreWindowSize(windowHandle(), configGroup());
    }

    ~SemanticInfoDialog() override
    {
        KConfigGroup group = configGroup();
        KWindowConfig::saveWindowSize(windowHandle(), group);
    }

    KConfigGroup configGroup() const
    {
        KSharedConfigPtr config = KSharedConfig::openConfig();
        return KConfigGroup(config, "SemanticInfoDialog");
    }
};

/**
 * A QGraphicsPixmapItem-like class, but which inherits from QGraphicsWidget
 */
class GraphicsPixmapWidget : public QGraphicsWidget
{
public:
    void setPixmap(const QPixmap& pix)
    {
        mPix = pix;
        setMinimumSize(pix.size());
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override
    {
        painter->drawPixmap(
            (size().width() - mPix.width()) / 2,
            (size().height() - mPix.height()) / 2,
            mPix);
    }

private:
    QPixmap mPix;
};

class RatingIndicator : public HudWidget
{
public:
    RatingIndicator()
    : HudWidget()
    , mPixmapWidget(new GraphicsPixmapWidget)
    , mDeleteTimer(new QTimer(this))
    {
        updatePixmap(0);
        setOpacity(0);
        init(mPixmapWidget, OptionNone);

        mDeleteTimer->setInterval(RATING_INDICATOR_HIDE_DELAY);
        mDeleteTimer->setSingleShot(true);
        connect(mDeleteTimer, &QTimer::timeout, this, &HudWidget::fadeOut);
        connect(this, &HudWidget::fadedOut, this, &QObject::deleteLater);
    }

    void setRating(int rating)
    {
        updatePixmap(rating);
        update();
        mDeleteTimer->start();
        fadeIn();
    }

private:
    GraphicsPixmapWidget* mPixmapWidget;
    QTimer* mDeleteTimer;

    void updatePixmap(int rating)
    {
        KRatingPainter ratingPainter;
        const int iconSize = KIconLoader::global()->currentSize(KIconLoader::Small);
        QPixmap pix(iconSize * 5 + ratingPainter.spacing() * 4, iconSize);
        pix.fill(Qt::transparent);
        {
            QPainter painter(&pix);
            ratingPainter.paint(&painter, pix.rect(), rating);
        }
        mPixmapWidget->setPixmap(pix);
    }
};

struct SemanticInfoContextManagerItemPrivate : public Ui_SemanticInfoSideBarItem
{
    SemanticInfoContextManagerItem* q;
    SideBarGroup* mGroup;
    KActionCollection* mActionCollection;
    ViewMainPage* mViewMainPage;
    QPointer<SemanticInfoDialog> mSemanticInfoDialog;
    TagInfo mTagInfo;
    QAction * mEditTagsAction;
    /** A list of all actions, so that we can disable them when necessary */
    QList<QAction *> mActions;
    QPointer<RatingIndicator> mRatingIndicator;
    FlowLayout *mTagLayout;
    QLabel *mEditLabel;

    void setupGroup()
    {
        mGroup = new SideBarGroup();
        q->setWidget(mGroup);
        EventWatcher::install(mGroup, QEvent::Show, q, SLOT(update()));

        auto* container = new QWidget;
        setupUi(container);
        container->layout()->setContentsMargins(0, 0, 0, 0);
        mGroup->addWidget(container);
        mTagLayout = new FlowLayout;
        mTagLayout->setHorizontalSpacing(2);
        mTagLayout->setVerticalSpacing(2);
        mTagLayout->setContentsMargins(0,0,0,0);
        mTagContainerWidget->setLayout(mTagLayout);
        DecoratedTag tempTag;
        tempTag.setVisible(false);
        mEditLabel = new QLabel(QStringLiteral("<a href='edit'>%1</a>").arg(i18n("Edit")));
        mEditLabel->setVisible(false);
        mEditLabel->setContentsMargins(tempTag.contentsMargins().left() / 2,
                                       tempTag.contentsMargins().top(),
                                       tempTag.contentsMargins().right() / 2,
                                       tempTag.contentsMargins().bottom());
        label_2->setContentsMargins(mEditLabel->contentsMargins());

        QObject::connect(mRatingWidget, SIGNAL(ratingChanged(int)),
                         q, SLOT(slotRatingChanged(int)));

        mDescriptionTextEdit->installEventFilter(q);

        QObject::connect(mEditLabel, &QLabel::linkActivated,
                         mEditTagsAction, &QAction::trigger);
    }

    void setupActions()
    {
        auto* edit = new KActionCategory(i18nc("@title actions category", "Edit"), mActionCollection);

        mEditTagsAction = edit->addAction("edit_tags");
        mEditTagsAction->setText(i18nc("@action", "Edit Tags"));
        mEditTagsAction->setIcon(QIcon::fromTheme("tag"));
        mActionCollection->setDefaultShortcut(mEditTagsAction, Qt::CTRL | Qt::Key_T);
        QObject::connect(mEditTagsAction, &QAction::triggered,
                         q, &SemanticInfoContextManagerItem::showSemanticInfoDialog);
        mActions << mEditTagsAction;

        for (int rating = 0; rating <= 5; ++rating) {
            QAction * action = edit->addAction(QStringLiteral("rate_%1").arg(rating));
            if (rating == 0) {
                action->setText(i18nc("@action Rating value of zero", "Zero"));
            } else {
                action->setText(QString(rating, QChar(0x22C6))); /* 0x22C6 is the 'star' character */
            }
            mActionCollection->setDefaultShortcut(action, Qt::Key_0 + rating);
            QObject::connect(action, &QAction::triggered, q, [this, rating]() {
                mRatingWidget->setRating(rating * 2);
            });
            mActions << action;
        }
    }

    void updateTags()
    {
        QLayoutItem *item;
        while ((item = mTagLayout->takeAt(0))) {
            auto tag = item->widget();
            if (tag != nullptr && tag != mEditLabel) {
                tag->deleteLater();
            }
        }
        if (q->contextManager()->selectedFileItemList().isEmpty()) {
            mEditLabel->setVisible(false);
            return;
        }

        AbstractSemanticInfoBackEnd* backEnd = q->contextManager()->dirModel()->semanticInfoBackEnd();

        TagInfo::ConstIterator
        it = mTagInfo.constBegin(),
        end = mTagInfo.constEnd();
        QMap<QString, QString> labelMap;
        for (; it != end; ++it) {
            SemanticInfoTag tag = it.key();
            QString label = backEnd->labelForTag(tag);
            if (!it.value()) {
                // Tag is not present for all urls
                label += '*';
            }
            labelMap[label.toLower()] = label;
        }
        QStringList labels(labelMap.values());

        for (const QString &label : labels) {
            DecoratedTag *decoratedTag = new DecoratedTag(label);
            mTagLayout->addWidget(decoratedTag);
        }
        mTagLayout->addWidget(mEditLabel);
        mEditLabel->setVisible(true);
        mTagLayout->update();
    }

    void updateSemanticInfoDialog()
    {
        mSemanticInfoDialog->mTagWidget->setEnabled(!q->contextManager()->selectedFileItemList().isEmpty());
        mSemanticInfoDialog->mTagWidget->setTagInfo(mTagInfo);
    }
};

SemanticInfoContextManagerItem::SemanticInfoContextManagerItem(ContextManager* manager, KActionCollection* actionCollection, ViewMainPage* viewMainPage)
: AbstractContextManagerItem(manager)
, d(new SemanticInfoContextManagerItemPrivate)
{
    d->q = this;
    d->mActionCollection = actionCollection;
    d->mViewMainPage = viewMainPage;

    connect(contextManager(), &ContextManager::selectionChanged,
            this, &SemanticInfoContextManagerItem::slotSelectionChanged);
    connect(contextManager(), &ContextManager::selectionDataChanged,
            this, &SemanticInfoContextManagerItem::update);
    connect(contextManager(), &ContextManager::currentDirUrlChanged,
            this, &SemanticInfoContextManagerItem::update);

    d->setupActions();
    d->setupGroup();
}

SemanticInfoContextManagerItem::~SemanticInfoContextManagerItem()
{
    delete d;
}

inline int ratingForVariant(const QVariant& variant)
{
    if (variant.isValid()) {
        return variant.toInt();
    } else {
        return 0;
    }
}

void SemanticInfoContextManagerItem::slotSelectionChanged()
{
    update();
}

void SemanticInfoContextManagerItem::update()
{
    const KFileItemList itemList = contextManager()->selectedFileItemList();

    bool first = true;
    int rating = 0;
    QString description;
    SortedDirModel* dirModel = contextManager()->dirModel();

    // This hash stores for how many items the tag is present
    // If you have 3 items, and only 2 have the "Holiday" tag,
    // then tagHash["Holiday"] will be 2 at the end of the loop.
    using TagHash = QHash<QString, int>;
    TagHash tagHash;

    for (const KFileItem & item : itemList) {
        QModelIndex index = dirModel->indexForItem(item);

        QVariant value = dirModel->data(index, SemanticInfoDirModel::RatingRole);
        if (first) {
            rating = ratingForVariant(value);
        } else if (rating != ratingForVariant(value)) {
            // Ratings aren't the same, reset
            rating = 0;
        }

        QString indexDescription = index.data(SemanticInfoDirModel::DescriptionRole).toString();
        if (first) {
            description = indexDescription;
        } else if (description != indexDescription) {
            description.clear();
        }

        // Fill tagHash, incrementing the tag count if it's already there
        const TagSet tagSet = TagSet::fromVariant(index.data(SemanticInfoDirModel::TagsRole));
        for (const QString & tag : tagSet) {
            TagHash::Iterator it = tagHash.find(tag);
            if (it == tagHash.end()) {
                tagHash[tag] = 1;
            } else {
                ++it.value();
            }
        }

        first = false;
    }
    {
        SignalBlocker blocker(d->mRatingWidget);
        d->mRatingWidget->setRating(rating);
    }
    d->mDescriptionTextEdit->setText(description);

    // Init tagInfo from tagHash
    d->mTagInfo.clear();
    int itemCount = itemList.count();
    TagHash::ConstIterator
    it = tagHash.constBegin(),
    end = tagHash.constEnd();
    for (; it != end; ++it) {
        QString tag = it.key();
        int count = it.value();
        d->mTagInfo[tag] = count == itemCount;
    }

    bool enabled = !contextManager()->selectedFileItemList().isEmpty();
    for (QAction * action : qAsConst(d->mActions)) {
        action->setEnabled(enabled);
    }
    d->updateTags();
    if (d->mSemanticInfoDialog) {
        d->updateSemanticInfoDialog();
    }
}

void SemanticInfoContextManagerItem::slotRatingChanged(int rating)
{
    const KFileItemList itemList = contextManager()->selectedFileItemList();

    // Show rating indicator in view mode, and only if sidebar is not visible
    if (d->mViewMainPage->isVisible() && !d->mRatingWidget->isVisible()) {
        if (!d->mRatingIndicator.data()) {
            d->mRatingIndicator = new RatingIndicator;
            d->mViewMainPage->showMessageWidget(d->mRatingIndicator, Qt::AlignBottom | Qt::AlignHCenter);
        }
        d->mRatingIndicator->setRating(rating);
    }

    SortedDirModel* dirModel = contextManager()->dirModel();
    for (const KFileItem & item : itemList) {
        QModelIndex index = dirModel->indexForItem(item);
        dirModel->setData(index, rating, SemanticInfoDirModel::RatingRole);
    }
}

void SemanticInfoContextManagerItem::storeDescription()
{
    if (!d->mDescriptionTextEdit->document()->isModified()) {
        return;
    }
    d->mDescriptionTextEdit->document()->setModified(false);
    QString description = d->mDescriptionTextEdit->toPlainText();
    const KFileItemList itemList = contextManager()->selectedFileItemList();

    SortedDirModel* dirModel = contextManager()->dirModel();
    for (const KFileItem & item : itemList) {
        QModelIndex index = dirModel->indexForItem(item);
        dirModel->setData(index, description, SemanticInfoDirModel::DescriptionRole);
    }
}

void SemanticInfoContextManagerItem::assignTag(const SemanticInfoTag& tag)
{
    const KFileItemList itemList = contextManager()->selectedFileItemList();

    SortedDirModel* dirModel = contextManager()->dirModel();
    for (const KFileItem & item : itemList) {
        QModelIndex index = dirModel->indexForItem(item);
        TagSet tags = TagSet::fromVariant(dirModel->data(index, SemanticInfoDirModel::TagsRole));
        if (!tags.contains(tag)) {
            tags << tag;
            dirModel->setData(index, tags.toVariant(), SemanticInfoDirModel::TagsRole);
        }
    }
}

void SemanticInfoContextManagerItem::removeTag(const SemanticInfoTag& tag)
{
    const KFileItemList itemList = contextManager()->selectedFileItemList();

    SortedDirModel* dirModel = contextManager()->dirModel();
    for (const KFileItem & item : itemList) {
        QModelIndex index = dirModel->indexForItem(item);
        TagSet tags = TagSet::fromVariant(dirModel->data(index, SemanticInfoDirModel::TagsRole));
        if (tags.contains(tag)) {
            tags.remove(tag);
            dirModel->setData(index, tags.toVariant(), SemanticInfoDirModel::TagsRole);
        }
    }
}

void SemanticInfoContextManagerItem::showSemanticInfoDialog()
{
    if (!d->mSemanticInfoDialog) {
        d->mSemanticInfoDialog = new SemanticInfoDialog(d->mGroup);
        d->mSemanticInfoDialog->setAttribute(Qt::WA_DeleteOnClose, true);

        connect(d->mSemanticInfoDialog->mPreviousButton, &QAbstractButton::clicked,
                d->mActionCollection->action(QStringLiteral("go_previous")), &QAction::trigger);
        connect(d->mSemanticInfoDialog->mNextButton, &QAbstractButton::clicked,
                d->mActionCollection->action(QStringLiteral("go_next")), &QAction::trigger);
        connect(d->mSemanticInfoDialog->mButtonBox, &QDialogButtonBox::rejected,
                d->mSemanticInfoDialog.data(), &QWidget::close);

        AbstractSemanticInfoBackEnd* backEnd = contextManager()->dirModel()->semanticInfoBackEnd();
        d->mSemanticInfoDialog->mTagWidget->setSemanticInfoBackEnd(backEnd);
        connect(d->mSemanticInfoDialog->mTagWidget, &TagWidget::tagAssigned,
                this, &SemanticInfoContextManagerItem::assignTag);
        connect(d->mSemanticInfoDialog->mTagWidget, &TagWidget::tagRemoved,
                this, &SemanticInfoContextManagerItem::removeTag);
    }
    d->updateSemanticInfoDialog();
    d->mSemanticInfoDialog->show();
}

bool SemanticInfoContextManagerItem::eventFilter(QObject*, QEvent* event)
{
    if (event->type() == QEvent::FocusOut) {
        storeDescription();
    }
    return false;
}

} // namespace
