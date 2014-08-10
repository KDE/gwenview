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
#include <QHeaderView>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTreeView>

// KDE
#include <QDebug>
#include <KLocale>

// STL
#include <memory>

// Local
#include <lib/preferredimagemetainfomodel.h>

namespace Gwenview
{

class MetaInfoDelegate : public QStyledItemDelegate
{
public:
    MetaInfoDelegate(QObject* parent)
        : QStyledItemDelegate(parent)
    {}

protected:
    virtual void paint(QPainter* painter, const QStyleOptionViewItem& _option, const QModelIndex& index) const
    {
        QStyleOptionViewItemV4 option = _option;
        if (!index.parent().isValid()) {
            option.displayAlignment = Qt::AlignCenter | Qt::AlignBottom;
            option.font.setBold(true);
        }
        QStyledItemDelegate::paint(painter, option, index);
        if (!index.parent().isValid()) {
            painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
        }
    }

    virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
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
    ExpandedTreeView(QWidget* parent)
        : QTreeView(parent)
        {}

protected:
    virtual void rowsInserted(const QModelIndex& parent, int start, int end)
    {
        QTreeView::rowsInserted(parent, start, end);
        if (!parent.isValid()) {
            for (int row = start; row <= end; ++row) {
                setUpRootIndex(row);
            }
        }
    }

    virtual void reset()
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

struct ImageMetaInfoDialogPrivate
{
    std::auto_ptr<PreferredImageMetaInfoModel> mModel;
    QTreeView* mTreeView;
};

ImageMetaInfoDialog::ImageMetaInfoDialog(QWidget* parent)
: KDialog(parent)
, d(new ImageMetaInfoDialogPrivate)
{
    d->mTreeView = new ExpandedTreeView(this);
    d->mTreeView->setRootIsDecorated(false);
    d->mTreeView->setIndentation(0);
    d->mTreeView->setItemDelegate(new MetaInfoDelegate(d->mTreeView));
    setMainWidget(d->mTreeView);
    setCaption(i18nc("@title:window", "Image Information"));
    setButtons(KDialog::Close);
}

ImageMetaInfoDialog::~ImageMetaInfoDialog()
{
    delete d;
}

void ImageMetaInfoDialog::setMetaInfo(ImageMetaInfoModel* model, const QStringList& list)
{
    if (model) {
        d->mModel.reset(new PreferredImageMetaInfoModel(model, list));
        connect(d->mModel.get(), SIGNAL(preferredMetaInfoKeyListChanged(QStringList)),
                this, SIGNAL(preferredMetaInfoKeyListChanged(QStringList)));
    } else {
        d->mModel.reset(0);
    }
    d->mTreeView->setModel(d->mModel.get());

    d->mTreeView->header()->resizeSection(0, sizeHint().width() / 2 - KDialog::marginHint() * 2);
}

QSize ImageMetaInfoDialog::sizeHint() const
{
    return QSize(400, 300);
}

} // namespace
