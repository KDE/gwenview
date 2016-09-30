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
#include "kipiinterface.h"

// Qt
#include <QList>
#include <QMenu>
#include <QRegExp>
#include <QDesktopServices>
#include <QFileSystemWatcher>
#include <QTimer>

// KDE
#include <QAction>
#include <KActionCollection>
#include <QDebug>
#include <QUrl>
#include <KXMLGUIFactory>
#include <KDirLister>
#include <KLocalizedString>

// KIPI
#include <kipi/imagecollectionshared.h>
#include <kipi/imageinfo.h>
#include <kipi/imageinfoshared.h>
#include <kipi/plugin.h>
#include <kipi/pluginloader.h>
//#include <kipi/version.h>

// local
#include "mainwindow.h"
#include "kipiimagecollectionselector.h"
#include "kipiuploadwidget.h"
#include <lib/contextmanager.h>
#include <lib/jpegcontent.h>
#include <lib/mimetypeutils.h>
#include <lib/timeutils.h>
#include <lib/semanticinfo/sorteddirmodel.h>

namespace Gwenview
{
#undef ENABLE_LOG
#undef LOG

//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) qDebug() << x
#else
#define LOG(x) ;
#endif

class KIPIImageInfo : public KIPI::ImageInfoShared
{
    static const QRegExp sExtensionRE;
public:
    KIPIImageInfo(KIPI::Interface* interface, const QUrl &url)
    : KIPI::ImageInfoShared(interface, url)
    {
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);

        mAttributes.insert("name", url.fileName());
        mAttributes.insert("comment", comment());
        mAttributes.insert("date", TimeUtils::dateTimeForFileItem(item));
        mAttributes.insert("orientation", orientation());
        mAttributes.insert("title", prettyFileName());
        int size = item.size();
        if (size > 0) {
            mAttributes.insert("filesize", size);
        }
    }

    QMap<QString, QVariant> attributes() {
        return mAttributes;
    }

    void delAttributes(const QStringList& attributeNames)
    {
        Q_FOREACH(const QString& name, attributeNames) {
            mAttributes.remove(name);
        }
    }

    void clearAttributes()
    {
        mAttributes.clear();
    }

    void addAttributes(const QVariantMap& attributes)
    {
        QVariantMap::ConstIterator
            it = attributes.constBegin(),
            end = attributes.constEnd();
        for (; it != end; ++it) {
            mAttributes.insert(it.key(), it.value());
        }
    }

private:
    QString prettyFileName() const
    {
        QString txt = _url.fileName();
        txt.replace('_', ' ');
        txt.remove(sExtensionRE);
        return txt;
    }

    QString comment() const
    {
        if (!_url.isLocalFile()) return QString();

        JpegContent content;
        bool ok = content.load(_url.toLocalFile());
        if (!ok) return QString();

        return content.comment();
    }

    int orientation() const
    {
#if 0 //PORT QT5
        KFileMetaInfo metaInfo(_url);

        if (!metaInfo.isValid()) {
            return 0;
        }

        const KFileMetaInfoItem& mii = metaInfo.item("http://freedesktop.org/standards/xesam/1.0/core#orientation");
        bool ok = false;
        const Orientation orientation = (Orientation)mii.value().toInt(&ok);
        if (!ok) {
            return 0;
        }

        switch (orientation) {
        case NOT_AVAILABLE:
        case NORMAL:
            return 0;

        case ROT_90:
            return 90;

        case ROT_180:
            return 180;

        case ROT_270:
            return 270;

        case HFLIP:
        case VFLIP:
        case TRANSPOSE:
        case TRANSVERSE:
            qWarning() << "Can't represent an orientation value of" << orientation << "as an angle (" << _url << ')';
            return 0;
        }
#endif
        //qWarning() << "Don't know how to handle an orientation value of" << orientation << '(' << _url << ')';
        return 0;
    }

