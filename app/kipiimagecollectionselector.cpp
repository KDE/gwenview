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
#include "kipiimagecollectionselector.h"

// Qt
#include <QListWidget>
#include <QVBoxLayout>

// KDE
#include <KLocalizedString>

// Local

namespace Gwenview
{

struct KIPIImageCollectionSelectorPrivate
{
    KIPIInterface* mInterface;
    QListWidget* mListWidget;
};

KIPIImageCollectionSelector::KIPIImageCollectionSelector(KIPIInterface* interface, QWidget* parent)
: KIPI::ImageCollectionSelector(parent)
, d(new KIPIImageCollectionSelectorPrivate)
{
    d->mInterface = interface;

    d->mListWidget = new QListWidget;
    const QList<KIPI::ImageCollection> list = interface->allAlbums();
    for (const KIPI::ImageCollection & collection : list) {
        QListWidgetItem* item = new QListWidgetItem(d->mListWidget);
        QString name = collection.name();
        int imageCount = collection.images().size();
        QString title = i18ncp("%1 is collection name, %2 is image count in collection",
                               "%1 (%2 image)", "%1 (%2 images)", name, imageCount);

        item->setText(title);
        item->setData(Qt::UserRole, name);
    }

    connect(d->mListWidget, &QListWidget::currentRowChanged, this, &KIPIImageCollectionSelector::selectionChanged);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(d->mListWidget);
    layout->setContentsMargins(0, 0, 0, 0);
}

KIPIImageCollectionSelector::~KIPIImageCollectionSelector()
{
    delete d;
}

QList<KIPI::ImageCollection> KIPIImageCollectionSelector::selectedImageCollections() const
{
    QListWidgetItem* item = d->mListWidget->currentItem();
    QList<KIPI::ImageCollection> selectedList;
    if (item) {
        QString name = item->data(Qt::UserRole).toString();
        const QList<KIPI::ImageCollection> list = d->mInterface->allAlbums();
        for (const KIPI::ImageCollection & collection : list) {
            if (collection.name() == name) {
                selectedList << collection;
                break;
            }
        }
    }
    return selectedList;
}

} // namespace
