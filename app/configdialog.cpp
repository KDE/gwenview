// vim: set tabstop=4 shiftwidth=4 noexpandtab
// kate: indent-mode csands; indent-width 4; replace-tabs-save off; replace-tabs off; replace-trailing-space-save off; space-indent off; tabs-indents on; tab-width 4;
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau

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
// Qt 
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmap.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qstylesheet.h>
#include <qtextedit.h>

// KDE
#include <kcolorbutton.h>
#include <kconfigdialogmanager.h>
#include <kdeversion.h>
#include <kdirsize.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kurllabel.h>
#include <kurlrequester.h>

#include <config.h>
// KIPI
#ifdef GV_HAVE_KIPI
#include <libkipi/pluginloader.h>
#endif

// Local 
#include "configfileoperationspage.h"
#include "configfullscreenpage.h"
#include "configimagelistpage.h"
#include "configimageviewpage.h"
#include "configmiscpage.h"
#include "configslideshowpage.h"
#include "gvcore/captionformatter.h"
#include "gvcore/filethumbnailview.h"
// This path is different because it's a generated file, so it's stored in builddir
#include <../gvcore/miscconfig.h>
#include <../gvcore/slideshowconfig.h>
#include <../gvcore/fileoperationconfig.h>
#include <../gvcore/fullscreenconfig.h>
#include <../gvcore/imageviewconfig.h>
#include <../gvcore/fileviewconfig.h>
#include "gvcore/thumbnailloadjob.h"

#include "configdialog.moc"
namespace Gwenview {

typedef QValueList<KConfigDialogManager*> ConfigManagerList;

class ConfigDialogPrivate {
public:
	ConfigImageViewPage* mImageViewPage;
	ConfigImageListPage* mImageListPage;
	ConfigFullScreenPage* mFullScreenPage;
	ConfigFileOperationsPage* mFileOperationsPage;
	ConfigMiscPage* mMiscPage;
	ConfigSlideshowPage* mSlideShowPage;
#ifdef GV_HAVE_KIPI
	KIPI::ConfigWidget* mKIPIConfigWidget;
#endif

