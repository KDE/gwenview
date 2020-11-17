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
#include "infocontextmanageritem.h"

// Qt
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

// KF
#include <KFileItem>
#include <KLocalizedString>

// Local
#include "imagemetainfodialog.h"
#include "sidebar.h"
#include <lib/archiveutils.h>
#include <lib/contextmanager.h>
#include <lib/eventwatcher.h>
#include <lib/gvdebug.h>
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
#define LOG(x) qCDebug(GWENVIEW_APP_LOG) << x
#else
#define LOG(x) ;
#endif

/**
 * This widget is capable of showing multiple lines of key/value pairs.
 */
class KeyValueWidget : public QWidget
{
    struct Row
    {
        Row(QWidget* parent)
        : keyLabel(new QLabel(parent))
        , valueLabel(new QLabel(parent))
        {
            initLabel(keyLabel);
            initLabel(valueLabel);

            QPalette pal = keyLabel->palette();
            QColor color = pal.color(QPalette::WindowText);
            color.setAlphaF(0.65);
            pal.setColor(QPalette::WindowText, color);
            keyLabel->setPalette(pal);

            valueLabel->setContentsMargins(6, 0, 0, 6);
        }

        ~Row()
        {
            delete keyLabel;
            delete valueLabel;
        }

        int setLabelGeometries(int rowY, int labelWidth)
        {
            int labelHeight = keyLabel->heightForWidth(labelWidth);
            keyLabel->setGeometry(0, rowY, labelWidth, labelHeight);
            rowY += labelHeight;
            labelHeight = valueLabel->heightForWidth(labelWidth);
            valueLabel->setGeometry(0, rowY, labelWidth, labelHeight);
            rowY += labelHeight;
            return rowY;
        }

        int heightForWidth(int width) const
        {
            return keyLabel->heightForWidth(width) + valueLabel->heightForWidth(width);
        }

        static void initLabel(QLabel* label)
        {
            label->setWordWrap(true);
            label->show();
            label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
        }

        QLabel* keyLabel;
        QLabel* valueLabel;
    };
public:
    explicit KeyValueWidget(QWidget* parent = nullptr)
    : QWidget(parent)
    {
        QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        policy.setHeightForWidth(true);
        setSizePolicy(policy);
    }
    ~KeyValueWidget()
    {
        qDeleteAll(mRows);
        mRows.clear();
    }

    QSize sizeHint() const override
    {
        int width = 150;
        int height = heightForWidth(width);
        return QSize(width, height);
    }

    int heightForWidth(int w) const override
    {
        int height = 0;
        for (Row* row : qAsConst(mRows)) {
             height += row->heightForWidth(w);
        }
        return height;
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
    }

    static bool rowsLessThan(const Row* row1, const Row* row2)
    {
        return row1->keyLabel->text() < row2->keyLabel->text();
    }

    void finishAddRows()
    {
        std::sort(mRows.begin(), mRows.end(), KeyValueWidget::rowsLessThan);
        updateGeometry();
    }

    void layoutRows()
    {
        // Layout labels manually: I tried to use a QVBoxLayout but for some
        // reason when using software animations the widget blinks when going
        // from one image to another
        int rowY = 0;
        const int labelWidth = width();
        for (Row* row : qAsConst(mRows)) {
            rowY = row->setLabelGeometries(rowY, labelWidth);
        }
    }

protected:
    void showEvent(QShowEvent* event) override
    {
        QWidget::showEvent(event);
        layoutRows();
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        layoutRows();
    }

private:
    QVector<Row*> mRows;
};

struct InfoContextManagerItemPrivate
{
    InfoContextManagerItem* q;
    SideBarGroup* mGroup;

