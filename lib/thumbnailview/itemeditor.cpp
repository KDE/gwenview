// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "itemeditor.h"

// Qt
#include <QApplication>
#include <QShowEvent>

// KDE
#include <KDebug>
#include <QMimeDatabase>

// Local

namespace Gwenview
{

struct ItemEditorPrivate
{
    QPoint mCenter;
};

ItemEditor::ItemEditor(QWidget* parent)
: KLineEdit(parent)
, d(new ItemEditorPrivate)
{
    setPalette(QApplication::palette());
    connect(this, &ItemEditor::textChanged, this, &ItemEditor::resizeToContents);
    setTrapReturnKey(true);
}

ItemEditor::~ItemEditor()
{
    delete d;
}

void ItemEditor::showEvent(QShowEvent* event)
{
    // We can't do this in PreviewItemDelegate::updateEditorGeometry() because QAbstractItemView outsmarts us by calling selectAll() on the editor if it is a QLineEdit
    QMimeDatabase db;
    const QString extension = db.suffixForFileName(text());
    if (!extension.isEmpty()) {
        // The filename contains an extension. Assure that only the filename
        // gets selected.
        const int selectionLength = text().length() - extension.length() - 1;
        setSelection(0, selectionLength);
    }
    KLineEdit::showEvent(event);
}

void ItemEditor::resizeToContents()
{
    if (d->mCenter.isNull()) {
        d->mCenter = geometry().center();
    }
    int textWidth = fontMetrics().width("  " + text() + "  ");
    QRect rect = geometry();
    rect.setWidth(textWidth);
    rect.moveCenter(d->mCenter);
    if (rect.right() > parentWidget()->width()) {
        rect.setRight(parentWidget()->width());
    }
    if (rect.left() < 0) {
        rect.setLeft(0);
    }
    setGeometry(rect);

}

} // namespace
