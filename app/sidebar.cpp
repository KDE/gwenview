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
#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionTabV3>
#include <QStylePainter>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>

// KDE
#include <kdebug.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kglobalsettings.h>
#include <kwordwrap.h>

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


//- ThinTabBar -----------------------------------------------------------------
class ThinTabBar : public QTabBar {
public:
	ThinTabBar(QWidget* parent=0)
	: QTabBar(parent) {
	}

	virtual QSize sizeHint() const {
		QSize sh = QTabBar::sizeHint();
		sh.setWidth(sh.height());
		return sh;
	}

	virtual QSize minimumSizeHint() const {
		return sizeHint();
	}

protected:
	enum {
		MARGIN = 4
	};

	virtual QSize tabSizeHint(int index) const {
		QSize sh = QTabBar::tabSizeHint(index);
		sh.setWidth(mTabWidths.value(index, 2 * iconSize().width()));
		return sh;
	}

	virtual void resizeEvent(QResizeEvent* event) {
		computeTabWidths();
		QTabBar::resizeEvent(event);
	}

	virtual void paintEvent(QPaintEvent*) {
		QStylePainter painter(this);
		QStyleOptionTabBarBaseV2 styleOption;
		initStyleBaseOption(&styleOption, this, size());
		for (int index = 0; index < count(); ++index) {
			styleOption.tabBarRect |= tabRect(index);
		}
		painter.drawPrimitive(QStyle::PE_FrameTabBarBase, styleOption);

		for (int index=0; index < count(); ++index) {
			if (index != currentIndex()) {
				paintTab(&painter, index, QSize(0, 0));
			}
		}
		QSize offset(
			style()->pixelMetric(QStyle::PM_TabBarTabShiftHorizontal),
			style()->pixelMetric(QStyle::PM_TabBarTabShiftVertical));
		paintTab(&painter, currentIndex(), offset);
	}

private:
	void paintTab(QStylePainter* painter, int index, const QSize& offset) {
		QRect rect = tabRect(index);
		QStyleOptionTabV3 styleOption;
		initStyleOption(&styleOption, index);
		painter->drawControl(QStyle::CE_TabBarTabShape, styleOption);

		QRect textRect, iconRect;
		textRect = rect.adjusted(MARGIN - offset.width(), MARGIN - offset.height(), -MARGIN, -MARGIN);
		iconRect = textRect;

		if (QApplication::isRightToLeft()) {
			iconRect.setLeft(iconRect.right() - iconSize().width());
			textRect.setRight(iconRect.left() - MARGIN);
		} else {
			iconRect.setWidth(iconSize().width());
			textRect.setLeft(iconRect.right() + MARGIN);
		}

		tabIcon(index).paint(painter, iconRect);
		painter->setPen(palette().windowText().color());

		int posY = textRect.top() + fontMetrics().height();
		KWordWrap::drawFadeoutText(painter, textRect.x(), posY, textRect.width(), tabText(index));
	}


	void computeTabWidths() {
		if (!parentWidget() || count() == 0) {
			return;
		}
		if (count() != mTabWidths.count()) {
			mTabWidths.resize(count());
		}

		int parentWidth = parentWidget()->width();

		// Compute totalWidth
		int totalWidth = 0;
		int margin = 3 * MARGIN + iconSize().width();
		for (int index = 0; index < count(); ++index) {
			mTabWidths[index] = fontMetrics().width(tabText(index)) + margin;
			totalWidth += mTabWidths[index];
			setTabToolTip(index, QString()); // reset tooltip while looping
		}

		if (totalWidth <= parentWidth) {
			// It fits, get out
			return;
		}

		int averageTabWidth = parentWidth / count();
		// Make a list of all tabs which are wider than averageTabWidth
		QList<int> tooLargeIndexes;
		int stickyWidth = 0;
		for (int index = 0; index < count(); ++index) {
			if (mTabWidths[index] > averageTabWidth) {
				tooLargeIndexes << index;
			} else {
				stickyWidth += mTabWidths[index];
			}
		}

		Q_ASSERT(tooLargeIndexes.count() > 0);
		const int shrunkTabWidth = (parentWidth - stickyWidth) / tooLargeIndexes.count();
		// Adjust big tabs
		Q_FOREACH(int index, tooLargeIndexes) {
			mTabWidths[index] = shrunkTabWidth;
			setTabToolTip(index, tabText(index));
		}
	}

	QVector<int> mTabWidths;

	// Copied from qtabbar_p.h
	static void initStyleBaseOption(QStyleOptionTabBarBaseV2 *optTabBase, QTabBar *tabbar, QSize size) {
		QStyleOptionTab tabOverlap;
		tabOverlap.shape = tabbar->shape();
		int overlap = tabbar->style()->pixelMetric(QStyle::PM_TabBarBaseOverlap, &tabOverlap, tabbar);
		QWidget *theParent = tabbar->parentWidget();
		optTabBase->init(tabbar);
		optTabBase->shape = tabbar->shape();
		optTabBase->documentMode = tabbar->documentMode();
		if (theParent && overlap > 0) {
			QRect rect;
			switch (tabOverlap.shape) {
				case QTabBar::RoundedNorth:
				case QTabBar::TriangularNorth:
					rect.setRect(0, size.height()-overlap, size.width(), overlap);
					break;
				case QTabBar::RoundedSouth:
				case QTabBar::TriangularSouth:
					rect.setRect(0, 0, size.width(), overlap);
					break;
				case QTabBar::RoundedEast:
				case QTabBar::TriangularEast:
					rect.setRect(0, 0, overlap, size.height());
					break;
				case QTabBar::RoundedWest:
				case QTabBar::TriangularWest:
					rect.setRect(size.width() - overlap, 0, overlap, size.height());
					break;
			}
			optTabBase->rect = rect;
		}
	}
};


//- SideBar --------------------------------------------------------------------
struct SideBarPrivate {
};


SideBar::SideBar(QWidget* parent)
: QTabWidget(parent)
, d(new SideBarPrivate) {
	setFont(KGlobalSettings::smallestReadableFont());
	setTabBar(new ThinTabBar);
	tabBar()->setDocumentMode(true);
	tabBar()->setUsesScrollButtons(false);
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


QString SideBar::currentPage() const {
	return currentWidget()->objectName();
}


void SideBar::setCurrentPage(const QString& name) {
	for (int index = 0; index < count(); ++index) {
		if (widget(index)->objectName() == name) {
			setCurrentIndex(index);
		}
	}
}


} // namespace
