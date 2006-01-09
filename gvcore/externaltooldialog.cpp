// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau

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
#include <qheader.h>
#include <qwhatsthis.h>

// KDE
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kicondialog.h>
#include <kiconloader.h>
#include <kimageio.h>
#include <klineedit.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kurllabel.h>
#include <kurlrequester.h>

// Local
#include "archive.h"
#include "mimetypeutils.h"
#include "externaltoolmanager.h"
#include "externaltooldialogbase.h"
#include "externaltooldialog.moc"
namespace Gwenview {


enum { ID_ALL_IMAGES=0, ID_ALL_FILES, ID_CUSTOM };


class ToolListViewItem : public KListViewItem {
public:
	ToolListViewItem(KListView* parent, const QString& label)
	: KListViewItem(parent, label), mDesktopFile(0L) {}
	
	void setDesktopFile(KDesktopFile* df) {
		mDesktopFile=df;
	}

	KDesktopFile* desktopFile() const {
		return mDesktopFile;
	}

private:
	KDesktopFile* mDesktopFile;
};


struct ExternalToolDialogPrivate {
	ExternalToolDialogBase* mContent;
	QPtrList<KDesktopFile> mDeletedTools;
	ToolListViewItem* mSelectedItem;


	ExternalToolDialogPrivate()
	: mSelectedItem(0L) {}
	
	void fillMimeTypeListView() {
		QStringList mimeTypes=MimeTypeUtils::rasterImageMimeTypes();
		mimeTypes.append("inode/directory");
		mimeTypes+=Archive::mimeTypes();

		QStringList::const_iterator it=mimeTypes.begin();
		for(; it!=mimeTypes.end(); ++it) {
			(void)new QCheckListItem(mContent->mMimeTypeListView, *it, QCheckListItem::CheckBox);
		}
	}

	
	void fillToolListView() {
		QDict<KDesktopFile> desktopFiles=ExternalToolManager::instance()->desktopFiles();
		
		QDictIterator<KDesktopFile> it(desktopFiles);
		for (; it.current(); ++it) {
			ToolListViewItem* item=new ToolListViewItem(mContent->mToolListView, it.current()->readName());
			item->setPixmap(0, SmallIcon(it.current()->readIcon()) );
			item->setDesktopFile(it.current());
		}
		mContent->mToolListView->setSortColumn(0);
		mContent->mToolListView->sort();
	}


	void writeServiceTypes(KDesktopFile* desktopFile) {
		QButton* button=mContent->mFileAssociationGroup->selected();
		if (!button) {
			desktopFile->writeEntry("ServiceTypes", "*");
			return;
		}

		int id=mContent->mFileAssociationGroup->id(button);
		if (id==ID_ALL_IMAGES) {
			desktopFile->writeEntry("ServiceTypes", "image/*");
			return;
		}
		if (id==ID_ALL_FILES) {
			desktopFile->writeEntry("ServiceTypes", "*");
			return;
		}

		QStringList mimeTypes;
		QListViewItem* item=mContent->mMimeTypeListView->firstChild();
		for (; item; item=item->nextSibling()) {
			if (static_cast<QCheckListItem*>(item)->isOn()) {
				mimeTypes.append(item->text(0));
			}
		}
		desktopFile->writeEntry("ServiceTypes", mimeTypes);
	}
	

	bool saveChanges() {
		if (!mSelectedItem) return true;

		// Check name
		QString name=mContent->mName->text().stripWhiteSpace();
		if (name.isEmpty()) {
			KMessageBox::sorry(mContent, i18n("The tool name cannot be empty"));
			return false;
		}

		QListViewItem* item=mContent->mToolListView->firstChild();
		for (; item; item=item->nextSibling()) {
			if (item==mSelectedItem) continue;
			if (name==item->text(0)) {
				KMessageBox::sorry(mContent, i18n("There is already a tool named \"%1\"").arg(name));
				return false;
			}
		}
		
		// Save data
		KDesktopFile* desktopFile=mSelectedItem->desktopFile();
		if (desktopFile) {
			if (desktopFile->isReadOnly()) {
				desktopFile=ExternalToolManager::instance()->editSystemDesktopFile(desktopFile);
				mSelectedItem->setDesktopFile(desktopFile);
			}
		} else {		
			desktopFile=ExternalToolManager::instance()->createUserDesktopFile(name);
			mSelectedItem->setDesktopFile(desktopFile);
		}
		desktopFile->writeEntry("Name", name);
		desktopFile->writeEntry("Icon", mContent->mIconButton->icon());
		desktopFile->writeEntry("Exec", mContent->mCommand->url());
		writeServiceTypes(desktopFile);

		mSelectedItem->setPixmap(0, SmallIcon(mContent->mIconButton->icon()) );
		mSelectedItem->setText(0, name);

		return true;
	}


	void updateFileAssociationGroup(const QStringList& serviceTypes) {
		QListViewItem* item=mContent->mMimeTypeListView->firstChild();
		for (; item; item=item->nextSibling()) {
			static_cast<QCheckListItem*>(item)->setOn(false);
		}
		
		if (serviceTypes.size()==0) {
			mContent->mFileAssociationGroup->setButton(ID_ALL_FILES);
			return;
		}
		if (serviceTypes.size()==1) {
			QString serviceType=serviceTypes[0];
			if (serviceType=="image/*") {
				mContent->mFileAssociationGroup->setButton(ID_ALL_IMAGES);
				return;
			}
			if (serviceType=="*") {
				mContent->mFileAssociationGroup->setButton(ID_ALL_FILES);
				return;
			}
		}

		mContent->mFileAssociationGroup->setButton(ID_CUSTOM);
		QStringList::ConstIterator it=serviceTypes.begin();
		for (;it!=serviceTypes.end(); ++it) {
			QListViewItem* item=
				mContent->mMimeTypeListView->findItem(*it, 0, Qt::ExactMatch);
			if (item) static_cast<QCheckListItem*>(item)->setOn(true);
		}
	}
	

