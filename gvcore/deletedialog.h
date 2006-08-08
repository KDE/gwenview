/***************************************************************************
    begin                : Tue Aug 31 21:54:20 EST 2004
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

#ifndef _DELETEDIALOG_H
#define _DELETEDIALOG_H


#include <kdialogbase.h>

class DeleteDialogBase;
class KGuiItem;

namespace Gwenview {

class DeleteDialog : public KDialogBase
{
    Q_OBJECT

public:
    DeleteDialog(QWidget *parent, const char *name = "delete_dialog");

    void setURLList(const KURL::List &files);
    bool shouldDelete() const;

    QSize sizeHint() const;

protected slots:
    virtual void accept();

private slots:
    void updateUI();

private:
    DeleteDialogBase *m_widget;
    KGuiItem m_trashGuiItem;
};

} // namespace

#endif

// vim: set et ts=4 sw=4:
