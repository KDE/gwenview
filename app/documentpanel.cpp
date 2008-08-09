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
#include "documentpanel.moc"

// Qt
#include <QShortcut>
#include <QSplitter>
#include <QStylePainter>
#include <QToolButton>
#include <QVBoxLayout>

// KDE
#include <kactioncollection.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmenu.h>
#include <kmimetype.h>
#include <kparts/componentfactory.h>
#include <kparts/statusbarextension.h>
#include <kstatusbar.h>
#include <ktoggleaction.h>

// Local
#include "thumbnailbarview.h"
#include <lib/abstractdocumentviewadapter.h>
#include <lib/documentview.h>
#include <lib/imageview.h>
#include <lib/paintutils.h>
#include <lib/gwenviewconfig.h>
#include <lib/signalblocker.h>
#include <lib/statusbartoolbutton.h>
#include <lib/zoomwidget.h>


namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif


static QString rgba(const QColor &color) {
	return QString::fromAscii("rgba(%1, %2, %3, %4)")
		.arg(color.red())
		.arg(color.green())
		.arg(color.blue())
		.arg(color.alpha());
}


static QString gradient(const QColor &color, int value) {
	QString grad =
		"qlineargradient(x1:0, y1:0, x2:0, y2:1,"
		"stop:0 %1, stop: 1 %2)";
	return grad.arg(
		rgba(PaintUtils::adjustedHsv(color, 0, 0, qMin(255 - color.value(), value/2))),
		rgba(PaintUtils::adjustedHsv(color, 0, 0, -qMin(color.value(), value/2)))
		);
}


class SplitterHandle : public QSplitterHandle {
public:
	SplitterHandle(Qt::Orientation orientation, QSplitter* parent)
	: QSplitterHandle(orientation, parent) {}

protected:
	virtual void paintEvent(QPaintEvent*) {
		QStylePainter painter(this);

		QStyleOption opt;
		opt.initFrom(this);

		// Draw a thin styled line below splitter handle
		QStyleOption lineOpt = opt;
		const int lineSize = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this);
		const int margin = 4 * lineSize;
		lineOpt.rect = QRect(-margin, height() - lineSize, width() + 2*margin, height());
		lineOpt.state |= QStyle::State_Sunken;
		painter.drawPrimitive(QStyle::PE_Frame, lineOpt);

		// Draw the normal splitter handle
		opt.rect.adjust(0, 0, 0, -lineSize);
		painter.drawControl(QStyle::CE_Splitter, opt);
	}
};


/**
 * Home made splitter to be able to define a custom handle:
 * We want to show a thin line between the splitter and the thumbnail bar but
 * we don't do it with css because "border-top:" forces a border around the
 * whole widget (Qt 4.4.0)
 */
class Splitter : public QSplitter {
public:
	Splitter(Qt::Orientation orientation, QWidget* parent)
	: QSplitter(orientation, parent) {
		const int lineSize = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this);
		setHandleWidth(handleWidth() + lineSize);
	}

protected:
	virtual QSplitterHandle* createHandle() {
		return new SplitterHandle(orientation(), this);
	}
};


/*
 * Layout of mThumbnailSplitter is:
 *
 * +-mThumbnailSplitter--------------------------------+
 * |+-mAdapterContainer-------------------------------+|
 * ||+-mDocumentView---------------------------------+||
 * |||                                               |||
 * |||                                               |||
 * |||                                               |||
 * |||                                               |||
 * |||                                               |||
 * ||+-----------------------------------------------+||
 * ||+-mStatusBarContainer---------------------------+||
 * |||[mToggleThumbnailBarButton]       [mZoomWidget]|||
 * ||+-----------------------------------------------+||
 * |+-------------------------------------------------+|
 * |===================================================|
 * |+-mThumbnailBar-----------------------------------+|
 * ||                                                 ||
 * ||                                                 ||
 * |+-------------------------------------------------+|
 * +---------------------------------------------------+
 */
