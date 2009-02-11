// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
// Self
#include "hudwidget.moc"

// Qt
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QStyleOption>
#include <QStylePainter>
#include <QToolButton>

// KDE
#include <kdebug.h>
#include <kiconloader.h>

// Local
#include "fullscreentheme.h"

namespace Gwenview {


class DragHandle : public QWidget {
public:
	DragHandle(QWidget* parent)
	: QWidget(parent) {
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
		setCursor(Qt::SizeAllCursor);
	}

	QSize sizeHint() const {
		const int width = style()->pixelMetric(QStyle::PM_ToolBarHandleExtent);
		return QSize(width, 30);
	}

protected:
	virtual void paintEvent(QPaintEvent*) {
		QStylePainter painter(this);
		QStyleOption option;
		option.palette = parentWidget()->palette();
		option.rect = rect();
		option.state = QStyle::State_Active | QStyle::State_Horizontal;
		painter.drawPrimitive(QStyle::PE_IndicatorToolBarHandle, option);
	}

	virtual void mousePressEvent(QMouseEvent* event) {
		mDragStartPos = event->pos();
	}

	virtual void mouseMoveEvent(QMouseEvent* event) {
		const QPoint delta = event->pos() - mDragStartPos;
		parentWidget()->move(parentWidget()->pos() + delta);
	}

private:
	QPoint mDragStartPos;
};


struct HudWidgetPrivate {
	HudWidget* that;
	DragHandle* mDragHandle;
	QWidget* mMainWidget;
	QToolButton* mCloseButton;

	void keepFullyVisible() {
		const QRect outerRect = that->parentWidget()->rect();

		if (outerRect.contains(that->geometry())) {
			return;
		}

		QPoint pos = that->pos();
		if (pos.x() < 0) {
			pos.setX(0);
		} else if (pos.x() + that->width() > outerRect.width()) {
			pos.setX(outerRect.width() - that->width());
		}
		if (pos.y() < 0) {
			pos.setY(0);
		} else if (pos.y() + that->height() > outerRect.height()) {
			pos.setY(outerRect.height() - that->height());
		}
		that->move(pos);
	}
};


HudWidget::HudWidget(QWidget* parent)
: QFrame(parent)
, d(new HudWidgetPrivate) {
	if (parent) {
		parent->installEventFilter(this);
	}
	d->that = this;
	d->mDragHandle = 0;
	d->mMainWidget = 0;
	d->mCloseButton = 0;
}


HudWidget::~HudWidget() {
	delete d;
}


void HudWidget::init(QWidget* mainWidget, Options options) {
	d->mMainWidget = mainWidget;
	d->mMainWidget->setParent(this);
	if (d->mMainWidget->layout()) {
		d->mMainWidget->layout()->setMargin(0);
	}

	FullScreenTheme theme(FullScreenTheme::currentThemeName());
	setStyleSheet(theme.styleSheet());

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setMargin(4);

	if (options & OptionDragHandle) {
		d->mDragHandle = new DragHandle(this);
		layout->addWidget(d->mDragHandle);
	}
	layout->addWidget(d->mMainWidget);

	if (options & OptionCloseButton) {
		d->mCloseButton = new QToolButton(this);
		d->mCloseButton->setAutoRaise(true);
		d->mCloseButton->setIcon(SmallIcon("window-close"));
		layout->addWidget(d->mCloseButton);
	}
}


QWidget* HudWidget::mainWidget() const {
	return d->mMainWidget;
}


QToolButton* HudWidget::closeButton() const {
	return d->mCloseButton;
}


void HudWidget::moveEvent(QMoveEvent*) {
	if (!parentWidget()) {
		return;
	}
	d->keepFullyVisible();
}


bool HudWidget::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::Resize) {
		d->keepFullyVisible();
	}
	return false;
}


} // namespace
