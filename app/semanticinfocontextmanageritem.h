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
#ifndef SEMANTICINFOCONTEXTMANAGERITEM_H
#define SEMANTICINFOCONTEXTMANAGERITEM_H

// Qt

// KDE

// Local
#include "abstractcontextmanageritem.h"
#include <lib/semanticinfo/abstractsemanticinfobackend.h>

class KActionCollection;

namespace Gwenview
{

class ViewMainPage;

struct SemanticInfoContextManagerItemPrivate;
class SemanticInfoContextManagerItem : public AbstractContextManagerItem
{
    Q_OBJECT
public:
    SemanticInfoContextManagerItem(ContextManager*, KActionCollection*, ViewMainPage* viewMainPage);
    ~SemanticInfoContextManagerItem() Q_DECL_OVERRIDE;

protected:
    bool eventFilter(QObject*, QEvent*) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotSelectionChanged();
    void update();
    void slotRatingChanged(int rating);
    void storeDescription();
    void assignTag(const SemanticInfoTag&);
    void removeTag(const SemanticInfoTag&);
    void showSemanticInfoDialog();

private:
    friend struct SemanticInfoContextManagerItemPrivate;
    SemanticInfoContextManagerItemPrivate* const d;
};

} // namespace

#endif /* SEMANTICINFOCONTEXTMANAGERITEM_H */
