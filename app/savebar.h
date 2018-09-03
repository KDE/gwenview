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
#ifndef SAVEBAR_H
#define SAVEBAR_H

// Qt

// KDE

// Local
#include <lib/slidecontainer.h>

class KActionCollection;
class QUrl;

namespace Gwenview
{

struct SaveBarPrivate;
class SaveBar : public SlideContainer
{
    Q_OBJECT
public:
    SaveBar(QWidget* parent, KActionCollection* collection);
    ~SaveBar() override;

    /**
     * Init widgets which depend on an initialized actionCollection
     */
    void initActionDependentWidgets();

    void setFullScreenMode(bool);

public Q_SLOTS:
    void setCurrentUrl(const QUrl&);

Q_SIGNALS:
    void requestSaveAll();
    void goToUrl(const QUrl&);

private:
    SaveBarPrivate* const d;

private Q_SLOTS:
    void updateContent();
    void triggerAction(const QString& action);
};

} // namespace

#endif /* SAVEBAR_H */
