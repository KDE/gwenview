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
#ifndef GVBUSYLEVELMANAGER_H
#define GVBUSYLEVELMANAGER_H

// Qt
#include <qtimer.h>

// KDE


/*
 Busy level of the application.
 Sorted by increasing priority.
*/
enum GVBusyLevel {
	BUSY_NONE,
	BUSY_THUMBNAILS,
	BUSY_LOADING,
	BUSY_SMOOTHING,
	BUSY_PAINTING,
	BUSY_CHECKING_NEW_IMAGE
};


class GVBusyLevelManager : public QObject {
Q_OBJECT
public:
	static GVBusyLevelManager* instance();

	/**
	 * Announces that the given object is busy.
	 */
	void setBusyLevel( QObject* obj, GVBusyLevel level );

	/**
	 * Returns the busy level of the whole application (i.e. maximum).
	 */
	GVBusyLevel busyLevel() const;

signals:
	/**
	 * When emitted, operations that are less important than current level
	 * should be suspended until the level decreases to their level.
	 * E.g. when loading a picture thumbnail generation should get suspended.
	 */
	void busyLevelChanged( GVBusyLevel level );

private slots:
	void delayedBusyLevelChanged();

private:
	GVBusyLevelManager();
	QMap< QObject*, GVBusyLevel > mBusyLevels;
	GVBusyLevel mCurrentBusyLevel;
	QTimer mDelayedBusyLevelTimer;
};


/**
  Helper class. Constructor sets its busy level to the given level,
  destructor resets the busy level to none.
 */
class GVBusyLevelHelper : public QObject {
Q_OBJECT
public:
	GVBusyLevelHelper( GVBusyLevel level );
	~GVBusyLevelHelper();
	void reset();
};

inline
GVBusyLevelHelper::GVBusyLevelHelper( GVBusyLevel level )
{
	GVBusyLevelManager::instance()->setBusyLevel( this, level );
}

inline
void GVBusyLevelHelper::reset()
{
	GVBusyLevelManager::instance()->setBusyLevel( this, BUSY_NONE );
}

inline
GVBusyLevelHelper::~GVBusyLevelHelper()
{
	reset();
}


#endif
