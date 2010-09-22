// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#include "fullscreencontent.moc"

// Qt
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QStyle>
#include <QToolButton>
#include <QWindowsStyle>

// KDE
#include <kactioncollection.h>
#include <klocale.h>

// Local
#include "imagemetainfodialog.h"
#include "ui_fullscreenconfigdialog.h"
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>
#include <lib/fullscreenbar.h>
#include <lib/fullscreentheme.h>
#include <lib/gwenviewconfig.h>
#include <lib/imagemetainfomodel.h>
#include <lib/thumbnailview/thumbnailbarview.h>
#include <lib/slideshow.h>


namespace Gwenview {

// Subclass QToolButton to make initStyleOption public
class ButtonBarButton : public QToolButton {
public:
	void initStyleOption(QStyleOptionToolButton* option) const {
		return QToolButton::initStyleOption(option);
	}
};

static QToolButton* createButtonBarButton() {

	ButtonBarButton* button = new ButtonBarButton;
	QSize iconSize = QSize(32, 32);
	button->setIconSize(iconSize);

	// When the action icon changes, the button gets a few pixels larger. This
	// is probably caused by the css styling.
	// Setting a fixed size prevents this problem.
	QStyleOptionToolButton opt;
	button->initStyleOption(&opt);
	QSize buttonSize = button->style()
		->sizeFromContents(QStyle::CT_ToolButton, &opt, iconSize, button)
		.expandedTo(QApplication::globalStrut());
	button->setFixedSize(buttonSize);
	return button;
}


class FullScreenConfigDialog : public QFrame, public Ui_FullScreenConfigDialog {
public:
	FullScreenConfigDialog(QWidget* parent)
	: QFrame(parent) {
		setWindowFlags(Qt::Popup);
		setupUi(this);
	}
};


struct FullScreenContentPrivate {
	FullScreenContent* that;
	FullScreenBar* mFullScreenBar;
	SlideShow* mSlideShow;
	QWidget* mButtonBar;
	ThumbnailBarView* mThumbnailBar;
	QLabel* mInformationLabel;
	Document::Ptr mCurrentDocument;
	QPointer<ImageMetaInfoDialog> mImageMetaInfoDialog;
	QPointer<FullScreenConfigDialog> mFullScreenConfigDialog;
	QToolButton* mOptionsButton;

	void createOptionsButton() {
		mOptionsButton = createButtonBarButton();
		mOptionsButton->setIcon(KIcon("configure"));
		mOptionsButton->setToolTip(i18nc("@info:tooltip", "Configure Full Screen Mode"));
		QObject::connect(mOptionsButton, SIGNAL(clicked()),
			that, SLOT(showFullScreenConfigDialog()) );
	}

	void setupThemeListWidget(QListWidget* listWidget) {
		QStringList themeList = FullScreenTheme::themeNameList();
		listWidget->addItems(themeList);
		int row = themeList.indexOf(FullScreenTheme::currentThemeName());
		listWidget->setCurrentRow(row);

		QObject::connect(listWidget, SIGNAL(currentTextChanged(const QString&)),
			that, SLOT(setCurrentFullScreenTheme(const QString&)) );
	}

	void applyCurrentFullScreenTheme() {
		FullScreenTheme theme(FullScreenTheme::currentThemeName());
		mFullScreenBar->setStyleSheet(theme.styleSheet());
		const bool fullWidth = GwenviewConfig::showFullScreenThumbnails();
		mFullScreenBar->setProperty("fullWidth", fullWidth);
	}

	void createLayout() {
		mThumbnailBar->setVisible(GwenviewConfig::showFullScreenThumbnails());
		if (GwenviewConfig::showFullScreenThumbnails()) {
			/*
			Layout looks like this:
			mButtonBar        |
			------------------| mThumbnailBar
			mInformationLabel |
			*/
			mInformationLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
			QGridLayout* layout = new QGridLayout(mFullScreenBar);
			layout->setMargin(0);
			layout->setSpacing(0);
			layout->addWidget(mButtonBar, 0, 0, Qt::AlignTop | Qt::AlignLeft);
			layout->addWidget(mInformationLabel, 1, 0);
			layout->addWidget(mThumbnailBar, 0, 1, 2, 1);
			mFullScreenBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
			mFullScreenBar->setFixedHeight(GwenviewConfig::fullScreenBarHeight());
		} else {
			mInformationLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
			QHBoxLayout* layout = new QHBoxLayout(mFullScreenBar);
			layout->setMargin(0);
			layout->setSpacing(2);
			layout->addWidget(mButtonBar);
			layout->addWidget(mInformationLabel);
			mFullScreenBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
			mFullScreenBar->setFixedHeight(layout->minimumSize().height());
		}
		mFullScreenBar->adjustSize();
		applyCurrentFullScreenTheme();
	}


