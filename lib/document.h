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
#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <memory>

// Qt
#include <QObject>

class QImage;

namespace Gwenview {

class Document : public QObject {
	Q_OBJECT
public:
	Document();
	void load(const QString&);

	QImage& image();

Q_SIGNALS:
	void loaded();

private:
	struct Private;
	std::auto_ptr<Private> d;
};

} // namespace

#endif /* DOCUMENT_H */
