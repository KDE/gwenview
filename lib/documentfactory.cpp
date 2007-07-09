/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "documentfactory.moc"

// Qt
#include <QMap>

// KDE
#include <KUrl>

namespace Gwenview {

typedef QMap<KUrl, Document::Ptr> DocumentMap;
struct DocumentFactoryPrivate {
	DocumentMap mDocumentMap;

	/**
	 * Removes items in mDocumentMap which are no longer referenced elsewhere
	 */
	void garbageCollect() {
		DocumentMap::Iterator
			it = mDocumentMap.begin(),
			end = mDocumentMap.end();

		while (it!=end) {
			Document::Ptr doc = it.value();
			if (doc.count() == 1 && !doc->isModified()) {
				it = mDocumentMap.erase(it);
			} else {
				++it;
			}
		}
	}
};

DocumentFactory::DocumentFactory()
: d(new DocumentFactoryPrivate) {
}

DocumentFactory::~DocumentFactory() {
	delete d;
}

DocumentFactory* DocumentFactory::instance() {
	static DocumentFactory factory;
	return &factory;
}

Document::Ptr DocumentFactory::load(const KUrl& url) {
	DocumentMap::ConstIterator it = d->mDocumentMap.find(url);
	Document::Ptr ptr;
	if (it != d->mDocumentMap.end()) {
		ptr = it.value();
	} else {
		Document* doc = new Document();
		doc->load(url);
		ptr = Document::Ptr(doc);
		d->mDocumentMap[url] = ptr;
		connect(doc, SIGNAL(saved(const KUrl&)), SIGNAL(saved(const KUrl&)) );
		connect(doc, SIGNAL(modified(const KUrl&)), SIGNAL(modified(const KUrl&)) );
	}
	d->garbageCollect();
	return ptr;
}

QList<KUrl> DocumentFactory::modifiedDocumentList() const {
	DocumentMap::Iterator
		it = d->mDocumentMap.begin(),
		end = d->mDocumentMap.end();

	QList<KUrl> list;
	for (;it!=end; ++it) {
		Document::Ptr doc = it.value();
		if (doc->isModified()) {
			list << doc->url();
		}
	}

	return list;
}


bool DocumentFactory::hasUrl(const KUrl& url) const {
	return d->mDocumentMap.contains(url);
}

} // namespace
