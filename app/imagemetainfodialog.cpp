// vim: set tabstop=4 shiftwidth=4 expandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "imagemetainfodialog.h"

// Qt
#include <QApplication>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QPainter>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QVBoxLayout>

// KF
#include <KLocalizedString>

// STL
#include <memory>

// Local
#include <lib/preferredimagemetainfomodel.h>

namespace Gwenview
{
class MetaInfoDelegate : public QStyledItemDelegate
{
public:
    MetaInfoDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
    {
    }

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &_option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem option = _option;
        if (!index.parent().isValid()) {
            option.displayAlignment = Qt::AlignCenter | Qt::AlignBottom;
            option.font.setBold(true);
        }
        QStyledItemDelegate::paint(painter, option, index);
        if (!index.parent().isValid()) {
            painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
        }
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize sh = QStyledItemDelegate::sizeHint(option, index);
        if (!index.parent().isValid()) {
            sh.setHeight(sh.height() * 3 / 2);
        }
        return sh;
    }
};

/**
 * A tree view which is always fully expanded
 */
class ExpandedTreeView : public QTreeView
{
public:
    explicit ExpandedTreeView(QWidget *parent)
        : QTreeView(parent)
    {
    }

protected:
    void rowsInserted(const QModelIndex &parent, int start, int end) override
    {
        QTreeView::rowsInserted(parent, start, end);
        if (!parent.isValid()) {
            for (int row = start; row <= end; ++row) {
                setUpRootIndex(row);
            }
        }
    }

    void reset() override
    {
        QTreeView::reset();
        if (model()) {
            for (int row = 0; row < model()->rowCount(); ++row) {
                setUpRootIndex(row);
            }
        }
    }

private:
    void setUpRootIndex(int row)
    {
        expand(model()->index(row, 0));
        setFirstColumnSpanned(row, QModelIndex(), true);
    }
};

struct ImageMetaInfoDialogPrivate {
    std::unique_ptr<PreferredImageMetaInfoModel> mModel;
    QTreeView *mTreeView;
};

ImageMetaInfoDialog::ImageMetaInfoDialog(QWidget *parent)
    : QDialog(parent)
    , d(new ImageMetaInfoDialogPrivate)
{
    d->mTreeView = new ExpandedTreeView(this);
    d->mTreeView->setRootIsDecorated(false);
    d->mTreeView->setIndentation(0);
    d->mTreeView->setItemDelegate(new MetaInfoDelegate(d->mTreeView));
    setWindowTitle(i18nc("@title:window", "Image Information"));

    setLayout(new QVBoxLayout);
    layout()->addWidget(d->mTreeView);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    layout()->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

ImageMetaInfoDialog::~ImageMetaInfoDialog()
{
    delete d;
}

void ImageMetaInfoDialog::setMetaInfo(ImageMetaInfoModel *model, const QStringList &list)
{
    if (model) {
        d->mModel = std::make_unique<PreferredImageMetaInfoModel>(model, list);
        connect(d->mModel.get(), &PreferredImageMetaInfoModel::preferredMetaInfoKeyListChanged, this, &ImageMetaInfoDialog::preferredMetaInfoKeyListChanged);
    } else {
        d->mModel.reset(nullptr);
    }
    d->mTreeView->setModel(d->mModel.get());

    const int marginSize = QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    d->mTreeView->header()->resizeSection(0, sizeHint().width() / 2 - marginSize * 2);
}

QSize ImageMetaInfoDialog::sizeHint() const
{
    return QSize(400, 300);
}

} // namespace
