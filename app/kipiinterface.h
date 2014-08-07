// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2000-2008 Aurélien Gâteau <agateau@kde.org>
Copyright 2008      Angelo Naselli  <anaselli@linux.it>

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
#ifndef KIPIINTERFACE_H
#define KIPIINTERFACE_H

// KIPI
#include <libkipi/interface.h>
#include <libkipi/imagecollectionshared.h>
#include <libkipi/plugin.h>

class QAction;

namespace Gwenview
{

struct KIPIInterfacePrivate;

class MainWindow;

class KIPIInterface : public KIPI::Interface
{
    Q_OBJECT

public:
    KIPIInterface(MainWindow*);
    virtual ~KIPIInterface();

    KIPI::ImageCollection currentAlbum();
    KIPI::ImageCollection currentSelection();
    QList<KIPI::ImageCollection> allAlbums();
    KIPI::ImageInfo info(const KUrl& url);
    int features() const;
    virtual bool addImage(const KUrl&, QString& err);
    virtual void delImage(const KUrl&);
    virtual void refreshImages(const KUrl::List& urls);

    virtual KIPI::ImageCollectionSelector* imageCollectionSelector(QWidget *parent);
    virtual KIPI::UploadWidget* uploadWidget(QWidget *parent);

    QList<QAction*> pluginActions(KIPI::Category) const;

    bool isLoadingFinished() const;

Q_SIGNALS:
    void loadingFinished();

public Q_SLOTS:
    void loadPlugins();

private Q_SLOTS:
    void slotSelectionChanged();
    void slotDirectoryChanged();
    void init();
    void loadOnePlugin();

private:
    KIPIInterfacePrivate* const d;
};

class ImageCollection : public KIPI::ImageCollectionShared
{
public:
    ImageCollection(KUrl dirURL, const QString& name, const KUrl::List& images)
        : KIPI::ImageCollectionShared()
        , mDirURL(dirURL)
        , mName(name)
        , mImages(images) {}

    QString name()           {
        return mName;
    }
    QString comment()        {
        return QString();
    }
    KUrl::List images()      {
        return mImages;
    }
    KUrl uploadRoot()        {
        return KUrl("/");
    }
    KUrl uploadPath()        {
        return mDirURL;
    }
    QString uploadRootName()
    {
        return "/";
    }
    bool isDirectory()       {
        return true;
    }

private:
    KUrl mDirURL;
    QString mName;
    KUrl::List mImages;
};

} // namespace
#endif /* KIPIINTERFACE_H */
