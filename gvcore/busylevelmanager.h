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
#ifndef BUSYLEVELMANAGER_H
#define BUSYLEVELMANAGER_H

// Qt
#include <qtimer.h>
namespace Gwenview {

// KDE
#include "libgwenview_export.h"

/*
 Busy level of the application.
 Sorted by increasing priority.
*/
enum BusyLevel {
	BUSY_NONE,
	BUSY_THUMBNAILS,
	BUSY_PRELOADING,
	BUSY_LOADING,
	BUSY_SMOOTHING,
	BUSY_PAINTING,
	BUSY_CHECKING_NEW_IMAGE
};

class LIBGWENVIEW_EXPORT BusyLevelManager : public QObject {
Q_OBJECT
public:
	static BusyLevelManager* instance();

	/**
	 * Announces that the given object is busy.
	 */
	void setBusyLevel( QObject* obj, BusyLevel level );

	/**
	 * Returns the busy level of the whole application (i.e. maximum).
	 */
	BusyLevel busyLevel() const;

signals:
	/**
	 * When emitted, operations that are less important than current level
	 * should be suspended until the level decreases to their level.
	 * E.g. when loading a picture thumbnail generation should get suspended.
	 */
	void busyLevelChanged( BusyLevel level );

private slots:
	void delayedBusyLevelChanged();
	void objectDestroyed( QObject* obj );

private:
	BusyLevelManager();
	QMap< QObject*, BusyLevel > mBusyLevels;
	BusyLevel mCurrentBusyLevel;
	QTimer mDelayedBusyLevelTimer;
};


/**
  Helper class. Constructor sets its busy level to the given level,
  destructor resets the busy level to none.
 */
class BusyLevelHelper : public QObject {
Q_OBJECT
public:
	BusyLevelHelper( BusyLevel level );
	~BusyLevelHelper();
	void reset();
};

inline
BusyLevelHelper::BusyLevelHelper( BusyLevel level )
{
	BusyLevelManager::instance()->setBusyLevel( this, level );
}

inline
void BusyLevelHelper::reset()
{
	BusyLevelManager::instance()->setBusyLevel( this, BUSY_NONE );
}

inline
BusyLevelHelper::~BusyLevelHelper()
{
	reset();
}


} // namespace
#endif