struct DocumentPanelPrivate {
	DocumentPanel* that;
	KActionCollection* mActionCollection;
	QSplitter *mThumbnailSplitter;
	QWidget* mAdapterContainer;
	DocumentView* mDocumentView;
	QToolButton* mToggleThumbnailBarButton;
	QWidget* mStatusBarContainer;
	ThumbnailBarView* mThumbnailBar;
	KToggleAction* mToggleThumbnailBarAction;

	bool mFullScreenMode;
	QPalette mNormalPalette;
	QPalette mFullScreenPalette;
	bool mThumbnailBarVisibleBeforeFullScreen;

	void setupThumbnailBar() {
		mThumbnailBar = new ThumbnailBarView;
		ThumbnailBarItemDelegate* delegate = new ThumbnailBarItemDelegate(mThumbnailBar);
		mThumbnailBar->setItemDelegate(delegate);
		mThumbnailBar->setVisible(GwenviewConfig::thumbnailBarIsVisible());

		QColor bgColor = mThumbnailBar->palette().color(QPalette::Normal, QPalette::Window);
		QColor bgSelColor = mThumbnailBar->palette().color(QPalette::Normal, QPalette::Highlight);

		// Avoid dark and bright colors
		bgColor.setHsv(bgColor.hue(), bgColor.saturation(), (127 + 3 * bgColor.value()) / 4);

		QColor leftBorderColor = PaintUtils::adjustedHsv(bgColor, 0, 0, qMin(20, 255 - bgColor.value()));
		QColor rightBorderColor = PaintUtils::adjustedHsv(bgColor, 0, 0, -qMin(40, bgColor.value()));
		QColor borderSelColor = PaintUtils::adjustedHsv(bgSelColor, 0, 0, -qMin(60, bgSelColor.value()));

		QString viewCss =
			"#thumbnailBarView {"
			"	background-color: rgba(0, 0, 0, 10%);"
			"}";

		QString itemCss =
			"QListView::item {"
			"	background-color: %1;"
			"	border-left: 1px solid %2;"
			"	border-right: 1px solid %3;"
			"}";
		itemCss = itemCss.arg(
			gradient(bgColor, 46),
			gradient(leftBorderColor, 36),
			gradient(rightBorderColor, 26));

		QString itemSelCss =
			"QListView::item:selected {"
			"	background-color: %1;"
			"	border-left: 1px solid %2;"
			"	border-right: 1px solid %2;"
			"}";
		itemSelCss = itemSelCss.arg(
			gradient(bgSelColor, 56),
			rgba(borderSelColor));

		mThumbnailBar->setStyleSheet(viewCss + itemCss + itemSelCss);
	}

	void setupAdapterContainer() {
		mAdapterContainer = new QWidget;

		QVBoxLayout* layout = new QVBoxLayout(mAdapterContainer);
		layout->setMargin(0);
		layout->setSpacing(0);
		layout->addWidget(mDocumentView);
		layout->addWidget(mStatusBarContainer);
	}

	void setupDocumentView() {
		mDocumentView = new DocumentView(0, mActionCollection);

		// Connect context menu
		mDocumentView->setContextMenuPolicy(Qt::CustomContextMenu);
		QObject::connect(mDocumentView, SIGNAL(customContextMenuRequested(const QPoint&)),
			that, SLOT(showContextMenu()) );

		QObject::connect(mDocumentView, SIGNAL(completed()),
			that, SIGNAL(completed()) );
		QObject::connect(mDocumentView, SIGNAL(resizeRequested(const QSize&)),
			that, SIGNAL(resizeRequested(const QSize&)) );
		QObject::connect(mDocumentView, SIGNAL(previousImageRequested()),
			that, SIGNAL(previousImageRequested()) );
		QObject::connect(mDocumentView, SIGNAL(nextImageRequested()),
			that, SIGNAL(nextImageRequested()) );
		QObject::connect(mDocumentView, SIGNAL(captionUpdateRequested(const QString&)),
			that, SIGNAL(captionUpdateRequested(const QString&)) );
		QObject::connect(mDocumentView, SIGNAL(completed()),
			that, SIGNAL(completed()) );
	}

