/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aurélien Gâteau

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
#include <qheader.h>

// KDE
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kicondialog.h>
#include <kiconloader.h>
#include <kimageio.h>
#include <klineedit.h>
#include <klistview.h>
#include <klocale.h>
#include <kurlrequester.h>

// Local
#include "gvexternaltoolmanager.h"
#include "gvexternaltooldialogbase.h"
#include "gvexternaltooldialog.moc"


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


struct GVExternalToolDialogPrivate {
	GVExternalToolDialogBase* mContent;
	QPtrList<KDesktopFile> mDeletedTools;
	ToolListViewItem* mSelectedItem;


	GVExternalToolDialogPrivate()
	: mSelectedItem(0L) {}
	
	void fillMimeTypeListView() {
		QStringList mimeTypes=KImageIO::mimeTypes(KImageIO::Reading);
		QStringList::const_iterator it=mimeTypes.begin();
		for(; it!=mimeTypes.end(); ++it) {
			(void)new QCheckListItem(mContent->mMimeTypeListView, *it, QCheckListItem::CheckBox);
		}
	}

	
	void fillToolListView() {
		QDict<KDesktopFile> desktopFiles=GVExternalToolManager::instance()->desktopFiles();
		
		QDictIterator<KDesktopFile> it(desktopFiles);
		for (; it.current(); ++it) {
			ToolListViewItem* item=new ToolListViewItem(mContent->mToolListView, it.current()->readName());
			item->setPixmap(0, SmallIcon(it.current()->readIcon()) );
			item->setDesktopFile(it.current());
		}
	}


	void saveChanges() {
		if (!mSelectedItem) return;

		KDesktopFile* desktopFile=mSelectedItem->desktopFile();
		QString name=mContent->mName->text();
		if (desktopFile) {
			if (desktopFile->isReadOnly()) {
				kdDebug() << "desktopFile->isReadOnly\n";
				desktopFile=GVExternalToolManager::instance()->createUserDesktopFile(desktopFile);
				mSelectedItem->setDesktopFile(desktopFile);
			}
		} else {		
			desktopFile=GVExternalToolManager::instance()->createUserDesktopFile(name);
			mSelectedItem->setDesktopFile(desktopFile);
		}
		desktopFile->writeEntry("Name", name);
		desktopFile->writeEntry("Icon", mContent->mIconButton->icon());
		desktopFile->writeEntry("Exec", mContent->mCommand->url());
		// FIXME: Implement real service type read/write
		desktopFile->writeEntry("ServiceTypes", "*");

		mSelectedItem->setPixmap(0, SmallIcon(mContent->mIconButton->icon()) );
		mSelectedItem->setText(0, name);
	}


	void updateDetails() {
		if (mSelectedItem) {
			KDesktopFile* desktopFile=mSelectedItem->desktopFile();
			if (desktopFile) {
				mContent->mName->setText(desktopFile->readName());
				mContent->mCommand->setURL(desktopFile->readEntry("Exec"));
				mContent->mIconButton->setIcon(desktopFile->readIcon());
				return;
			}
		}

		mContent->mName->setText(QString::null);
		mContent->mCommand->setURL(QString::null);
		mContent->mIconButton->setIcon(QString::null);
	}
};


GVExternalToolDialog::GVExternalToolDialog(QWidget* parent)
: KDialogBase(
	parent,0, false, QString::null, KDialogBase::Ok|KDialogBase::Apply|KDialogBase::Cancel,
	KDialogBase::Ok, true)
{
	setWFlags(getWFlags() | Qt::WDestructiveClose);
	d=new GVExternalToolDialogPrivate;
	
	d->mContent=new GVExternalToolDialogBase(this);
	setMainWidget(d->mContent);
	setCaption(d->mContent->caption());

	d->mContent->mToolListView->header()->hide();
	d->mContent->mMimeTypeListView->header()->hide();

	d->fillMimeTypeListView();
	d->fillToolListView();

	connect( d->mContent->mToolListView, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(slotSelectionChanged(QListViewItem*)) );
	connect( d->mContent->mAddButton, SIGNAL(clicked()),
		this, SLOT(addTool()) );
	connect( d->mContent->mDeleteButton, SIGNAL(clicked()),
		this, SLOT(deleteTool()) );
}


GVExternalToolDialog::~GVExternalToolDialog() {
	delete d;
}


void GVExternalToolDialog::slotOk() {
	slotApply();
	accept();
}


void GVExternalToolDialog::slotApply() {
	d->saveChanges();
	GVExternalToolManager::instance()->updateServices();
}


void GVExternalToolDialog::slotCancel() {
	KDialogBase::slotCancel();
}


void GVExternalToolDialog::slotSelectionChanged(QListViewItem* item) {
	d->saveChanges();
	d->mSelectedItem=static_cast<ToolListViewItem*>(item);
	d->updateDetails();
}


void GVExternalToolDialog::addTool() {
	KListView* view=d->mContent->mToolListView;
	QString name=i18n("<Unnamed tool>");
	ToolListViewItem* item=new ToolListViewItem(view, name);
	view->setSelected(item, true);
	d->mContent->mName->setText(name);
}


void GVExternalToolDialog::deleteTool() {
	KListView* view=d->mContent->mToolListView;
	ToolListViewItem* item=static_cast<ToolListViewItem*>(view->selectedItem());
	if (!item) return;

	KDesktopFile* desktopFile=item->desktopFile();
	delete item;
	d->mDeletedTools.append(desktopFile);
}