    QVariantMap mAttributes;
};

const QRegExp KIPIImageInfo::sExtensionRE("\\.[a-z0-9]+$", Qt::CaseInsensitive);

struct MenuInfo
{
    QString mName;
    QList<QAction*> mActions;

    MenuInfo()
    {}

    MenuInfo(const QString& name)
    : mName(name)
    {}
};
typedef QMap<KIPI::Category, MenuInfo> MenuInfoMap;

struct KIPIInterfacePrivate
{
    KIPIInterface* q;
    MainWindow* mMainWindow;
    QMenu* mPluginMenu;
    KIPI::PluginLoader* mPluginLoader;
    KIPI::PluginLoader::PluginList mPluginQueue;
    MenuInfoMap mMenuInfoMap;
    QAction * mLoadingAction;
    QAction * mNoPluginAction;
    QAction * mInstallPluginAction;
    QPointer<QFileSystemWatcher> mPluginWatcher;

    void setupPluginsMenu()
    {
        mPluginMenu = static_cast<QMenu*>(
                          mMainWindow->factory()->container("plugins", mMainWindow));
        QObject::connect(mPluginMenu, &QMenu::aboutToShow, q, &KIPIInterface::loadPlugins);
    }

    QAction * createDummyPluginAction(const QString& text)
    {
        QAction * action = new QAction(q);
        action->setText(text);
        //PORT QT5 action->setShortcutConfigurable(false);
        action->setEnabled(false);
        return action;
    }
};

KIPIInterface::KIPIInterface(MainWindow* mainWindow)
: KIPI::Interface(mainWindow)
, d(new KIPIInterfacePrivate)
{
    d->q = this;
    d->mMainWindow = mainWindow;
    d->mPluginLoader = 0;
    d->mPluginWatcher = 0;
    d->mLoadingAction = d->createDummyPluginAction(i18n("Loading..."));
    d->mNoPluginAction = d->createDummyPluginAction(i18n("No Plugin Found"));
    d->mInstallPluginAction = d->createDummyPluginAction(i18n("Install Plugins"));

    d->setupPluginsMenu();
    QObject::connect(d->mMainWindow->contextManager(), SIGNAL(selectionChanged()),
                     this, SLOT(slotSelectionChanged()));
    QObject::connect(d->mMainWindow->contextManager(), SIGNAL(currentDirUrlChanged(QUrl)),
                     this, SLOT(slotDirectoryChanged()));
#if 0
//TODO instead of delaying can we load them all at start-up to use actions somewhere else?
// delay a bit, so that it's called after loadPlugins()
    QTimer::singleShot(0, this, SLOT(init()));
#endif
}

KIPIInterface::~KIPIInterface()
{
    delete d;
}

static bool actionLessThan(QAction* a1, QAction* a2)
{
    QString a1Text = a1->text().replace('&', QString());
    QString a2Text = a2->text().replace('&', QString());
    return QString::compare(a1Text, a2Text, Qt::CaseInsensitive) < 0;
}

void KIPIInterface::loadPlugins()
{
    // Already done
    if (d->mPluginLoader) {
        return;
    }

    d->mMenuInfoMap[KIPI::ImagesPlugin]      = MenuInfo(i18nc("@title:menu", "Images"));
    d->mMenuInfoMap[KIPI::ToolsPlugin]       = MenuInfo(i18nc("@title:menu", "Tools"));
    d->mMenuInfoMap[KIPI::ImportPlugin]      = MenuInfo(i18nc("@title:menu", "Import"));
    d->mMenuInfoMap[KIPI::ExportPlugin]      = MenuInfo(i18nc("@title:menu", "Export"));
    d->mMenuInfoMap[KIPI::BatchPlugin]       = MenuInfo(i18nc("@title:menu", "Batch Processing"));
    d->mMenuInfoMap[KIPI::CollectionsPlugin] = MenuInfo(i18nc("@title:menu", "Collections"));

    d->mPluginLoader = new KIPI::PluginLoader();
    d->mPluginLoader->setInterface(this);
    d->mPluginLoader->init();
    d->mPluginQueue = d->mPluginLoader->pluginList();
    d->mPluginMenu->addAction(d->mLoadingAction);
    loadOnePlugin();
}

