/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
#include "infocontextmanageritem.moc"

// Qt
#include <QApplication>
#include <QGridLayout>
#include <QHelpEvent>
#include <QLabel>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QToolTip>
#include <QVBoxLayout>

// KDE
#include <KFileItem>
#include <KLocale>
#include <KSqueezedTextLabel>

// Local
#include "imagemetainfodialog.h"
#include "sidebar.h"
#include <lib/archiveutils.h>
#include <lib/contextmanager.h>
#include <lib/eventwatcher.h>
#include <lib/gwenviewconfig.h>
#include <lib/preferredimagemetainfomodel.h>
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>

namespace Gwenview
{

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

/**
 * A label which fades out if its content does not fit. If the content is
 * cropped, a tooltip is shown when the mouse hovers the widget.
 */
class FadingLabel : public QLabel
{
public:
    explicit FadingLabel(QWidget* parent = 0)
    : QLabel(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setTextInteractionFlags(Qt::TextBrowserInteraction);
    }

    QSize minimumSizeHint() const
    {
        return QSize();
    }

protected:
    void paintEvent(QPaintEvent* event)
    {
        QLabel::paintEvent(event);
        if (!isCropped()) {
            return;
        }

        QLinearGradient gradient;
        int gradientWidth = fontMetrics().averageCharWidth() * 4;
        if (alignment() & Qt::AlignLeft) {
            gradient.setStart(width() - gradientWidth, 0);
            gradient.setFinalStop(width(), 0);
            gradient.setColorAt(0, Qt::transparent);
            gradient.setColorAt(1, palette().color(backgroundRole()));
        } else {
            gradient.setStart(0, 0);
            gradient.setFinalStop(gradientWidth, 0);
            gradient.setColorAt(0, palette().color(backgroundRole()));
            gradient.setColorAt(1, Qt::transparent);
        }
        QPainter painter(this);
        painter.fillRect(rect(), gradient);
    }

    bool event(QEvent* event)
    {
        if (event->type() == QEvent::ToolTip) {
            // Show tooltip if cropped
            QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
            if (isCropped()) {
                QToolTip::showText(helpEvent->globalPos(), text());
            } else {
                QToolTip::hideText();
                event->ignore();
            }
            return true;
        }
        return QLabel::event(event);
    }

    inline bool isCropped() const
 {
        return sizeHint().width() > width();
    }
};

/**
 * This widget is capable of showing multiple lines of key/value pairs.
 */
class KeyValueWidget : public QWidget
{
    struct Row
    {
        Row(QWidget* parent)
        : keyLabel(new FadingLabel(parent))
        , valueLabel(new FadingLabel(parent))
        {
            keyLabel->show();
            valueLabel->show();
            if (QApplication::isLeftToRight()) {
                keyLabel->setAlignment(Qt::AlignRight);
            } else {
                valueLabel->setAlignment(Qt::AlignRight);
            }
        }

        ~Row()
        {
            delete keyLabel;
            delete valueLabel;
        }

        FadingLabel* keyLabel;
        FadingLabel* valueLabel;
    };
public:
    KeyValueWidget(QWidget* parent)
    : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    QSize sizeHint() const
    {
        int height = fontMetrics().height() * mRows.count();
        return QSize(150, height);
    }

    void clear()
    {
        qDeleteAll(mRows);
        mRows.clear();
        updateGeometry();
    }

    void addRow(const QString& key, const QString& value)
    {
        Row* row = new Row(this);
        row->keyLabel->setText(i18nc(
           "@item:intext %1 is a key, we append a colon to it. A value is displayed after",
           "%1:", key));
        row->valueLabel->setText(value);
        mRows << row;
        updateGeometry();
    }

