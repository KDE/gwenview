/* This file is part of the KDE libraries
    Copyright (C) 1999 Reginald Stadlbauer <reggie@kde.org>
              (C) 1999 Simon Hausmann <hausmann@kde.org>
              (C) 2000 Nicolas Hadacek <haadcek@kde.org>
              (C) 2000 Kurt Granroth <granroth@kde.org>
              (C) 2000 Michael Koch <koch@kde.org>
	      (C) 2001 Holger Freyther <freyther@kde.org>
              (C) 2002 Ellis Whitehead <ellis@kde.org>
              (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

#include <kdeversion.h>
#if KDE_VERSION < 0x30100

#include "kwidgetaction.h"
/*
#include "kactionshortcutlist.h"

#include <assert.h>

#include <qfontdatabase.h>
#include <qobjectlist.h>
#include <qptrdict.h>
#include <qregexp.h>
#include <qtl.h>
#include <qtooltip.h>
#include <qvariant.h>
#include <qwhatsthis.h>
#include <qtimer.h>

#include <kaccel.h>
#include <kaccelbase.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kfontcombo.h>
#include <kglobalsettings.h>
#include <kguiitem.h>
#include <kiconloader.h>
#include <kmainwindow.h>
#include <kmenubar.h>
#include <kpopupmenu.h>
#include <kstdaccel.h>
#include <ktoolbarbutton.h>
#include <kurl.h>
#include <klocale.h>
*/ // FIXME
#include <kapplication.h>
#include <kdebug.h>
#include <ktoolbar.h>

KWidgetAction::KWidgetAction( QWidget* widget,
    const QString& text, const KShortcut& cut,
    const QObject* receiver, const char* slot,
    KActionCollection* parent, const char* name )
  : KAction( text, cut, receiver, slot, parent, name )
  , m_widget( widget )
  , m_autoSized( false )
{
}

KWidgetAction::~KWidgetAction()
{
}

void KWidgetAction::setAutoSized( bool autoSized )
{
  if( m_autoSized == autoSized )
    return;

  m_autoSized = autoSized;

  if( !m_widget || !isPlugged() )
    return;

  KToolBar* toolBar = (KToolBar*)m_widget->parent();
  int i = findContainer( toolBar );
  if ( i == -1 )
    return;
  int id = itemId( i );

  toolBar->setItemAutoSized( id, m_autoSized );
}

int KWidgetAction::plug( QWidget* w, int index )
{
  if (kapp && !kapp->authorizeKAction(name()))
      return -1;

  if ( !w->inherits( "KToolBar" ) ) {
    kdError() << "KWidgetAction::plug: KWidgetAction must be plugged into KToolBar." << endl;
    return -1;
  }
  if ( !m_widget ) {
    kdError() << "KWidgetAction::plug: Widget was deleted or null!" << endl;
    return -1;
  }

  KToolBar* toolBar = static_cast<KToolBar*>( w );

  int id = KAction::getToolButtonID();

  m_widget->reparent( toolBar, QPoint() );
  toolBar->insertWidget( id, 0, m_widget, index );
  toolBar->setItemAutoSized( id, m_autoSized );

  addContainer( toolBar, id );

  connect( toolBar, SIGNAL( toolbarDestroyed() ), this, SLOT( slotToolbarDestroyed() ) );
  connect( toolBar, SIGNAL( destroyed() ), this, SLOT( slotDestroyed() ) );

  return containerCount() - 1;
}

void KWidgetAction::unplug( QWidget *w )
{
  // ### shouldn't this method check if w == m_widget->parent() ? (Simon)
  if( !m_widget || !isPlugged() )
    return;

  KToolBar* toolBar = (KToolBar*)m_widget->parent();
  disconnect( toolBar, SIGNAL( toolbarDestroyed() ), this, SLOT( slotToolbarDestroyed() ) );

  m_widget->reparent( 0L, QPoint(), false /*showIt*/ );

  KAction::unplug( w );
}

void KWidgetAction::slotToolbarDestroyed()
{
  Q_ASSERT( m_widget );
  Q_ASSERT( isPlugged() );
  if( !m_widget || !isPlugged() )
    return;

  // Don't let a toolbar being destroyed, delete my widget.
  m_widget->reparent( 0L, QPoint(), false /*showIt*/ );
}

#include "kwidgetaction.moc"


#endif // KDE_VERSION