	void adjustBarWidth() {
		if (GwenviewConfig::showFullScreenThumbnails()) {
			return;
		}
		mFullScreenBar->adjustSize();
	}
};


FullScreenContent::FullScreenContent(FullScreenBar* bar, KActionCollection* actionCollection, SlideShow* slideShow)
: QObject(bar)
, d(new FullScreenContentPrivate) {
	d->that = this;
	d->mFullScreenBar = bar;
	d->mSlideShow = slideShow;
	bar->installEventFilter(this);

	// Hardcode base style. This is necessary because some styles like
	// to paint stuff with an event filter rather than overloading QStyle
	// methods, overwriting the stylesheet rendering.
	// Oxygen is one of those styles.
	QStyle* style = new QWindowsStyle;
	d->mFullScreenBar->setStyle(style);
	style->setParent(d->mFullScreenBar);

	// Apply theme
	d->applyCurrentFullScreenTheme();

	// Button bar
	d->mButtonBar = new QWidget;
	d->mButtonBar->setObjectName( QLatin1String("buttonBar" ));
	QHBoxLayout* buttonBarLayout = new QHBoxLayout(d->mButtonBar);
	buttonBarLayout->setMargin(0);
	buttonBarLayout->setSpacing(0);
	QStringList actionNameList;
	actionNameList
		<< "fullscreen"
		<< "toggle_slideshow"
		<< "go_previous"
		<< "go_next"
		<< "rotate_left"
		<< "rotate_right"
		;
	Q_FOREACH(const QString& actionName, actionNameList) {
		QAction* action = actionCollection->action(actionName);
		QToolButton* button = createButtonBarButton();
		button->setDefaultAction(action);
		buttonBarLayout->addWidget(button);
	}

	d->createOptionsButton();
	buttonBarLayout->addWidget(d->mOptionsButton);

	// Thumbnail bar
	d->mThumbnailBar = new ThumbnailBarView(bar);
	ThumbnailBarItemDelegate* delegate = new ThumbnailBarItemDelegate(d->mThumbnailBar);
	d->mThumbnailBar->setItemDelegate(delegate);

	// mInformationLabel
	d->mInformationLabel = new QLabel;
	d->mInformationLabel->setWordWrap(true);
	d->mInformationLabel->setAlignment(Qt::AlignCenter);

	d->createLayout();
}


FullScreenContent::~FullScreenContent() {
	delete d;
}


ThumbnailBarView* FullScreenContent::thumbnailBar() const {
	return d->mThumbnailBar;
}


void FullScreenContent::setCurrentUrl(const KUrl& url) {
	d->mCurrentDocument = DocumentFactory::instance()->load(url);
	connect(d->mCurrentDocument.data(),SIGNAL(metaInfoUpdated()),
		SLOT(updateInformationLabel()) );
	connect(d->mCurrentDocument.data(),SIGNAL(metaInfoUpdated()),
		SLOT(updateMetaInfoDialog()) );
	updateInformationLabel();
	updateMetaInfoDialog();
}


void FullScreenContent::updateInformationLabel() {
	if (!d->mCurrentDocument) {
		kWarning() << "No document";
		return;
	}

	if (!d->mInformationLabel->isVisible()) {
		return;
	}

	ImageMetaInfoModel* model = d->mCurrentDocument->metaInfo();

	QStringList valueList;
	Q_FOREACH(const QString& key, GwenviewConfig::fullScreenPreferredMetaInfoKeyList()) {
		const QString value = model->getValueForKey(key);
		if (value.length() > 0) {
			valueList << value;
		}
	}
	QString text = valueList.join(i18nc("@item:intext fullscreen meta info separator", ", "));

	d->mInformationLabel->setText(text);

	d->adjustBarWidth();
}


void FullScreenContent::setCurrentFullScreenTheme(const QString& themeName) {
	FullScreenTheme::setCurrentThemeName(themeName);
	d->applyCurrentFullScreenTheme();
}


bool FullScreenContent::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::Show) {
		updateInformationLabel();
		updateMetaInfoDialog();
	}
	return false;
}


void FullScreenContent::showImageMetaInfoDialog() {
	if (!d->mImageMetaInfoDialog) {
		d->mImageMetaInfoDialog = new ImageMetaInfoDialog(d->mInformationLabel);
		// Do not let the fullscreen theme propagate to this dialog for now,
		// it's already quite complicated to create a theme
		d->mImageMetaInfoDialog->setStyle(QApplication::style());
		d->mImageMetaInfoDialog->setAttribute(Qt::WA_DeleteOnClose, true);
		connect(d->mImageMetaInfoDialog, SIGNAL(preferredMetaInfoKeyListChanged(const QStringList&)),
			SLOT(slotPreferredMetaInfoKeyListChanged(const QStringList&)) );
		connect(d->mImageMetaInfoDialog, SIGNAL(destroyed()),
			SLOT(enableAutoHiding()) );
	}
	if (d->mCurrentDocument) {
		d->mImageMetaInfoDialog->setMetaInfo(d->mCurrentDocument->metaInfo(), GwenviewConfig::fullScreenPreferredMetaInfoKeyList());
	}
	d->mFullScreenBar->setAutoHidingEnabled(false);
	d->mImageMetaInfoDialog->show();
}


