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
#include "invisiblebuttongroup.h"

// Qt
#include <QAbstractButton>
#include <QButtonGroup>

// Local

namespace Gwenview
{
struct InvisibleButtonGroupPrivate {
    QButtonGroup *mGroup = nullptr;
};

InvisibleButtonGroup::InvisibleButtonGroup(QWidget *parent)
    : QWidget(parent)
    , d(new InvisibleButtonGroupPrivate)
{
    hide();
    d->mGroup = new QButtonGroup(this);
    d->mGroup->setExclusive(true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(d->mGroup,
            &QButtonGroup::idClicked,
#else
    connect(d->mGroup,
            QOverload<int>::of(&QButtonGroup::buttonClicked),
#endif
            this,
            &InvisibleButtonGroup::selectionChanged);
}

InvisibleButtonGroup::~InvisibleButtonGroup()
{
    delete d;
}

int InvisibleButtonGroup::selected() const
{
    return d->mGroup->checkedId();
}

void InvisibleButtonGroup::addButton(QAbstractButton *button, int id)
{
    d->mGroup->addButton(button, id);
}

void InvisibleButtonGroup::setSelected(int id)
{
    QAbstractButton *button = d->mGroup->button(id);
    if (button) {
        button->setChecked(true);
    }
}

} // namespace