	ConfigManagerList mManagers;
};


// Two helper functions to create the config pages
template<class T>
void addConfigPage(KDialogBase* dialog, T* content, const QString& header, const QString& name, const char* iconName) {
	QFrame* page=dialog->addPage(name, header, BarIcon(iconName, 32));
	content->reparent(page, QPoint(0,0));
	QVBoxLayout* layout=new QVBoxLayout(page, 0, KDialog::spacingHint());
	layout->addWidget(content);
	layout->addStretch();
}

template<class T>
T* addConfigPage(KDialogBase* dialog, const QString& header, const QString& name, const char* iconName) {
	T* content=new T;
	addConfigPage(dialog, content, header, name, iconName);
	return content;
}


ConfigDialog::ConfigDialog(QWidget* parent, KIPI::PluginLoader* pluginLoader)
: KDialogBase(
	KDialogBase::IconList,
	i18n("Configure"),
	KDialogBase::Ok | KDialogBase::Cancel | KDialogBase::Apply,
	KDialogBase::Ok,
	parent,
	"ConfigDialog",
	true,
	true)
{
	d=new ConfigDialogPrivate;

	// Create dialog pages
	d->mImageListPage = addConfigPage<ConfigImageListPage>(
		this, i18n("Configure Image List"), i18n("Image List"), "view_icon");
	d->mManagers << new KConfigDialogManager(d->mImageListPage, FileViewConfig::self());
	
	d->mImageViewPage = addConfigPage<ConfigImageViewPage>(
		this, i18n("Configure Image View"), i18n("Image View"), "looknfeel");
	d->mManagers << new KConfigDialogManager(d->mImageViewPage, ImageViewConfig::self());
	
	d->mFullScreenPage = addConfigPage<ConfigFullScreenPage>(
		this, i18n("Configure Full Screen Mode"), i18n("Full Screen"), "window_fullscreen");
	d->mManagers << new KConfigDialogManager(d->mFullScreenPage, FullScreenConfig::self());

	d->mFileOperationsPage = addConfigPage<ConfigFileOperationsPage>(
		this, i18n("Configure File Operations"), i18n("File Operations"), "folder");
	d->mManagers << new KConfigDialogManager(d->mFileOperationsPage, FileOperationConfig::self());

	d->mSlideShowPage = addConfigPage<ConfigSlideshowPage>(
		this, i18n("SlideShow"), i18n("SlideShow"), "slideshow_play");
	d->mManagers << new KConfigDialogManager(d->mSlideShowPage, SlideShowConfig::self());

#ifdef GV_HAVE_KIPI
	Q_ASSERT(pluginLoader);
	d->mKIPIConfigWidget = pluginLoader->configWidget(this);
	addConfigPage(
		this, d->mKIPIConfigWidget, i18n("Configure KIPI Plugins"), i18n("KIPI Plugins"), "kipi");
#else
	// Avoid "unused parameter" warning
	pluginLoader=pluginLoader;
#endif

	d->mMiscPage = addConfigPage<ConfigMiscPage>(
		this, i18n("Miscellaneous Settings"), i18n("Misc"), "gear");
	d->mManagers << new KConfigDialogManager(d->mMiscPage, MiscConfig::self());
	// Read config, because the modified behavior might have changed
	MiscConfig::self()->readConfig();

	// Image List tab
	int details=FileViewConfig::thumbnailDetails();
	d->mImageListPage->mShowFileName->setChecked(details & FileThumbnailView::FILENAME);
	d->mImageListPage->mShowFileDate->setChecked(details & FileThumbnailView::FILEDATE);
	d->mImageListPage->mShowFileSize->setChecked(details & FileThumbnailView::FILESIZE);
	d->mImageListPage->mShowImageSize->setChecked(details & FileThumbnailView::IMAGESIZE);

	connect(d->mImageListPage->mCalculateCacheSize,SIGNAL(clicked()),
		this,SLOT(calculateCacheSize()));
	connect(d->mImageListPage->mEmptyCache,SIGNAL(clicked()),
		this,SLOT(emptyCache()));

	// Image View tab
	d->mImageViewPage->mMouseWheelGroup->setButton(ImageViewConfig::mouseWheelScroll()?1:0);

	// Full Screen tab
	QTextEdit* edit=d->mFullScreenPage->kcfg_osdFormat;
	edit->setMaximumHeight(edit->fontMetrics().height()*3);
	connect(edit, SIGNAL(textChanged()), SLOT(updateOSDPreview()) );

	// File Operations tab
	d->mFileOperationsPage->kcfg_destDir->fileDialog()->setMode(
		static_cast<KFile::Mode>(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly));

	d->mFileOperationsPage->mDeleteGroup->setButton(FileOperationConfig::deleteToTrash()?1:0);
	
	ConfigManagerList::Iterator it(d->mManagers.begin());
	for (;it!=d->mManagers.end(); ++it) {
		(*it)->updateWidgets();
	}
	updateOSDPreview();
}



ConfigDialog::~ConfigDialog() {
	delete d;
}


void ConfigDialog::slotOk() {
	slotApply();
	accept();
}


void ConfigDialog::slotApply() {
	bool needSignal=false;

	// Image List tab
	int details=
		(d->mImageListPage->mShowFileName->isChecked() ? FileThumbnailView::FILENAME : 0)
		| (d->mImageListPage->mShowFileDate->isChecked() ? FileThumbnailView::FILEDATE : 0)
		| (d->mImageListPage->mShowFileSize->isChecked() ? FileThumbnailView::FILESIZE : 0)
		| (d->mImageListPage->mShowImageSize->isChecked() ? FileThumbnailView::IMAGESIZE : 0)
		;
	if (details!=FileViewConfig::thumbnailDetails()) {
		FileViewConfig::setThumbnailDetails(details);
		needSignal=true;
	}
	
	// Image View tab
	ImageViewConfig::setMouseWheelScroll(
		d->mImageViewPage->mMouseWheelGroup->selected()==d->mImageViewPage->mMouseWheelScroll);

	// File Operations tab
	FileOperationConfig::setDeleteToTrash(
		d->mFileOperationsPage->mDeleteGroup->selected()==d->mFileOperationsPage->mDeleteToTrash);

	// KIPI tab
#ifdef GV_HAVE_KIPI
	d->mKIPIConfigWidget->apply();
#endif
	
	ConfigManagerList::Iterator it(d->mManagers.begin());
	for (;it!=d->mManagers.end(); ++it) {
		if ((*it)->hasChanged()) {
			needSignal=true;
		}
		(*it)->updateSettings();
	}
	if (needSignal) {
		emit settingsChanged();
	}
}


void ConfigDialog::calculateCacheSize() {
	KURL url;
	url.setPath(ThumbnailLoadJob::thumbnailBaseDir());
	unsigned long size=KDirSize::dirSize(url);
	KMessageBox::information( this,i18n("Cache size is %1").arg(KIO::convertSize(size)) );
}


void ConfigDialog::updateOSDPreview() {
	CaptionFormatter formatter;
	KURL url;
	url.setPath(i18n("/path/to/some/image.jpg"));
	formatter.mPath=url.path();
	formatter.mFileName=url.fileName();
	formatter.mComment=i18n("A comment");
	formatter.mImageSize=QSize(1600, 1200);
	formatter.mPosition=4;
	formatter.mCount=12;
	formatter.mAperture="F2.8";
	formatter.mExposureTime="1/60 s";
	formatter.mIso="100";
	formatter.mFocalLength="8.88 mm";
	
	QString txt=formatter.format( d->mFullScreenPage->kcfg_osdFormat->text() );
	d->mFullScreenPage->mOSDPreviewLabel->setText(txt);
}


void ConfigDialog::emptyCache() {
	QString dir=ThumbnailLoadJob::thumbnailBaseDir();

	if (!QFile::exists(dir)) {
		KMessageBox::information( this,i18n("Cache is already empty.") );
		return;
	}

	int response=KMessageBox::warningContinueCancel(this,
		"<qt>" + i18n("Are you sure you want to empty the thumbnail cache?"
		" This will delete the folder <b>%1</b>.").arg(QStyleSheet::escape(dir)) + "</qt>",
		QString::null,
		KStdGuiItem::del());

	if (response==KMessageBox::Cancel) return;

	KURL url;
	url.setPath(dir);
	if (KIO::NetAccess::del(url, topLevelWidget()) ) {
		KMessageBox::information( this,i18n("Cache emptied.") );
	}
}


void ConfigDialog::onCacheEmptied(KIO::Job* job) {
	if ( job->error() ) {
		job->showErrorDialog(this);
		return;
	}
	KMessageBox::information( this,i18n("Cache emptied.") );
}

} // namespace
