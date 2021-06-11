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
#ifndef TAGITEMDELEGATE_H
#define TAGITEMDELEGATE_H

// KF
#include <KWidgetItemDelegate>

namespace Gwenview
{

using SemanticInfoTag = QString;

class TagItemDelegate : public KWidgetItemDelegate
{
    Q_OBJECT
public:
    TagItemDelegate(QAbstractItemView* view);

protected:
    QList<QWidget*> createItemWidgets(const QModelIndex &index) const override;

    void updateItemWidgets(const QList<QWidget*> widgets,
                           const QStyleOptionViewItem& option,
                           const QPersistentModelIndex& /*index*/) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &/*option*/,
                   const QModelIndex &/*index*/) const override;

Q_SIGNALS:
    void removeTagRequested(const SemanticInfoTag& tag);
    void assignTagToAllRequested(const SemanticInfoTag& tag);

private Q_SLOTS:
    void slotRemoveButtonClicked();
    void slotAssignToAllButtonClicked();

private:
    int mButtonSize;
    int mMargin;
    int mSpacing;
};

} // namespace

#endif /* TAGITEMDELEGATE_H */
