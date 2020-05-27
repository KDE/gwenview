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
#include <kipi/interface.h>
#include <kipi/imagecollectionshared.h>
#include <kipi/plugin.h>
#include <libkipi_version.h>

class QAction;

#ifndef KIPI_VERSION_MAJOR
#error KIPI_VERSION_MAJOR should be provided.
#endif
#if KIPI_VERSION_MAJOR >= 5
#define GWENVIEW_KIPI_WITH_CREATE_METHODS
#if KIPI_VERSION_MINOR == 0
# define GWENVIEW_KIPI_WITH_CREATE_RAW_PROCESSOR
#endif
#endif

namespace Gwenview
{

struct KIPIInterfacePrivate;

class MainWindow;

class KIPIInterface : public KIPI::Interface
{
    Q_OBJECT

public:
    KIPIInterface(MainWindow*);
    ~KIPIInterface() override;

    KIPI::ImageCollection currentAlbum() override;
    KIPI::ImageCollection currentSelection() override;
    QList<KIPI::ImageCollection> allAlbums() override;
    KIPI::ImageInfo info(const QUrl &url) override;
    int features() const override;
    bool addImage(const QUrl&, QString& err) override;
    void delImage(const QUrl&) override;
    void refreshImages(const QList<QUrl>& urls) override;

    KIPI::ImageCollectionSelector* imageCollectionSelector(QWidget *parent) override;
    KIPI::UploadWidget* uploadWidget(QWidget *parent) override;

    QList<QAction*> pluginActions(KIPI::Category) const;

    bool isLoadingFinished() const;

#ifdef GWENVIEW_KIPI_WITH_CREATE_METHODS
    KIPI::FileReadWriteLock* createReadWriteLock(const QUrl& url) const override;
    KIPI::MetadataProcessor* createMetadataProcessor() const override;
#ifdef GWENVIEW_KIPI_WITH_CREATE_RAW_PROCESSOR
    virtual KIPI::RawProcessor* createRawProcessor() const;
#endif
#endif

Q_SIGNALS:
    void loadingFinished();

public Q_SLOTS:
    void loadPlugins();

private Q_SLOTS:
    void slotSelectionChanged();
    void slotDirectoryChanged();
    void packageFinished();
    void init();
    void loadOnePlugin();

private:
    KIPIInterfacePrivate* const d;
};

class ImageCollection : public KIPI::ImageCollectionShared
{
public:
    ImageCollection(const QUrl &dirURL, const QString& name, const QList<QUrl>& images)
        : KIPI::ImageCollectionShared()
        , mDirURL(dirURL)
        , mName(name)
        , mImages(images) {}

    QString name() override {
        return mName;
    }
    QString comment() override {
        return QString();
    }
    QList<QUrl> images() override {
        return mImages;
    }
    QUrl uploadRoot() {
        return QUrl(QStringLiteral("/"));
    }
    QUrl uploadPath() {
        return mDirURL;
    }
    QString uploadRootName() override
    {
        return QStringLiteral("/");
    }
    bool isDirectory() override {
        return true;
    }

private:
    QUrl mDirURL;
    QString mName;
    QList<QUrl> mImages;
};

} // namespace
#endif /* KIPIINTERFACE_H */
