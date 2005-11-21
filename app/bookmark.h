// vim: set tabstop=4 shiftwidth=4 noexpandtab:
// kate: indent-mode csands; indent-width 4; replace-tabs-save off; replace-tabs off; replace-trailing-space-save off; space-indent off; tabs-indents on; tab-width 4;
/*
Gwenview - A simple image viewer for KDE
Copyright 2005 Aurelien Gateau

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
#ifndef BOOKMARK_H
#define BOOKMARK_H   

class QString;
class KConfig;
class KURL;

namespace Gwenview {

class Bookmark {
public:
	Bookmark();
	~Bookmark();

	void initFromURL(const KURL&);

	void setIcon(const QString&);
	QString icon() const;
	void setTitle(const QString&);
	QString title() const;
	void setURL(const KURL&);
	KURL url() const;

	void readConfig(KConfig*, int pos);
	void writeConfig(KConfig*, int pos) const;

private:
	class Private;
	Private* d;
};


} // namespace

#endif /* BOOKMARK_H */
