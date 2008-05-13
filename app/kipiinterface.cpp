// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau
          2008      Angelo Naselli
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "kipiinterface.moc"

// Qt
#include <qlist.h>
#include <QMenu>
#include <QRegExp>

// KDE
#include <kdebug.h>
#include <kurl.h>
#include <kxmlguifactory.h>
#include <kdirlister.h>

// KIPI
#include <libkipi/imagecollectionshared.h>
#include <libkipi/imageinfo.h>
#include <libkipi/imageinfoshared.h>
#include <libkipi/imagecollectionshared.h>
#include <libkipi/plugin.h>
#include <libkipi/pluginloader.h>

// local
#include "mainwindow.h"
#include "contextmanager.h"
#include <lib/jpegcontent.h>
#include <lib/metadata/sorteddirmodel.h>

namespace Gwenview {
#undef ENABLE_LOG
#undef LOG

//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

class ImageCollection : public KIPI::ImageCollectionShared {
public:
	ImageCollection(KUrl dirURL, const QString& name, const KUrl::List& images)
	: KIPI::ImageCollectionShared()
    , mDirURL(dirURL)
    , mName(name)
    , mImages(images) {}

	QString name()           { return mName; }
	QString comment()        { return QString::null; }
	KUrl::List images()      { return mImages; }
	KUrl uploadRoot()        { return KUrl("/"); }
	KUrl uploadPath()        { return mDirURL; }
	QString uploadRootName() { return "/"; }
	bool isDirectory()       { return true; }

private:
    KUrl mDirURL;
	QString mName;
	KUrl::List mImages;
};

class KIPIImageInfo : public KIPI::ImageInfoShared {
	static const QRegExp sExtensionRE;
public:
	KIPIImageInfo(KIPI::Interface* interface, const KUrl& url) : KIPI::ImageInfoShared(interface, url) {}

	QString title() {
		QString txt=_url.fileName();
		txt.replace("_", " ");
		txt.replace(sExtensionRE, "");
		return txt;
	}

	QString description() {
		if (!_url.isLocalFile()) return QString::null;

		JpegContent content;
		bool ok=content.load(_url.path());
		if (!ok) return QString::null;

		return content.comment();
	}

	void setDescription(const QString&) {}

	QMap<QString,QVariant> attributes() {
		return QMap<QString,QVariant>();
	}

	void delAttributes( const QStringList& ) {}

	void clearAttributes() {}

	void addAttributes(const QMap<QString, QVariant>&) {}
};

const QRegExp KIPIImageInfo::sExtensionRE("\\.[a-z0-9]+$", Qt::CaseInsensitive );


struct KIPIInterfacePrivate {
	KIPIInterface* that;
	MainWindow* mMainWindow;
	KIPI::PluginLoader* mPluginLoader;