    void layoutRows()
    {
        const bool ltr = QApplication::isLeftToRight();
        const int padding = 4;
        const int keyWidth = computeKeyColumnWidth();
        const int valueWidth = width() - keyWidth - padding;
        const int rowHeight = fontMetrics().height();
        int rowY = 0;
        Q_FOREACH(Row* row, mRows) {
            if (ltr) {
                row->keyLabel->setGeometry(0, rowY, keyWidth, rowHeight);
                row->valueLabel->setGeometry(keyWidth + padding, rowY, valueWidth, rowHeight);
            } else {
                row->valueLabel->setGeometry(0, rowY, valueWidth, rowHeight);
                row->keyLabel->setGeometry(valueWidth + padding, rowY, keyWidth, rowHeight);
            }
            rowY += rowHeight;
        }
    }

protected:
    void showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        layoutRows();
    }

    void resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);
        layoutRows();
    }

private:
    QVector<Row*> mRows;

    int computeKeyColumnWidth() const
    {
        const int maxWidth = width() / 2;
        int keyWidth = 0;
        Q_FOREACH(Row* row, mRows) {
            int wantedWidth = row->keyLabel->sizeHint().width();
            if (wantedWidth > keyWidth) {
                keyWidth = qMin(wantedWidth, maxWidth);
            }
        }
        return keyWidth;
    }
};

struct InfoContextManagerItemPrivate
{
    InfoContextManagerItem* q;
    SideBarGroup* mGroup;

    // One selection fields
    QWidget* mOneFileWidget;
    KeyValueWidget* mKeyValueWidget;
    Document::Ptr mDocument;

    // Multiple selection fields
    QLabel* mMultipleFilesLabel;

    QPointer<ImageMetaInfoDialog> mImageMetaInfoDialog;

    void updateMetaInfoDialog()
    {
        if (!mImageMetaInfoDialog) {
            return;
        }
        ImageMetaInfoModel* model = mDocument ? mDocument->metaInfo() : 0;
        mImageMetaInfoDialog->setMetaInfo(model, GwenviewConfig::preferredMetaInfoKeyList());
    }

    void setupGroup()
    {
        mOneFileWidget = new QWidget();

        mKeyValueWidget = new KeyValueWidget(mOneFileWidget);

        QLabel* moreLabel = new QLabel(mOneFileWidget);
        moreLabel->setText(QString("<a href='#'>%1</a>").arg(i18nc("@action show more image meta info", "More...")));
        moreLabel->setAlignment(Qt::AlignRight);

        QVBoxLayout* layout = new QVBoxLayout(mOneFileWidget);
        layout->setMargin(2);
        layout->setSpacing(2);
        layout->addWidget(mKeyValueWidget);
        layout->addWidget(moreLabel);

        mMultipleFilesLabel = new QLabel();

        mGroup = new SideBarGroup(i18nc("@title:group", "Meta Information"));
        q->setWidget(mGroup);
        mGroup->addWidget(mOneFileWidget);
        mGroup->addWidget(mMultipleFilesLabel);

        EventWatcher::install(mGroup, QEvent::Show, q, SLOT(updateSideBarContent()));

        QObject::connect(moreLabel, SIGNAL(linkActivated(QString)),
                         q, SLOT(showMetaInfoDialog()));
    }

    void forgetCurrentDocument()
    {
        if (mDocument) {
            QObject::disconnect(mDocument.data(), 0, q, 0);
            // "Garbage collect" document
            mDocument = 0;
        }
    }
};

InfoContextManagerItem::InfoContextManagerItem(ContextManager* manager)
: AbstractContextManagerItem(manager)
, d(new InfoContextManagerItemPrivate)
{
    d->q = this;
    d->setupGroup();
    connect(contextManager(), SIGNAL(selectionChanged()),
            SLOT(updateSideBarContent()));
    connect(contextManager(), SIGNAL(selectionDataChanged()),
            SLOT(updateSideBarContent()));
}

InfoContextManagerItem::~InfoContextManagerItem()
{
    delete d;
}