void KIPIInterface::loadOnePlugin()
{
    while (!d->mPluginQueue.isEmpty()) {
        KIPI::PluginLoader::Info* pluginInfo = d->mPluginQueue.takeFirst();
        if (!pluginInfo->shouldLoad()) {
            continue;
        }

        KIPI::Plugin* plugin = pluginInfo->plugin();
        if (!plugin) {
            qWarning() << "Plugin from library" << pluginInfo->library() << "failed to load";
            continue;
        }

        plugin->setup(d->mMainWindow);
        QList<QAction *> actions = plugin->actions();
        Q_FOREACH(QAction * action, actions) {
            KIPI::Category category = plugin->category(action);

            if (!d->mMenuInfoMap.contains(category)) {
                qWarning() << "Unknown category '" << category;
                continue;
            }

            d->mMenuInfoMap[category].mActions << action;
        }
        // FIXME: Port
        //plugin->actionCollection()->readShortcutSettings();

        // If we reach this point, we just loaded one plugin. Go back to the
        // event loop. We will come back to load the remaining plugins or create
        // the menu later
        QMetaObject::invokeMethod(this, "loadOnePlugin", Qt::QueuedConnection);
        return;
    }

    // If we reach this point, all plugins have been loaded. We can fill the
    // menu
    MenuInfoMap::Iterator
    it = d->mMenuInfoMap.begin(),
    end = d->mMenuInfoMap.end();
    for (; it != end; ++it) {
        MenuInfo& info = it.value();
        if (!info.mActions.isEmpty()) {
            QMenu* menu = d->mPluginMenu->addMenu(info.mName);
            qSort(info.mActions.begin(), info.mActions.end(), actionLessThan);
            Q_FOREACH(QAction * action, info.mActions) {
                menu->addAction(action);
            }
        }
    }

    if (d->mPluginWatcher) {
        delete d->mPluginWatcher;
    }
    d->mPluginMenu->removeAction(d->mLoadingAction);
    if (d->mPluginMenu->isEmpty()) {
        d->mPluginMenu->addAction(d->mNoPluginAction);
#ifdef KIPI_INSTALLER
        d->mPluginMenu->addAction(d->mInstallPluginAction);
        d->mInstallPluginAction->setEnabled(true);
        QObject::connect(d->mInstallPluginAction, SIGNAL(triggered(bool)),
                         this, SLOT(slotInstallPlugins(bool)));
        d->mPluginWatcher = new QFileSystemWatcher(d->mMainWindow);
        d->mPluginWatcher->addPaths(QCoreApplication::libraryPaths());
        connect(d->mPluginWatcher, SIGNAL(directoryChanged(QString)), SLOT(packageFinished()));
#endif
    }

    loadingFinished();
}

#ifdef KIPI_INSTALLER
void KIPIInterface::slotInstallPlugins(bool checked) {
    Q_UNUSED(checked);
    QDesktopServices::openUrl(QUrl("appstream://photolayoutseditor.desktop"));
}

void KIPIInterface::packageFinished() {
    if (d->mPluginLoader) {
        delete d->mPluginLoader;
    }
    d->mPluginLoader = 0;
    d->mPluginMenu->removeAction(d->mInstallPluginAction);
    d->mPluginMenu->removeAction(d->mNoPluginAction);
    QTimer::singleShot(5000, this, SLOT(loadPlugins()));
}

#endif