	void setupStatusBar() {
		mStatusBarContainer = new QWidget;
		mToggleThumbnailBarButton = new StatusBarToolButton;

		QHBoxLayout* layout = new QHBoxLayout(mStatusBarContainer);
		layout->setMargin(0);
		layout->setSpacing(0);
		layout->addWidget(mToggleThumbnailBarButton);
		layout->addStretch();
		layout->addWidget(mDocumentView->zoomWidget());
		mDocumentView->zoomWidget()->hide();
	}

	void setupSplitter() {
		mThumbnailSplitter = new Splitter(Qt::Vertical, that);
		mThumbnailSplitter->addWidget(mAdapterContainer);
		mThumbnailSplitter->addWidget(mThumbnailBar);
		mThumbnailSplitter->setSizes(GwenviewConfig::thumbnailSplitterSizes());

		QVBoxLayout* layout = new QVBoxLayout(that);
		layout->setMargin(0);
		layout->addWidget(mThumbnailSplitter);
	}

	void applyPalette() {
		QPalette palette = mFullScreenMode ? mFullScreenPalette : mNormalPalette;
		that->setPalette(palette);

		if (!mDocumentView->adapter()) {
			return;
		}
		QWidget* widget = mDocumentView->adapter()->widget();
		if (!widget) {
			return;
		}

		QPalette partPalette = widget->palette();
		partPalette.setBrush(widget->backgroundRole(), palette.base());
		partPalette.setBrush(widget->foregroundRole(), palette.text());
		widget->setPalette(partPalette);
	}

	void saveSplitterConfig() {
		if (mThumbnailBar->isVisible()) {
			GwenviewConfig::setThumbnailSplitterSizes(mThumbnailSplitter->sizes());
		}
	}

};


DocumentPanel::DocumentPanel(QWidget* parent, KActionCollection* actionCollection)
: QWidget(parent)
, d(new DocumentPanelPrivate)
{
	d->that = this;
	d->mActionCollection = actionCollection;
	d->mFullScreenMode = false;
	d->mThumbnailBarVisibleBeforeFullScreen = false;
	d->mFullScreenPalette = QPalette(palette());
	d->mFullScreenPalette.setColor(QPalette::Base, Qt::black);
	d->mFullScreenPalette.setColor(QPalette::Text, Qt::white);

	QShortcut* enterFullScreenShortcut = new QShortcut(this);
	enterFullScreenShortcut->setKey(Qt::Key_Return);
	connect(enterFullScreenShortcut, SIGNAL(activated()), SIGNAL(enterFullScreenRequested()) );

	d->setupDocumentView();

	d->setupStatusBar();

	d->setupAdapterContainer();

	d->setupThumbnailBar();

	d->setupSplitter();

	d->mToggleThumbnailBarAction = actionCollection->add<KToggleAction>("toggle_thumbnailbar");
	d->mToggleThumbnailBarAction->setText(i18n("Thumbnail Bar"));
	d->mToggleThumbnailBarAction->setIcon(KIcon("folder-image"));
	d->mToggleThumbnailBarAction->setShortcut(Qt::CTRL | Qt::Key_B);
	d->mToggleThumbnailBarAction->setChecked(GwenviewConfig::thumbnailBarIsVisible());
	connect(d->mToggleThumbnailBarAction, SIGNAL(triggered(bool)),
		this, SLOT(setThumbnailBarVisibility(bool)) );
	d->mToggleThumbnailBarButton->setDefaultAction(d->mToggleThumbnailBarAction);
}


DocumentPanel::~DocumentPanel() {
	delete d;
}


