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
#include "sidebar.moc"

// Qt
#include <QAction>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

// KDE
#include <kicon.h>
#include <kiconloader.h>
#include <kglobalsettings.h>

// Local
#include "lib/expandbutton.h"

namespace Gwenview {


/**
 * A button which always leave room for an icon, even if there is none, so that
 * all button texts are correctly aligned.
 */
class SideBarButton : public QToolButton {
protected:
	virtual void paintEvent(QPaintEvent* event) {
		forceIcon();
		QToolButton::paintEvent(event);
	}

	virtual QSize sizeHint() const {
		const_cast<SideBarButton*>(this)->forceIcon();
		return QToolButton::sizeHint();
	}

private:
	void forceIcon() {
		if (!icon().isNull()) {
			return;
		}

		// Assign an empty icon to the button if there is no icon associated
		// with the action so that all button texts are correctly aligned.
		QSize wantedSize = iconSize();
		if (mEmptyIcon.isNull() || mEmptyIcon.actualSize(wantedSize) != wantedSize) {
			QPixmap pix(wantedSize);
			pix.fill(Qt::transparent);
			mEmptyIcon.addPixmap(pix);
		}
		setIcon(mEmptyIcon);
	}

	QIcon mEmptyIcon;
};


//- SideBarGroup ---------------------------------------------------------------
struct SideBarGroupPrivate {
	QFrame* mContainer;
	ExpandButton* mTitleButton;
};


SideBarGroup::SideBarGroup(const QString& title)
: QFrame()
, d(new SideBarGroupPrivate) {
	d->mContainer = 0;
	d->mTitleButton = new ExpandButton(this);
	d->mTitleButton->setChecked(true);
	d->mTitleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	d->mTitleButton->setFixedHeight(d->mTitleButton->sizeHint().height() * 3 / 2);
	QFont font(d->mTitleButton->font());
	font.setBold(true);
	d->mTitleButton->setFont(font);
	d->mTitleButton->setText(title);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);

	layout->addWidget(d->mTitleButton);

	clear();
}


SideBarGroup::~SideBarGroup() {
	delete d;
}


void SideBarGroup::paintEvent(QPaintEvent* event) {
	QFrame::paintEvent(event);
	if (y() != 0) {
		// Draw a separator, but only if we are not the first group
		QPainter painter(this);
		QPen pen(palette().mid().color());
		painter.setPen(pen);
		painter.drawLine(rect().topLeft(), rect().topRight());
	}
}


void SideBarGroup::addWidget(QWidget* widget) {
	widget->setParent(d->mContainer);
	d->mContainer->layout()->addWidget(widget);
}


void SideBarGroup::clear() {
	if (d->mContainer) {
		d->mContainer->deleteLater();
	}

	d->mContainer = new QFrame(this);
	QVBoxLayout* containerLayout = new QVBoxLayout(d->mContainer);
	containerLayout->setMargin(0);
	containerLayout->setSpacing(0);

	layout()->addWidget(d->mContainer);

	connect(d->mTitleButton, SIGNAL(toggled(bool)),
		d->mContainer, SLOT(setVisible(bool)) );
}


void SideBarGroup::addAction(QAction* action) {
	int size = KIconLoader::global()->currentSize(KIconLoader::Small);
	QToolButton* button = new SideBarButton();
	button->setFocusPolicy(Qt::NoFocus);
	button->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	button->setAutoRaise(true);
	button->setDefaultAction(action);
	button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	button->setIconSize(QSize(size, size));
	if (action->menu()) {
		button->setPopupMode(QToolButton::InstantPopup);
	}
	addWidget(button);
}


//- SideBarPage ----------------------------------------------------------------
struct SideBarPagePrivate {
	QString mTitle;
	KIcon mIcon;
	QVBoxLayout* mLayout;
};


SideBarPage::SideBarPage(const QString& title, const QString& iconName)
: QWidget()
, d(new SideBarPagePrivate)
{
	d->mTitle = title;
	d->mIcon = KIcon(iconName);

	d->mLayout = new QVBoxLayout(this);
	d->mLayout->setMargin(0);
}


const QString& SideBarPage::title() const {
	return d->mTitle;
}


const KIcon& SideBarPage::icon() const {
	return d->mIcon;
}


void SideBarPage::addWidget(QWidget* widget) {
	d->mLayout->addWidget(widget);
}


void SideBarPage::addStretch() {
	d->mLayout->addStretch();
}


//- SideBar --------------------------------------------------------------------
struct SideBarPrivate {
};


SideBar::SideBar(QWidget* parent)
: QTabWidget(parent)
, d(new SideBarPrivate) {
	setFont(KGlobalSettings::toolBarFont());
}


SideBar::~SideBar() {
	delete d;
}


QSize SideBar::sizeHint() const {
	return QSize(200, 200);
}


void SideBar::addPage(SideBarPage* page) {
	addTab(page, page->icon(), page->title());
}


} // namespace
