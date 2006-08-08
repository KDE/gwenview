/***************************************************************************
    begin                : Tue Aug 31 21:59:58 EST 2004
    copyright            : (C) 2004 by Michael Pyne <michael.pyne@kdemail.net>
                           (C) 2006 by Ian Monroe <ian@monroe.nu>
                           (C) 2006 by Aurelien Gateau <aurelien.gateau@free.fr>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kconfig.h>
#include <kdeversion.h>
#include <kdialogbase.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <klocale.h>
#include <kstdguiitem.h>
#include <kurl.h>

#include <qstringlist.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qpushbutton.h>

#include "fileoperationconfig.h"
#include "deletedialog.h"
#include "deletedialogbase.h"

namespace Gwenview {

DeleteDialog::DeleteDialog(QWidget *parent, const char *name) :
    KDialogBase(Swallow, WStyle_DialogBorder, parent, name,
        true /* modal */, i18n("About to delete selected files"),
        Ok | Cancel, Cancel /* Default */, true /* separator */),
    m_trashGuiItem(i18n("&Send to Trash"), "trashcan_full")
{
    m_widget = new DeleteDialogBase(this, "delete_dialog_widget");
    setMainWidget(m_widget);
    
    m_widget->setMinimumSize(400, 300);

    actionButton(Ok)->setFocus();

    bool deleteInstead = ! FileOperationConfig::deleteToTrash();
    m_widget->ddShouldDelete->setChecked(deleteInstead);

    connect(m_widget->ddShouldDelete, SIGNAL(toggled(bool)), SLOT(updateUI()));
}

void DeleteDialog::setURLList(const KURL::List &files)
{
    m_widget->ddFileList->clear();
    for( KURL::List::ConstIterator it = files.begin(); it != files.end(); it++) {
        m_widget->ddFileList->insertItem( (*it).pathOrURL() );
    }
    m_widget->ddNumFiles->setText(i18n("<b>1</b> item selected.", "<b>%n</b> items selected.", files.count()));
    updateUI();
}

void DeleteDialog::accept()
{
    FileOperationConfig::setDeleteToTrash( ! shouldDelete() );
    FileOperationConfig::writeConfig();

    KDialogBase::accept();
}


void DeleteDialog::updateUI()
{
    QString msg, iconName;
    
    int fileCount = m_widget->ddFileList->count();
    bool reallyDelete = m_widget->ddShouldDelete->isChecked();

    if(reallyDelete) {
        msg = i18n(
            "<qt>This item will be <b>permanently deleted</b> from your hard disk.</qt>",
            "<qt>These items will be <b>permanently deleted</b> from your hard disk.</qt>",
            fileCount);
        iconName = "messagebox_warning";
    }
    else {
        msg = i18n(
            "<qt>This item will be moved to the trash bin.</qt>",
            "<qt>These items will be moved to the trash bin.</qt>",
            fileCount);
        iconName = "trashcan_full";
    }
    QPixmap icon = KGlobal::iconLoader()->loadIcon(iconName, KIcon::NoGroup, KIcon::SizeMedium);

    m_widget->ddDeleteText->setText(msg);
    m_widget->ddWarningIcon->setPixmap(icon);
    
    setButtonGuiItem(Ok, reallyDelete ? KStdGuiItem::del() : m_trashGuiItem);
    adjustSize();
}


bool DeleteDialog::shouldDelete() const {
    return m_widget->ddShouldDelete->isChecked();
}


QSize DeleteDialog::sizeHint() const {
    m_widget->adjustSize();
    QSize hint = m_widget->minimumSize();
    hint = calculateSize(hint.width(), hint.height());

    // For some reason calculateSize does not return a correct height. As I'm
    // fed up fighting with it, let's just add a few more pixels.
    hint.rheight() += 50;
    return hint;
}
    


} // namespace

#include "deletedialog.moc"

// vim: set et ts=4 sw=4:
