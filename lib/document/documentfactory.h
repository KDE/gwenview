/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#ifndef DOCUMENTFACTORY_H
#define DOCUMENTFACTORY_H

// Qt
#include <QObject>

#include <lib/document/document.h>

class QUndoGroup;

class KUrl;

namespace Gwenview {

class DocumentFactoryPrivate;

class GWENVIEWLIB_EXPORT DocumentFactory : public QObject {
	Q_OBJECT
public:
	static DocumentFactory* instance();
	~DocumentFactory();

	Document::Ptr load(const KUrl&, Document::LoadState state = Document::LoadAll);

	QList<KUrl> modifiedDocumentList() const;

	bool hasUrl(const KUrl&, Document::LoadState) const;

	void clearCache();

	QUndoGroup* undoGroup();

Q_SIGNALS:
	void modifiedDocumentListChanged();
	void documentChanged(const KUrl&);

private Q_SLOTS:
	void slotLoaded(const KUrl&);
	void slotSaved(const KUrl&);
	void slotModified(const KUrl&);

private:
	DocumentFactory();

	DocumentFactoryPrivate* const d;
};

} // namespace
#endif /* DOCUMENTFACTORY_H */