void InfoContextManagerItem::updateSideBarContent()
{
    LOG("updateSideBarContent");
    if (!d->mGroup->isVisible()) {
        LOG("updateSideBarContent: not visible, not updating");
        return;
    }
    LOG("updateSideBarContent: really updating");

    KFileItemList itemList = contextManager()->selectedFileItemList();
    if (itemList.count() == 0) {
        d->forgetCurrentDocument();
        d->mOneFileWidget->hide();
        d->mMultipleFilesLabel->hide();
        d->updateMetaInfoDialog();
        return;
    }

    KFileItem item = itemList.first();
    if (itemList.count() == 1 && !ArchiveUtils::fileItemIsDirOrArchive(item)) {
        fillOneFileGroup(item);
    } else {
        fillMultipleItemsGroup(itemList);
    }
    d->updateMetaInfoDialog();
}

void InfoContextManagerItem::fillOneFileGroup(const KFileItem& item)
{
    d->mOneFileWidget->show();
    d->mMultipleFilesLabel->hide();

    d->forgetCurrentDocument();
    d->mDocument = DocumentFactory::instance()->load(item.url());
    connect(d->mDocument.data(), SIGNAL(metaInfoUpdated()),
            SLOT(updateOneFileInfo()));

    d->updateMetaInfoDialog();
    updateOneFileInfo();
}

void InfoContextManagerItem::fillMultipleItemsGroup(const KFileItemList& itemList)
{
    d->forgetCurrentDocument();

    int folderCount = 0, fileCount = 0;
    Q_FOREACH(const KFileItem & item, itemList) {
        if (item.isDir()) {
            folderCount++;
        } else {
            fileCount++;
        }
    }

    if (folderCount == 0) {
        d->mMultipleFilesLabel->setText(i18ncp("@label", "%1 file selected", "%1 files selected", fileCount));
    } else if (fileCount == 0) {
        d->mMultipleFilesLabel->setText(i18ncp("@label", "%1 folder selected", "%1 folders selected", folderCount));
    } else {
        d->mMultipleFilesLabel->setText(i18nc("@label. The two parameters are strings like '2 folders' and '1 file'.",
                                              "%1 and %2 selected", i18np("%1 folder", "%1 folders", folderCount), i18np("%1 file", "%1 files", fileCount)));
    }
    d->mOneFileWidget->hide();
    d->mMultipleFilesLabel->show();
}

void InfoContextManagerItem::updateOneFileInfo()
{
    if (!d->mDocument) {
        return;
    }

    ImageMetaInfoModel* metaInfoModel = d->mDocument->metaInfo();
    d->mKeyValueWidget->clear();
    Q_FOREACH(const QString & key, GwenviewConfig::preferredMetaInfoKeyList()) {
        QString label;
        QString value;
        metaInfoModel->getInfoForKey(key, &label, &value);

        if (!label.isEmpty() && !value.isEmpty()) {
            d->mKeyValueWidget->addRow(label, value);
        }
    }
    d->mKeyValueWidget->layoutRows();
}

void InfoContextManagerItem::showMetaInfoDialog()
{
    if (!d->mImageMetaInfoDialog) {
        d->mImageMetaInfoDialog = new ImageMetaInfoDialog(d->mOneFileWidget);
        d->mImageMetaInfoDialog->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(d->mImageMetaInfoDialog, SIGNAL(preferredMetaInfoKeyListChanged(QStringList)),
                SLOT(slotPreferredMetaInfoKeyListChanged(QStringList)));
    }
    d->mImageMetaInfoDialog->setMetaInfo(d->mDocument ? d->mDocument->metaInfo() : 0, GwenviewConfig::preferredMetaInfoKeyList());
    d->mImageMetaInfoDialog->show();
}

void InfoContextManagerItem::slotPreferredMetaInfoKeyListChanged(const QStringList& list)
{
    GwenviewConfig::setPreferredMetaInfoKeyList(list);
    GwenviewConfig::self()->writeConfig();
    updateOneFileInfo();
}

} // namespace
