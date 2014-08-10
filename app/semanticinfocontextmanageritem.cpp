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
#include <QSignalMapper>
#include <QStyle>
#include <QTimer>

// KDE
#include <QAction>
#include <KActionCategory>
#include <KActionCollection>
#include <QDebug>
#include <KDialog>
#include <KLocale>
#include <KRatingPainter>

// Local
#include "viewmainpage.h"
#include "sidebar.h"
#include "ui_semanticinfosidebaritem.h"
#include "ui_semanticinfodialog.h"
#include <lib/contextmanager.h>
#include <lib/documentview/documentview.h>
#include <lib/eventwatcher.h>
#include <lib/hud/hudwidget.h>
#include <lib/signalblocker.h>
#include <lib/widgetfloater.h>
#include <lib/semanticinfo/abstractsemanticinfobackend.h>
#include <lib/semanticinfo/semanticinfodirmodel.h>
#include <lib/semanticinfo/sorteddirmodel.h>

namespace Gwenview
{

static const int RATING_INDICATOR_HIDE_DELAY = 2000;

struct SemanticInfoDialog : public KDialog, public Ui_SemanticInfoDialog
{
    SemanticInfoDialog(QWidget* parent)
    : KDialog(parent)
    {
        setButtons(None);
        QWidget* mainWidget = new QWidget;
        setMainWidget(mainWidget);
        setupUi(mainWidget);
        mainWidget->layout()->setMargin(0);
        setWindowTitle(mainWidget->windowTitle());

        restoreDialogSize(configGroup());
    }

    ~SemanticInfoDialog()
    {
        KConfigGroup group = configGroup();
        saveDialogSize(group);
    }

