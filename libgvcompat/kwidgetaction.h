// This is a backport of KWidgetAction from 3.1 to 3.0
/* This file is part of the KDE libraries
    Copyright (C) 1999 Reginald Stadlbauer <reggie@kde.org>
              (C) 1999 Simon Hausmann <hausmann@kde.org>
              (C) 2000 Nicolas Hadacek <haadcek@kde.org>
              (C) 2000 Kurt Granroth <granroth@kde.org>
              (C) 2000 Michael Koch <koch@kde.org>
              (C) 2001 Holger Freyther <freyther@kde.org>
              (C) 2002 Ellis Whitehead <ellis@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KWIDGETACTION_H
#define KWIDGETACTION_H 

#include <kdeversion.h>
#if KDE_VERSION < 0X30100

#include <qguardedptr.h>

#include <kaction.h>

/**
 * An action that automatically embeds a widget into a
 * toolbar.
 */
class KWidgetAction : public KAction
{
    Q_OBJECT
public:
    /**
     * Create an action that will embed widget into a toolbar
     * when plugged. This action may only be plugged into
     * a toolbar.
     */
    KWidgetAction( QWidget* widget, const QString& text,
                   const KShortcut& cut,
                   const QObject* receiver, const char* slot,
                   KActionCollection* parent, const char* name );
    virtual ~KWidgetAction();

    /**
     * Returns the widget associated with this action.
     */
    QWidget* widget() { return m_widget; }

    void setAutoSized( bool );

    /**
     * Plug the action. The widget passed to the constructor
     * will be reparented to w, which must inherit KToolBar.
     */
    virtual int plug( QWidget* w, int index = -1 );
    /**
     * Unplug the action. Ensures that the action is not
     * destroyed. It will be hidden and reparented to 0L instead.
     */
    virtual void unplug( QWidget *w );
protected slots:
    void slotToolbarDestroyed();
private:
    QGuardedPtr<QWidget> m_widget;
    bool                 m_autoSized;
    class KWidgetActionPrivate;
    KWidgetActionPrivate *d;
};

#endif // KDE_VERSION

#endif