void FullScreenContent::slotPreferredMetaInfoKeyListChanged(const QStringList& list) {
	GwenviewConfig::setFullScreenPreferredMetaInfoKeyList(list);
	GwenviewConfig::self()->writeConfig();
	updateInformationLabel();
}


void FullScreenContent::updateMetaInfoDialog() {
	if (!d->mImageMetaInfoDialog) {
		return;
	}
	ImageMetaInfoModel* model = d->mCurrentDocument ? d->mCurrentDocument->metaInfo() : 0;
	d->mImageMetaInfoDialog->setMetaInfo(model, GwenviewConfig::fullScreenPreferredMetaInfoKeyList());
}


static QString formatSlideShowIntervalText(int value) {
	return i18ncp("Slideshow interval in seconds", "%1 sec", "%1 secs", value);
}


void FullScreenContent::updateSlideShowIntervalLabel() {
	Q_ASSERT(d->mFullScreenConfigDialog);
	int value = d->mFullScreenConfigDialog->mSlideShowIntervalSlider->value();
	QString text = formatSlideShowIntervalText(value);
	d->mFullScreenConfigDialog->mSlideShowIntervalLabel->setText(text);
}


void FullScreenContent::setFullScreenBarHeight(int value) {
	d->mFullScreenBar->setFixedHeight(value);
	GwenviewConfig::setFullScreenBarHeight(value);
}


/**
 * Helper function for showFullScreenConfigDialog
 */
static void setupCheckBox(QCheckBox* checkBox, QAction* action) {
	checkBox->setChecked(action->isChecked());
	QObject::connect(checkBox, SIGNAL(toggled(bool)),
		action, SLOT(trigger()) );
}

void FullScreenContent::showFullScreenConfigDialog() {
	FullScreenConfigDialog* dialog = new FullScreenConfigDialog(d->mOptionsButton);
	d->mFullScreenConfigDialog = dialog;
	dialog->setAttribute(Qt::WA_DeleteOnClose, true);
	dialog->setObjectName( QLatin1String("configDialog" ));

	// Checkboxes
	setupCheckBox(dialog->mSlideShowLoopCheckBox, d->mSlideShow->loopAction());
	setupCheckBox(dialog->mSlideShowRandomCheckBox, d->mSlideShow->randomAction());

	// Interval slider
	dialog->mSlideShowIntervalSlider->setValue(int(GwenviewConfig::interval()));
	connect(dialog->mSlideShowIntervalSlider, SIGNAL(valueChanged(int)),
		d->mSlideShow, SLOT(setInterval(int)) );

	// Interval label
	connect(dialog->mSlideShowIntervalSlider, SIGNAL(valueChanged(int)),
		SLOT(updateSlideShowIntervalLabel()) );
	QString text = formatSlideShowIntervalText(88);
	int width = dialog->mSlideShowIntervalLabel->fontMetrics().width(text);
	dialog->mSlideShowIntervalLabel->setFixedWidth(width);
	updateSlideShowIntervalLabel();

	// Config button
	connect(dialog->mConfigureDisplayedInformationButton, SIGNAL(clicked()),
		SLOT(showImageMetaInfoDialog()) );

	d->setupThemeListWidget(dialog->mThemeListWidget);

	// Thumbnails check box
	dialog->mShowThumbnailsCheckBox->setChecked(GwenviewConfig::showFullScreenThumbnails());
	connect(dialog->mShowThumbnailsCheckBox, SIGNAL(toggled(bool)),
		SLOT(slotShowThumbnailsToggled(bool)));

	// Height slider
	dialog->mHeightSlider->setValue(d->mFullScreenBar->height());
	connect(dialog->mHeightSlider, SIGNAL(valueChanged(int)),
		this, SLOT(setFullScreenBarHeight(int)) );

	// Show dialog below the button
	QRect buttonRect = d->mOptionsButton->rect();
	QPoint pos;
	if (dialog->isRightToLeft()) {
		pos = buttonRect.bottomRight();
		pos.rx() -= dialog->width() - 1;
	} else {
		pos = buttonRect.bottomLeft();
	}
	pos = d->mOptionsButton->mapToGlobal(pos);
	dialog->move(pos);
	dialog->show();
}


void FullScreenContent::enableAutoHiding() {
	d->mFullScreenBar->setAutoHidingEnabled(true);
}


void FullScreenContent::slotShowThumbnailsToggled(bool value) {
	GwenviewConfig::setShowFullScreenThumbnails(value);
	GwenviewConfig::self()->writeConfig();
	delete d->mFullScreenBar->layout();
	d->createLayout();
}


} // namespace