	void setupPluginsMenu() {
		QMenu* menu = static_cast<QMenu*>(
			mMainWindow->factory()->container("plugins", mMainWindow));
		QObject::connect(menu, SIGNAL(aboutToShow()),
			that, SLOT(loadPlugins()) );
	}
};

KIPIInterface::KIPIInterface(MainWindow* mainWindow)
:KIPI::Interface(mainWindow)
, d(new KIPIInterfacePrivate) {
	d->that = this;
	d->mMainWindow = mainWindow;
	d->mPluginLoader = 0;

	d->setupPluginsMenu();
	QObject::connect(d->mMainWindow->contextManager(), SIGNAL(selectionChanged()),
		this, SLOT(slotSelectionChanged()) );
	QObject::connect(d->mMainWindow->contextManager(), SIGNAL(currentDirUrlChanged()),
		this, SLOT(slotDirectoryChanged()) );
#if 0
//TODO instead of delaying can we load them all at start-up to use actions somewhere else?
// delay a bit, so that it's called after loadPlugins()
	QTimer::singleShot( 0, this, SLOT( init()));
#endif
}


KIPIInterface::~KIPIInterface() {
	delete d;
}


void KIPIInterface::loadPlugins() {
	// Already done
	if (d->mPluginLoader) {
		return;
	}

	d->mPluginLoader = new KIPI::PluginLoader(QStringList(), this);
	connect(d->mPluginLoader, SIGNAL(replug()), SLOT( slotReplug()) );
	d->mPluginLoader->loadPlugins();
}


// Helper class for slotReplug(), gcc does not want to instantiate templates
// with local classes, so this is declared outside of slotReplug()
struct MenuInfo {
	QString mName;
	QList<QAction*> mActions;
	MenuInfo() {}
	MenuInfo(const QString& name) : mName(name) {}
};

void KIPIInterface::slotReplug() {
	typedef QMap<KIPI::Category, MenuInfo> CategoryMap;
	CategoryMap categoryMap;
	categoryMap[KIPI::IMAGESPLUGIN]=MenuInfo("image_actions");
	categoryMap[KIPI::EFFECTSPLUGIN]=MenuInfo("effect_actions");
	categoryMap[KIPI::TOOLSPLUGIN]=MenuInfo("tool_actions");
	categoryMap[KIPI::IMPORTPLUGIN]=MenuInfo("import_actions");
	categoryMap[KIPI::EXPORTPLUGIN]=MenuInfo("export_actions");
	categoryMap[KIPI::BATCHPLUGIN]=MenuInfo("batch_actions");
	categoryMap[KIPI::COLLECTIONSPLUGIN]=MenuInfo("collection_actions");

	// Fill the mActions
	KIPI::PluginLoader::PluginList pluginList = d->mPluginLoader->pluginList();
	Q_FOREACH(KIPI::PluginLoader::Info* pluginInfo, pluginList) {
		if (!pluginInfo->shouldLoad()) {
			continue;
		}
		KIPI::Plugin* plugin = pluginInfo->plugin();
		if (!plugin) {
			kWarning() << "Plugin from library" << pluginInfo->library() << "failed to load";
			continue;
		}

		plugin->setup(d->mMainWindow);
		QList<KAction*> actions = plugin->actions();
		Q_FOREACH(KAction* action, actions) {
			KIPI::Category category = plugin->category(action);

			if (!categoryMap.contains(category)) {
				kWarning() << "Unknown category '" << category;
				continue;
			}

			categoryMap[category].mActions << action;
		}
		// FIXME: Port
		//plugin->actionCollection()->readShortcutSettings();
	}

	// Create a dummy "no plugin" action list
	KAction* noPluginAction = d->mMainWindow->actionCollection()->add<KAction>("no_plugin");
	noPluginAction->setText(i18n("No Plugin"));
	noPluginAction->setShortcutConfigurable(false);
	noPluginAction->setEnabled(false);
	QList<QAction*> noPluginList;
	noPluginList << noPluginAction;

	// Fill the menu
	Q_FOREACH(const MenuInfo& info, categoryMap) {
		d->mMainWindow->unplugActionList(info.mName);
		if (info.mActions.count()>0) {
			d->mMainWindow->plugActionList(info.mName, info.mActions);
		} else {
			d->mMainWindow->plugActionList(info.mName, noPluginList);
		}
	}
}


void KIPIInterface::init() {
	slotDirectoryChanged();
	slotSelectionChanged();
}

KIPI::ImageCollection KIPIInterface::currentAlbum() {
	LOG("");
	KUrl url = d->mMainWindow->contextManager()->currentDirUrl();

	KFileItemList fileList =
				d->mMainWindow->contextManager()->dirModel()->dirLister()->itemsForDir(url);
	KUrl::List list = fileList.urlList();

	return KIPI::ImageCollection(new ImageCollection(url, url.fileName(), list));
}


KIPI::ImageCollection KIPIInterface::currentSelection() {
	LOG("");

	KFileItemList fileList = d->mMainWindow->contextManager()->selection();
	KUrl::List list = fileList.urlList();
	KUrl url = d->mMainWindow->contextManager()->currentUrl();
	
	return KIPI::ImageCollection(new ImageCollection(url, url.fileName(), list));
}


QList<KIPI::ImageCollection> KIPIInterface::allAlbums() {
	LOG("");
	QList<KIPI::ImageCollection> list;
	list << currentAlbum() << currentSelection();
	return list;
}


KIPI::ImageInfo KIPIInterface::info(const KUrl& url) {
	LOG("");
	return KIPI::ImageInfo( new KIPIImageInfo(this, url) );
}

int KIPIInterface::features() const {
	return KIPI::HostAcceptNewImages;
}

/**
 * KDirLister will pick up the image if necessary, so no updating is needed
 * here, it is however necessary to discard caches if the plugin preserves timestamp
 */
bool KIPIInterface::addImage(const KUrl&, QString&) {
//TODO	setContext(const KUrl& currentUrl, const KFileItemList& selection)?
	//Cache::instance()->invalidate( url );
	return true;
}

void KIPIInterface::delImage(const KUrl&) {
//TODO
}

void KIPIInterface::refreshImages( const KUrl::List&) {
// TODO
}

KIPI::ImageCollectionSelector* KIPIInterface::imageCollectionSelector(QWidget *parent) {
	return 0;
}

KIPI::UploadWidget* KIPIInterface::uploadWidget(QWidget *parent) {
	return 0;
}

void KIPIInterface::slotSelectionChanged() {
	emit selectionChanged(d->mMainWindow->contextManager()->selection().count() >0);
}


void KIPIInterface::slotDirectoryChanged() {
// TODO check if getting it from selected
	emit currentAlbumChanged(d->mMainWindow->contextManager()->selection().count() >0);
}

} //namespace
