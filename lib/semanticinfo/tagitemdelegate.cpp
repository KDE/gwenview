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
#include <tagitemdelegate.moc>

// Qt
#include <QAbstractItemView>
#include <QPainter>
#include <QToolButton>

// KDE
#include <kdebug.h>
#include <kdialog.h>
#include <kiconloader.h>

// Local
#include <lib/semanticinfo/tagmodel.h>

namespace Gwenview {

TagItemDelegate::TagItemDelegate(QAbstractItemView* view)
: KWidgetItemDelegate(view, view)
{
	#define pm(x) view->style()->pixelMetric(QStyle::x)
	mMargin     = pm(PM_ToolBarItemMargin);
	mSpacing    = pm(PM_ToolBarItemSpacing);
	#undef pm
	const int iconSize = KIconLoader::global()->currentSize(KIconLoader::Toolbar);
	const QSize sz = view->style()->sizeFromContents(QStyle::CT_ToolButton, 0, QSize(iconSize, iconSize));
	mButtonSize = qMax(sz.width(), sz.height());
}


QList<QWidget*> TagItemDelegate::createItemWidgets() const {
	QToolButton* removeButton = new QToolButton;
	removeButton->setIcon(KIcon("list-remove"));
	removeButton->setAutoRaise(true);
	connect(removeButton, SIGNAL(clicked()), SLOT(slotRemoveButtonClicked()));
	setBlockedEventTypes(removeButton, QList<QEvent::Type>()
		<< QEvent::MouseButtonPress
		<< QEvent::MouseButtonRelease
		<< QEvent::MouseButtonDblClick);
	return QList<QWidget*>() << removeButton;
}


void TagItemDelegate::updateItemWidgets(const QList<QWidget*> widgets, const QStyleOptionViewItem& option, const QPersistentModelIndex& /*index*/) const {
	QToolButton* removeButton = static_cast<QToolButton*>(widgets[0]);
	removeButton->resize(mButtonSize, option.rect.height() - 2 * mMargin);
	removeButton->move(option.rect.width() - mButtonSize - mMargin, mMargin);
}


void TagItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	if (!index.isValid()) {
		return;
	}

	itemView()->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, 0);

	QRect textRect = option.rect;
	textRect.setLeft(textRect.left() + mMargin);
	textRect.setWidth(textRect.width() - mButtonSize - mMargin - mSpacing);
	painter->setPen(option.palette.color(QPalette::Normal,
		option.state & QStyle::State_Selected
		? QPalette::HighlightedText
		: QPalette::Text));
	painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, index.data().toString());
}


QSize TagItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
	const int width  = option.fontMetrics.width(index.data().toString());
	const int height = qMax(mButtonSize, option.fontMetrics.height());
	return QSize(width + 2 * mMargin, height + 2 * mMargin);
}


void TagItemDelegate::slotRemoveButtonClicked() {
	const QModelIndex index = focusedIndex();
	if (!index.isValid()) {
		kWarning() << "!index.isValid()";
		return;
	}
	emit removeTagRequested(index.data(TagModel::TagRole).toString());
}


} // namespace