void DocumentPanel::loadConfig() {
	// FIXME: Not symetric with saveConfig(). Check if it matters.
	if (d->mDocumentView->adapter()) {
		d->mDocumentView->adapter()->loadConfig();
	}
}


void DocumentPanel::saveConfig() {
	d->saveSplitterConfig();
	GwenviewConfig::setThumbnailBarIsVisible(d->mToggleThumbnailBarAction->isChecked());
}


void DocumentPanel::setThumbnailBarVisibility(bool visible) {
	d->saveSplitterConfig();
	d->mThumbnailBar->setVisible(visible);
}


int DocumentPanel::statusBarHeight() const {
	return d->mStatusBarContainer->height();
}


void DocumentPanel::setStatusBarHeight(int height) {
	d->mStatusBarContainer->setFixedHeight(height);
}


void DocumentPanel::setFullScreenMode(bool fullScreenMode) {
	d->mFullScreenMode = fullScreenMode;
	d->mStatusBarContainer->setVisible(!fullScreenMode);
	d->applyPalette();
	if (fullScreenMode) {
		d->mThumbnailBarVisibleBeforeFullScreen = d->mToggleThumbnailBarAction->isChecked();
		if (d->mThumbnailBarVisibleBeforeFullScreen) {
			d->mToggleThumbnailBarAction->trigger();
		}
	} else if (d->mThumbnailBarVisibleBeforeFullScreen) {
		d->mToggleThumbnailBarAction->trigger();
	}
	d->mToggleThumbnailBarAction->setEnabled(!fullScreenMode);
}


ThumbnailBarView* DocumentPanel::thumbnailBar() const {
	return d->mThumbnailBar;
}


inline void addActionToMenu(KMenu* menu, KActionCollection* actionCollection, const char* name) {
	QAction* action = actionCollection->action(name);
	if (action) {
		menu->addAction(action);
	}
}


void DocumentPanel::showContextMenu() {
	KMenu menu(this);
	addActionToMenu(&menu, d->mActionCollection, "fullscreen");
	menu.addSeparator();
	addActionToMenu(&menu, d->mActionCollection, "go_previous");
	addActionToMenu(&menu, d->mActionCollection, "go_next");
	if (d->mDocumentView->adapter()->canZoom()) {
		menu.addSeparator();
		addActionToMenu(&menu, d->mActionCollection, "view_actual_size");
		addActionToMenu(&menu, d->mActionCollection, "view_zoom_to_fit");
		addActionToMenu(&menu, d->mActionCollection, "view_zoom_in");
		addActionToMenu(&menu, d->mActionCollection, "view_zoom_out");
	}
	menu.exec(QCursor::pos());
}


QSize DocumentPanel::sizeHint() const {
	return QSize(400, 300);
}


KUrl DocumentPanel::url() const {
	if (!d->mDocumentView->adapter()) {
		return KUrl();
	}

	if (!d->mDocumentView->adapter()->document()) {
		kWarning() << "!d->mDocumentView->adapter()->document()";
		return KUrl();
	}

	return d->mDocumentView->adapter()->document()->url();
}


bool DocumentPanel::currentDocumentIsRasterImage() const {
	// If the document view is visible, we assume we have a raster image if and
	// only if we have an ImageView. This avoids having to determine the
	// mimetype a second time.
	return imageView() != 0;
}


bool DocumentPanel::isEmpty() const {
	return d->mDocumentView->isEmpty();
}


ImageView* DocumentPanel::imageView() const {
	return d->mDocumentView->adapter()->imageView();
}


void DocumentPanel::setNormalPalette(const QPalette& palette) {
	d->mNormalPalette = palette;
	d->applyPalette();
}


bool DocumentPanel::openUrl(const KUrl& url) {
	return d->mDocumentView->openUrl(url);
}


void DocumentPanel::reset() {
	d->mDocumentView->reset();
}


} // namespace
