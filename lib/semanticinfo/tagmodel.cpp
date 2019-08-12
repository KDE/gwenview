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
#include "tagmodel.h"

// Qt

// KDE
#include <QDebug>
#include <QIcon>

// Local
#include "abstractsemanticinfobackend.h"

namespace Gwenview
{

struct TagModelPrivate
{
    AbstractSemanticInfoBackEnd* mBackEnd;
};

static QStandardItem* createItem(const SemanticInfoTag& tag, const QString& label, TagModel::AssignmentStatus status)
{
    QStandardItem* item = new QStandardItem(label);
    item->setData(tag, TagModel::TagRole);
    item->setData(label.toLower(), TagModel::SortRole);
    item->setData(status, TagModel::AssignmentStatusRole);
    item->setData(QIcon::fromTheme("mail-tagged"), Qt::DecorationRole);
    return item;
}

TagModel::TagModel(QObject* parent)
: QStandardItemModel(parent)
, d(new TagModelPrivate)
{
    d->mBackEnd = nullptr;
    setSortRole(SortRole);
}

TagModel::~TagModel()
{
    delete d;
}

void TagModel::setSemanticInfoBackEnd(AbstractSemanticInfoBackEnd* backEnd)
{
    d->mBackEnd = backEnd;
}

void TagModel::setTagSet(const TagSet& set)
{
    clear();
    for (const SemanticInfoTag & tag : set) {
        QString label = d->mBackEnd->labelForTag(tag);
        QStandardItem* item = createItem(tag, label, TagModel::FullyAssigned);
        appendRow(item);
    }
    sort(0);
}

void TagModel::addTag(const SemanticInfoTag& tag, const QString& _label, TagModel::AssignmentStatus status)
{
    int row;
    QString label = _label.isEmpty() ? d->mBackEnd->labelForTag(tag) : _label;

    const QString sortLabel = label.toLower();
    // This is not optimal, implement dichotomic search if necessary
    for (row = 0; row < rowCount(); ++row) {
        const QModelIndex idx = index(row, 0);
        if (idx.data(SortRole).toString().compare(sortLabel) > 0) {
            break;
        }
    }
    if (row > 0) {
        QStandardItem* _item = item(row - 1);
        Q_ASSERT(_item);
        if (_item->data(TagRole).toString() == tag) {
            // Update, do not add
            _item->setData(label.toLower(), SortRole);
            _item->setData(status, AssignmentStatusRole);
            return;
        }
    }
    QStandardItem* _item = createItem(tag, label, status);
    insertRow(row, _item);
}

void TagModel::removeTag(const SemanticInfoTag& tag)
{
    // This is not optimal, implement dichotomic search if necessary
    for (int row = 0; row < rowCount(); ++row) {
        if (index(row, 0).data(TagRole).toString() == tag) {
            removeRow(row);
            return;
        }
    }
}

TagModel* TagModel::createAllTagsModel(QObject* parent, AbstractSemanticInfoBackEnd* backEnd)
{
    TagModel* tagModel = new TagModel(parent);
    tagModel->setSemanticInfoBackEnd(backEnd);
    tagModel->setTagSet(backEnd->allTags());
    connect(backEnd, SIGNAL(tagAdded(SemanticInfoTag,QString)),
            tagModel, SLOT(addTag(SemanticInfoTag,QString)));
    return tagModel;
}

} // namespace
