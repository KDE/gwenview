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
// Self
#include "bookmark.h"

// KDE
#include <kmimetype.h>

namespace Gwenview {

const QString CONFIG_URL="URL_%1";
const QString CONFIG_DESCRIPTION="Description_%1";
const QString CONFIG_ICON="Icon_%1";
// Not used for now, but we want to be compatible with KURLBar
const QString CONFIG_ICON_GROUP="IconGroup_%1";


struct Bookmark::Private {
	KURL mURL;
	QString mIcon;
	QString mTitle;
};


Bookmark::Bookmark() {
	d=new Private;
}


Bookmark::~Bookmark() {
	delete d;
}


void Bookmark::initFromURL(const KURL& url) {
	d->mURL=url;
	d->mIcon=KMimeType::iconForURL(url);
	d->mTitle=url.fileName();
}


QString Bookmark::icon() const {
	return d->mIcon;
}


void Bookmark::setIcon(const QString& icon) {
	d->mIcon=icon;
}


QString Bookmark::title() const {
	return d->mTitle;
}


void Bookmark::setTitle(const QString& title) {
	d->mTitle=title;
}


KURL Bookmark::url() const {
	return d->mURL;
}


void Bookmark::setURL(const KURL& url) {
	d->mURL=url;
}


void Bookmark::readConfig(KConfig* config, int pos) {
	d->mURL=config->readPathEntry(CONFIG_URL.arg(pos));
	
	d->mTitle=config->readEntry( CONFIG_DESCRIPTION.arg(pos) );
	if (d->mTitle.isEmpty()) d->mTitle=d->mURL.fileName();
	
	d->mIcon=config->readEntry( CONFIG_ICON.arg(pos) );
	if (d->mIcon.isEmpty()) d->mIcon=KMimeType::iconForURL(d->mURL);
}


void Bookmark::writeConfig(KConfig* config, int pos) const {
	config->writePathEntry(CONFIG_URL.arg(pos), d->mURL.prettyURL());

	QString title=d->mTitle;
	if (title==d->mURL.fileName()) title=QString::null;
	config->writeEntry(CONFIG_DESCRIPTION.arg(pos), title);

	QString icon=d->mIcon;
	if (icon==KMimeType::iconForURL(d->mURL)) icon=QString::null;
	config->writeEntry(CONFIG_ICON.arg(pos), icon);
	
	config->writeEntry(CONFIG_ICON_GROUP.arg(pos), KIcon::Panel);
}

	
} // namespace
