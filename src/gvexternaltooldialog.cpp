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
#include <kdesktopfile.h>
#include <kicondialog.h>
#include <kiconloader.h>
#include <kimageio.h>
#include <klineedit.h>
#include <klistview.h>
#include <kurlrequester.h>

// Local
#include "gvexternaltoolmanager.h"
#include "gvexternaltooldialogbase.h"
#include "gvexternaltooldialog.moc"


class ToolListViewItem : public KListViewItem {
public:
	ToolListViewItem(KListView* parent, const QString& label)
	: KListViewItem(parent, label) {}
	
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
};


GVExternalToolDialog::GVExternalToolDialog(QWidget* parent)
: KDialogBase(
	parent,0,true,QString::null,KDialogBase::Ok|KDialogBase::Apply|KDialogBase::Cancel,
	KDialogBase::Ok,true)
{
	d=new GVExternalToolDialogPrivate;
	
	d->mContent=new GVExternalToolDialogBase(this);
	setMainWidget(d->mContent);
	setCaption(d->mContent->caption());

	d->mContent->mToolListView->header()->hide();
	d->mContent->mMimeTypeListView->header()->hide();

	d->fillMimeTypeListView();
	d->fillToolListView();

	connect( d->mContent->mToolListView, SIGNAL(selectionChanged()),
		this, SLOT(updateDetails()) );
}


GVExternalToolDialog::~GVExternalToolDialog() {
	delete d;
}


void GVExternalToolDialog::slotOk() {
	slotApply();
	accept();
}


void GVExternalToolDialog::slotApply() {
}


void GVExternalToolDialog::slotCancel() {
	KDialogBase::slotCancel();
}


void GVExternalToolDialog::updateDetails() {
	ToolListViewItem* item=static_cast<ToolListViewItem*>(d->mContent->mToolListView->selectedItem());
	if (item) {
		KDesktopFile* desktopFile=item->desktopFile();
		d->mContent->mName->setText(desktopFile->readName());
		d->mContent->mCommand->setURL(desktopFile->readEntry("Exec"));
		d->mContent->mIconButton->setIcon(desktopFile->readIcon());
	} else {
		d->mContent->mName->setText(QString::null);
		d->mContent->mCommand->setURL(QString::null);
		d->mContent->mIconButton->setIcon(QString::null);
	}
}