	void updateDetails() {
		mContent->mDetails->setEnabled(mSelectedItem!=0);
		
		if (mSelectedItem) {
			KDesktopFile* desktopFile=mSelectedItem->desktopFile();
			if (desktopFile) {
				mContent->mName->setText(desktopFile->readName());
				mContent->mCommand->setURL(desktopFile->readEntry("Exec"));
				mContent->mIconButton->setIcon(desktopFile->readIcon());
				QStringList serviceTypes=desktopFile->readListEntry("ServiceTypes");
				updateFileAssociationGroup(serviceTypes);
				return;
			}
		}

		mContent->mName->setText(QString::null);
		mContent->mCommand->setURL(QString::null);
		mContent->mIconButton->setIcon(QString::null);
		mContent->mFileAssociationGroup->setButton(ID_ALL_IMAGES);
	}
	
	bool apply() {
		if (!saveChanges()) return false;
		QPtrListIterator<KDesktopFile> it(mDeletedTools);
		for(; it.current(); ++it) {
			ExternalToolManager::instance()->hideDesktopFile(it.current());
		}
		ExternalToolManager::instance()->updateServices();
		return true;
	}
};


/**
 * This event filter object is here to prevent the user from selecting a
 * different tool in the tool list view if the current tool could not be saved.
 */
class ToolListViewFilterObject : public QObject {
	ExternalToolDialogPrivate* d;
public:
	ToolListViewFilterObject(QObject* parent, ExternalToolDialogPrivate* _d)
	: QObject(parent), d(_d) {}

	bool eventFilter(QObject*, QEvent* event) {
		if (event->type()!=QEvent::MouseButtonPress) return false;
		return !d->saveChanges();
	}
};


ExternalToolDialog::ExternalToolDialog(QWidget* parent)
: KDialogBase(
	parent,0, false, QString::null, KDialogBase::Ok|KDialogBase::Apply|KDialogBase::Cancel,
	KDialogBase::Ok, true)
{
	setWFlags(getWFlags() | Qt::WDestructiveClose);
	d=new ExternalToolDialogPrivate;
	
	d->mContent=new ExternalToolDialogBase(this);
	setMainWidget(d->mContent);
	setCaption(d->mContent->caption());
	
	d->mContent->mToolListView->header()->hide();
	d->mContent->mMimeTypeListView->header()->hide();

	d->fillMimeTypeListView();
	d->fillToolListView();
	d->mContent->mToolListView->viewport()->installEventFilter(
		new ToolListViewFilterObject(this, d));

	connect( d->mContent->mToolListView, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(slotSelectionChanged(QListViewItem*)) );
	connect( d->mContent->mAddButton, SIGNAL(clicked()),
		this, SLOT(addTool()) );
	connect( d->mContent->mDeleteButton, SIGNAL(clicked()),
		this, SLOT(deleteTool()) );
	connect( d->mContent->mHelp, SIGNAL(leftClickedURL()),
		this, SLOT(showCommandHelp()) );
	connect( d->mContent->mMoreTools, SIGNAL(leftClickedURL(const QString&)),
		this, SLOT(openURL(const QString&)) );

	KListView* view=d->mContent->mToolListView;
	if (view->firstChild()) {
		view->setSelected(view->firstChild(), true);
	}
	d->updateDetails();
}


ExternalToolDialog::~ExternalToolDialog() {
	delete d;
}


void ExternalToolDialog::slotOk() {
	if (!d->apply()) return;
	accept();
}


void ExternalToolDialog::slotApply() {
	d->apply();
}


void ExternalToolDialog::slotCancel() {
	KDialogBase::slotCancel();
}


void ExternalToolDialog::slotSelectionChanged(QListViewItem* item) {
	d->mSelectedItem=static_cast<ToolListViewItem*>(item);
	d->updateDetails();
}


void ExternalToolDialog::addTool() {
	KListView* view=d->mContent->mToolListView;
	QString name=i18n("<Unnamed tool>");
	ToolListViewItem* item=new ToolListViewItem(view, name);
	view->setSelected(item, true);
}


void ExternalToolDialog::deleteTool() {
	KListView* view=d->mContent->mToolListView;
	ToolListViewItem* item=static_cast<ToolListViewItem*>(view->selectedItem());
	if (!item) return;

	KDesktopFile* desktopFile=item->desktopFile();
	delete item;
	d->mDeletedTools.append(desktopFile);
	d->mSelectedItem=0L;
	d->updateDetails();
}


void ExternalToolDialog::showCommandHelp() {
	KURLRequester* lbl=d->mContent->mCommand;
	QWhatsThis::display(QWhatsThis::textFor(lbl),
		lbl->mapToGlobal( lbl->rect().bottomRight() ) );
}


void ExternalToolDialog::openURL(const QString& url) {
	new KRun(KURL(url));
}

} // namespace
