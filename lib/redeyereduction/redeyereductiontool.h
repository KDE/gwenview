// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#ifndef REDEYEREDUCTIONTOOL_H
#define REDEYEREDUCTIONTOOL_H

#include <lib/gwenviewlib_export.h>

// Qt

// KDE

// Local
#include <lib/abstractimageviewtool.h>

namespace Gwenview {

class AbstractImageOperation;
class ImageView;

struct RedEyeReductionToolPrivate;
class GWENVIEWLIB_EXPORT RedEyeReductionTool : public AbstractImageViewTool {
	Q_OBJECT
public:
	enum Status {
		NotSet,
		Adjusting
	};

	RedEyeReductionTool(ImageView* parent);
	~RedEyeReductionTool();

	virtual void paint(QPainter*);

	virtual void mousePressEvent(QMouseEvent*);
	virtual void mouseMoveEvent(QMouseEvent*);

	virtual void toolActivated();
	virtual void toolDeactivated();

Q_SIGNALS:
	void done();
	void imageOperationRequested(AbstractImageOperation*);

private Q_SLOTS:
	void setDiameter(int);
	void slotApplyClicked();

private:
	RedEyeReductionToolPrivate* const d;
};


} // namespace

#endif /* REDEYEREDUCTIONTOOL_H */