QList<QAction*> KIPIInterface::pluginActions(KIPI::Category category) const
{
    const_cast<KIPIInterface*>(this)->loadPlugins();

    if (isLoadingFinished()) {
        QList<QAction*> list = d->mMenuInfoMap.value(category).mActions;
        if (list.isEmpty()) {
            list << d->mNoPluginAction;
        }
        return list;
    } else {
        return QList<QAction*>() << d->mLoadingAction;
    }
}

bool KIPIInterface::isLoadingFinished() const
{
    if (!d->mPluginLoader) {
        // Not even started
        return false;
    }
    return d->mPluginQueue.isEmpty();
}

void KIPIInterface::init()
{
    slotDirectoryChanged();
    slotSelectionChanged();
}

KIPI::ImageCollection KIPIInterface::currentAlbum()
{
    LOG("");
    const ContextManager* contextManager = d->mMainWindow->contextManager();
    const QUrl url = contextManager->currentDirUrl();
    const SortedDirModel* model = contextManager->dirModel();

    QList<QUrl> list;
    const int count = model->rowCount();
    for (int row = 0; row < count; ++row) {
        const QModelIndex& index = model->index(row, 0);
        const KFileItem item = model->itemForIndex(index);
        if (MimeTypeUtils::fileItemKind(item) == MimeTypeUtils::KIND_RASTER_IMAGE) {
            list << item.targetUrl();
        }
    }

    return KIPI::ImageCollection(new ImageCollection(url, url.fileName(), list));
}

KIPI::ImageCollection KIPIInterface::currentSelection()
{
    LOG("");

    KFileItemList fileList = d->mMainWindow->contextManager()->selectedFileItemList();
    QList<QUrl> list = fileList.urlList();
    QUrl url = d->mMainWindow->contextManager()->currentUrl();

    return KIPI::ImageCollection(new ImageCollection(url, url.fileName(), list));
}

QList<KIPI::ImageCollection> KIPIInterface::allAlbums()
{
    LOG("");
    QList<KIPI::ImageCollection> list;
    list << currentAlbum() << currentSelection();
    return list;
}

KIPI::ImageInfo KIPIInterface::info(const QUrl &url)
{
    LOG("");
    return KIPI::ImageInfo(new KIPIImageInfo(this, url));
}

int KIPIInterface::features() const
{
    return KIPI::HostAcceptNewImages;
}

/**
 * KDirLister will pick up the image if necessary, so no updating is needed
 * here, it is however necessary to discard caches if the plugin preserves timestamp
 */
bool KIPIInterface::addImage(const QUrl&, QString&)
{
//TODO  setContext(const QUrl &currentUrl, const KFileItemList& selection)?
    //Cache::instance()->invalidate( url );
    return true;
}

void KIPIInterface::delImage(const QUrl&)
{
//TODO
}

void KIPIInterface::refreshImages(const QList<QUrl>&)
{
// TODO
}

KIPI::ImageCollectionSelector* KIPIInterface::imageCollectionSelector(QWidget *parent)
{
    return new KIPIImageCollectionSelector(this, parent);
}

KIPI::UploadWidget* KIPIInterface::uploadWidget(QWidget *parent)
{
    return (new KIPIUploadWidget(this, parent));
}

void KIPIInterface::slotSelectionChanged()
{
    emit selectionChanged(!d->mMainWindow->contextManager()->selectedFileItemList().isEmpty());
}

void KIPIInterface::slotDirectoryChanged()
{
    emit currentAlbumChanged(true);
}

#ifdef GWENVIEW_KIPI_WITH_CREATE_METHODS
KIPI::FileReadWriteLock* KIPIInterface::createReadWriteLock(const QUrl& url) const
{
    return NULL;
}

KIPI::MetadataProcessor* KIPIInterface::createMetadataProcessor() const
{
    return NULL;
}

#ifdef GWENVIEW_KIPI_WITH_CREATE_RAW_PROCESSOR
KIPI::RawProcessor* KIPIInterface::createRawProcessor() const
{
    return NULL;
}
#endif
#endif


} //namespace
