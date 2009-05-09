// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef VIDEOVIEWADAPTER_H
#define VIDEOVIEWADAPTER_H

#include <lib/gwenviewlib_export.h>

// Qt

// KDE

// Local
#include <lib/documentview/abstractdocumentviewadapter.h>

namespace Gwenview {


class VideoViewAdapterPrivate;
class GWENVIEWLIB_EXPORT VideoViewAdapter : public AbstractDocumentViewAdapter {
	Q_OBJECT
public:
	VideoViewAdapter(QWidget*);
	~VideoViewAdapter();

	virtual void installEventFilterOnViewWidgets(QObject*);

	virtual MimeTypeUtils::Kind kind() const { return MimeTypeUtils::KIND_VIDEO; }

	virtual Document::Ptr document() const;

	virtual void setDocument(Document::Ptr);

protected:
	bool eventFilter(QObject*, QEvent*);

private Q_SLOTS:
	void slotPlayPauseClicked();
	void updatePlayPauseButton();

private:
	friend class VideoViewAdapterPrivate;
	VideoViewAdapterPrivate* const d;
};


} // namespace

#endif /* VIDEOVIEWADAPTER_H */