    // One selection fields
    QScrollArea* mOneFileWidget;
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
        ImageMetaInfoModel* model = mDocument ? mDocument->metaInfo() : nullptr;
        mImageMetaInfoDialog->setMetaInfo(model, GwenviewConfig::preferredMetaInfoKeyList());
    }

    void setupGroup()
    {
        mOneFileWidget = new QScrollArea();
        mOneFileWidget->setFrameStyle(QFrame::NoFrame);
        mOneFileWidget->setWidgetResizable(true);

        mKeyValueWidget = new KeyValueWidget;

        QLabel* moreLabel = new QLabel(mOneFileWidget);
        moreLabel->setText(QStringLiteral("<a href='#'>%1</a>").arg(i18nc("@action show more image meta info", "More...")));
        moreLabel->setAlignment(Qt::AlignRight);

        QWidget* content = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(content);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(2);
        layout->addWidget(mKeyValueWidget);
        layout->addWidget(moreLabel);

        mOneFileWidget->setWidget(content);

        mMultipleFilesLabel = new QLabel();

        mGroup = new SideBarGroup(i18nc("@title:group", "Meta Information"));
        q->setWidget(mGroup);
        mGroup->addWidget(mOneFileWidget);
        mGroup->addWidget(mMultipleFilesLabel);

        EventWatcher::install(mGroup, QEvent::Show, q, SLOT(updateSideBarContent()));

        QObject::connect(moreLabel, &QLabel::linkActivated, q, &InfoContextManagerItem::showMetaInfoDialog);
    }

    void forgetCurrentDocument()
    {
        if (mDocument) {
            QObject::disconnect(mDocument.data(), nullptr, q, nullptr);
            // "Garbage collect" document
            mDocument = nullptr;
        }
    }
};

InfoContextManagerItem::InfoContextManagerItem(ContextManager* manager)
: AbstractContextManagerItem(manager)
, d(new InfoContextManagerItemPrivate)
{
    d->q = this;
    d->setupGroup();
    connect(contextManager(), &ContextManager::selectionChanged,
            this, &InfoContextManagerItem::updateSideBarContent);
    connect(contextManager(), &ContextManager::selectionDataChanged,
            this, &InfoContextManagerItem::updateSideBarContent);
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
    if (itemList.isEmpty()) {
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
    connect(d->mDocument.data(), &Document::metaInfoUpdated,
            this, &InfoContextManagerItem::updateOneFileInfo);

    d->updateMetaInfoDialog();
    updateOneFileInfo();
}

void InfoContextManagerItem::fillMultipleItemsGroup(const KFileItemList& itemList)
{
    d->forgetCurrentDocument();

    int folderCount = 0, fileCount = 0;
    for (const KFileItem & item : itemList) {
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
    const QStringList preferredMetaInfoKeyList = GwenviewConfig::preferredMetaInfoKeyList();
    for (const QString & key : preferredMetaInfoKeyList) {
        QString label;
        QString value;
        metaInfoModel->getInfoForKey(key, &label, &value);
        if (!label.isEmpty() && !value.isEmpty()) {
            d->mKeyValueWidget->addRow(label, value);
        }
    }
    d->mKeyValueWidget->finishAddRows();
    d->mKeyValueWidget->layoutRows();
}

void InfoContextManagerItem::showMetaInfoDialog()
{
    if (!d->mImageMetaInfoDialog) {
        d->mImageMetaInfoDialog = new ImageMetaInfoDialog(d->mOneFileWidget);
        d->mImageMetaInfoDialog->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(d->mImageMetaInfoDialog.data(), &ImageMetaInfoDialog::preferredMetaInfoKeyListChanged,
                this, &InfoContextManagerItem::slotPreferredMetaInfoKeyListChanged);
    }
    d->mImageMetaInfoDialog->setMetaInfo(d->mDocument ? d->mDocument->metaInfo() : nullptr, GwenviewConfig::preferredMetaInfoKeyList());
    d->mImageMetaInfoDialog->show();
}

void InfoContextManagerItem::slotPreferredMetaInfoKeyListChanged(const QStringList& list)
{
    GwenviewConfig::setPreferredMetaInfoKeyList(list);
    GwenviewConfig::self()->save();
    updateOneFileInfo();
}

} // namespace