    KConfigGroup configGroup() const
    {
        KSharedConfigPtr config = KGlobal::config();
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

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
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
        connect(mDeleteTimer, SIGNAL(timeout()), SLOT(fadeOut()));
        connect(this, SIGNAL(fadedOut()), SLOT(deleteLater()));
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
    QSignalMapper* mRatingMapper;
    /** A list of all actions, so that we can disable them when necessary */
    QList<QAction *> mActions;
    QPointer<RatingIndicator> mRatingIndicator;

    void setupGroup()
    {
        mGroup = new SideBarGroup(i18n("Semantic Information"));
        q->setWidget(mGroup);
        EventWatcher::install(mGroup, QEvent::Show, q, SLOT(update()));

        QWidget* container = new QWidget;
        setupUi(container);
        container->layout()->setMargin(0);
        mGroup->addWidget(container);

        QObject::connect(mRatingWidget, SIGNAL(ratingChanged(int)),
                         q, SLOT(slotRatingChanged(int)));
        QObject::connect(mRatingMapper, SIGNAL(mapped(int)),
                         mRatingWidget, SLOT(setRating(int)));

        mDescriptionTextEdit->installEventFilter(q);

        QObject::connect(mTagLabel, SIGNAL(linkActivated(QString)),
                         mEditTagsAction, SLOT(trigger()));
    }

    void setupActions()
    {
        KActionCategory* edit = new KActionCategory(i18nc("@title actions category", "Edit"), mActionCollection);

        mEditTagsAction = edit->addAction("edit_tags");
        mEditTagsAction->setText(i18nc("@action", "Edit Tags"));
        mEditTagsAction->setShortcut(Qt::CTRL | Qt::Key_T);
        QObject::connect(mEditTagsAction, SIGNAL(triggered()),
                         q, SLOT(showSemanticInfoDialog()));
        mActions << mEditTagsAction;

        mRatingMapper = new QSignalMapper(q);
        for (int rating = 0; rating <= 5; ++rating) {
            QAction * action = edit->addAction(QString("rate_%1").arg(rating));
            if (rating == 0) {
                action->setText(i18nc("@action Rating value of zero", "Zero"));
            } else {
                action->setText(QString(rating, QChar(0x22C6))); /* 0x22C6 is the 'star' character */
            }
            action->setShortcut(Qt::Key_0 + rating);
            QObject::connect(action, SIGNAL(triggered()), mRatingMapper, SLOT(map()));
            mRatingMapper->setMapping(action, rating * 2);
            mActions << action;
        }
        QObject::connect(mRatingMapper, SIGNAL(mapped(int)), q, SLOT(slotRatingChanged(int)));
    }

    void updateTagLabel()
    {
        if (q->contextManager()->selectedFileItemList().isEmpty()) {
            mTagLabel->clear();
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

        QString editLink = i18n("Edit");
        QString text = labels.join(", ") + QString(" <a href='edit'>%1</a>").arg(editLink);
        mTagLabel->setText(text);
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

    connect(contextManager(), SIGNAL(selectionChanged()),
            SLOT(slotSelectionChanged()));
    connect(contextManager(), SIGNAL(selectionDataChanged()),
            SLOT(update()));
    connect(contextManager(), SIGNAL(currentDirUrlChanged(KUrl)),
            SLOT(update()));

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
    KFileItemList itemList = contextManager()->selectedFileItemList();

    bool first = true;
    int rating = 0;
    QString description;
    SortedDirModel* dirModel = contextManager()->dirModel();

    // This hash stores for how many items the tag is present
    // If you have 3 items, and only 2 have the "Holiday" tag,
    // then tagHash["Holiday"] will be 2 at the end of the loop.
    typedef QHash<QString, int> TagHash;
    TagHash tagHash;

    Q_FOREACH(const KFileItem & item, itemList) {
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
        TagSet tagSet = TagSet::fromVariant(index.data(SemanticInfoDirModel::TagsRole));
        Q_FOREACH(const QString & tag, tagSet) {
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
    Q_FOREACH(QAction * action, d->mActions) {
        action->setEnabled(enabled);
    }
    d->updateTagLabel();
    if (d->mSemanticInfoDialog) {
        d->updateSemanticInfoDialog();
    }
}

void SemanticInfoContextManagerItem::slotRatingChanged(int rating)
{
    KFileItemList itemList = contextManager()->selectedFileItemList();

    // Show rating indicator in view mode, and only if sidebar is not visible
    if (d->mViewMainPage->isVisible() && !d->mRatingWidget->isVisible()) {
        if (!d->mRatingIndicator.data()) {
            d->mRatingIndicator = new RatingIndicator;
            d->mViewMainPage->showMessageWidget(d->mRatingIndicator, Qt::AlignBottom | Qt::AlignHCenter);
        }
        d->mRatingIndicator->setRating(rating);
    }

    SortedDirModel* dirModel = contextManager()->dirModel();
    Q_FOREACH(const KFileItem & item, itemList) {
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
    KFileItemList itemList = contextManager()->selectedFileItemList();

    SortedDirModel* dirModel = contextManager()->dirModel();
    Q_FOREACH(const KFileItem & item, itemList) {
        QModelIndex index = dirModel->indexForItem(item);
        dirModel->setData(index, description, SemanticInfoDirModel::DescriptionRole);
    }
}

void SemanticInfoContextManagerItem::assignTag(const SemanticInfoTag& tag)
{
    KFileItemList itemList = contextManager()->selectedFileItemList();

    SortedDirModel* dirModel = contextManager()->dirModel();
    Q_FOREACH(const KFileItem & item, itemList) {
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
    KFileItemList itemList = contextManager()->selectedFileItemList();

    SortedDirModel* dirModel = contextManager()->dirModel();
    Q_FOREACH(const KFileItem & item, itemList) {
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

        connect(d->mSemanticInfoDialog->mPreviousButton, SIGNAL(clicked()),
                d->mActionCollection->action("go_previous"), SLOT(trigger()));
        connect(d->mSemanticInfoDialog->mNextButton, SIGNAL(clicked()),
                d->mActionCollection->action("go_next"), SLOT(trigger()));
        connect(d->mSemanticInfoDialog->mButtonBox, SIGNAL(rejected()),
                d->mSemanticInfoDialog, SLOT(close()));

        AbstractSemanticInfoBackEnd* backEnd = contextManager()->dirModel()->semanticInfoBackEnd();
        d->mSemanticInfoDialog->mTagWidget->setSemanticInfoBackEnd(backEnd);
        connect(d->mSemanticInfoDialog->mTagWidget, SIGNAL(tagAssigned(SemanticInfoTag)),
                SLOT(assignTag(SemanticInfoTag)));
        connect(d->mSemanticInfoDialog->mTagWidget, SIGNAL(tagRemoved(SemanticInfoTag)),
                SLOT(removeTag(SemanticInfoTag)));
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
