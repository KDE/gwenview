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
#ifndef TAGWIDGET_H
#define TAGWIDGET_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QMap>
#include <QWidget>

// KDE

// Local
#include <lib/semanticinfo/abstractsemanticinfobackend.h>

namespace Gwenview
{

typedef QMap<SemanticInfoTag, bool> TagInfo;

struct TagWidgetPrivate;
class GWENVIEWLIB_EXPORT TagWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TagWidget(QWidget* parent = nullptr);
    ~TagWidget();
    void setTagInfo(const TagInfo&);
    void setSemanticInfoBackEnd(AbstractSemanticInfoBackEnd*);

Q_SIGNALS:
    void tagAssigned(const SemanticInfoTag&);
    void tagRemoved(const SemanticInfoTag&);

private Q_SLOTS:
    void addTagFromComboBox();
    void assignTag(const SemanticInfoTag& tag);
    void removeTag(const SemanticInfoTag&);

private:
    TagWidgetPrivate* const d;
};

} // namespace

#endif /* TAGWIDGET_H */
