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
// Self
#include "thumbnailpage.moc"

// Qt
#include <QDate>
#include <QDir>
#include <QPushButton>

// KDE
#include <kdebug.h>
#include <kdirlister.h>
#include <kurlnavigator.h>

// Local
#include <lib/archiveutils.h>
#include <lib/gwenviewconfig.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/thumbnailview/abstractthumbnailviewhelper.h>
#include <lib/thumbnailview/previewitemdelegate.h>
#include <lib/thumbnailview/thumbnailslidercontroller.h>
#include <ui_thumbnailpage.h>

namespace Gwenview {

static const int DEFAULT_THUMBNAIL_SIZE = 128;


class ImporterThumbnailViewHelper : public AbstractThumbnailViewHelper {
public:
	ImporterThumbnailViewHelper(QObject* parent)
	: AbstractThumbnailViewHelper(parent)
	{}

	void showContextMenu(QWidget*)
	{}

	void showMenuForUrlDroppedOnViewport(QWidget*, const KUrl::List&)
	{}

	void showMenuForUrlDroppedOnDir(QWidget*, const KUrl::List&, const KUrl&)
	{}

	bool isDocumentModified(const KUrl&) {
		return false;
	}

	void thumbnailForDocument(const KUrl&, ThumbnailGroup::Enum, QPixmap* outPix, QSize* outFullSize) const {
		*outPix = QPixmap();
		*outFullSize = QSize();
	}
};


struct ThumbnailPagePrivate : public Ui_ThumbnailPage {
	ThumbnailPage* q;
	KUrlNavigator* mUrlNavigator;
	SortedDirModel* mDirModel;
	QPushButton* mImportSelectedButton;
	QPushButton* mImportAllButton;
	KUrl::List mUrlList;
	KUrl mDstBaseUrl;

	void setupDirModel() {
		mDirModel = new SortedDirModel(q);
		mDirModel->setKindFilter(
			MimeTypeUtils::KIND_DIR
			| MimeTypeUtils::KIND_ARCHIVE
			| MimeTypeUtils::KIND_RASTER_IMAGE
			| MimeTypeUtils::KIND_SVG_IMAGE
			| MimeTypeUtils::KIND_VIDEO
			);
	}

	void setupDstBaseUrl() {
		// FIXME: Use xdg, and make this configurable
		mDstBaseUrl = KUrl::fromPath(QDir::home().absoluteFilePath("photos"));
		int year = QDate::currentDate().year();
		mDstBaseUrl.addPath(QString::number(year));
	}

	void setupUrlNavigator() {
		mUrlNavigator = new KUrlNavigator(0 /* KFilePlacesModel */, KUrl(), mUrlNavigatorContainer);
		mUrlNavigator->setShowFullPath(true);
		QVBoxLayout* layout = new QVBoxLayout(mUrlNavigatorContainer);
		layout->setMargin(0);
		layout->addWidget(mUrlNavigator);

		QObject::connect(mUrlNavigator, SIGNAL(urlChanged(const KUrl&)),
			q, SLOT(openUrl(const KUrl&)) );
	}

	void setupThumbnailView() {
		mThumbnailView->setModel(mDirModel);
		mThumbnailView->setSelectionMode(QAbstractItemView::ExtendedSelection);
		mThumbnailView->setThumbnailViewHelper(new ImporterThumbnailViewHelper(q));

		PreviewItemDelegate* delegate = new PreviewItemDelegate(mThumbnailView);
		delegate->setThumbnailDetails(PreviewItemDelegate::FileNameDetail);
		mThumbnailView->setItemDelegate(delegate);

		// Colors
		QColor bgColor = GwenviewConfig::viewBackgroundColor();
		QColor fgColor = bgColor.value() > 128 ? Qt::black : Qt::white;

		QPalette pal = mThumbnailView->palette();
		pal.setColor(QPalette::Base, bgColor);
		pal.setColor(QPalette::Text, fgColor);

		mThumbnailView->setPalette(pal);

		ThumbnailSliderController* sliderController = new ThumbnailSliderController(mThumbnailView, mSlider);
		int thumbnailSize = DEFAULT_THUMBNAIL_SIZE;
		mSlider->setValue(thumbnailSize);
		sliderController->updateToolTip();
		mThumbnailView->setThumbnailSize(thumbnailSize);

		QObject::connect(mThumbnailView, SIGNAL(indexActivated(const QModelIndex&)),
			q, SLOT(slotThumbnailViewIndexActivated(const QModelIndex&)));
	}

	void setupButtonBox() {
		mImportSelectedButton = mButtonBox->addButton(
			i18n("Import Selected"), QDialogButtonBox::AcceptRole,
			q, SLOT(slotImportSelected()));

		mImportAllButton = mButtonBox->addButton(
			i18n("Import All"), QDialogButtonBox::AcceptRole,
			q, SLOT(slotImportAll()));

		QObject::connect(
			mThumbnailView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
			q, SLOT(updateImportButtons()));
		QObject::connect(
			mDirModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
			q, SLOT(updateImportButtons()));
		QObject::connect(
			mDirModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)),
			q, SLOT(updateImportButtons()));
		QObject::connect(
			mDirModel, SIGNAL(modelReset()),
			q, SLOT(updateImportButtons()));

		QObject::connect(
			mButtonBox, SIGNAL(rejected()),
			q, SIGNAL(rejected()));

		q->updateImportButtons();
	}

	void setupEventComboBox() {
		QObject::connect(
			mEventComboBox, SIGNAL(editTextChanged(const QString&)),
			q, SLOT(updateDstLabel()));
		QObject::connect(
			mEventComboBox, SIGNAL(editTextChanged(const QString&)),
			q, SLOT(updateImportButtons()));
		q->updateDstLabel();
	}
};


ThumbnailPage::ThumbnailPage()
: d(new ThumbnailPagePrivate) {
	d->q = this;
	d->setupUi(this);
	d->setupDirModel();
	d->setupDstBaseUrl();
	d->setupUrlNavigator();
	d->setupThumbnailView();
	d->setupButtonBox();
	d->setupEventComboBox();
}


ThumbnailPage::~ThumbnailPage() {
	delete d;
}


void ThumbnailPage::setSourceUrl(const KUrl& url) {
	openUrl(url);
}


void ThumbnailPage::openUrl(const KUrl& url) {
	d->mUrlNavigator->setUrl(url);
	d->mDirModel->dirLister()->openUrl(url);
}


KUrl::List ThumbnailPage::urlList() const {
	return d->mUrlList;
}


KUrl ThumbnailPage::destinationUrl() const {
	KUrl url = d->mDstBaseUrl;
	url.addPath(d->mEventComboBox->currentText());
	return url;
}


void ThumbnailPage::slotThumbnailViewIndexActivated(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	KFileItem item = d->mDirModel->itemForIndex(index);
	if (item.isDir()) {
		// Item is a dir, open it
		openUrl(item.url());
	} else {
		QString protocol = ArchiveUtils::protocolForMimeType(item.mimetype());
		if (!protocol.isEmpty()) {
			// Item is an archive, tweak url then open it
			KUrl url = item.url();
			url.setProtocol(protocol);
			openUrl(url);
		}
	}
}


void ThumbnailPage::slotImportSelected() {
	d->mUrlList.clear();
	QModelIndexList list = d->mThumbnailView->selectionModel()->selectedIndexes();
	Q_FOREACH(const QModelIndex& index, list) {
		d->mUrlList << d->mDirModel->urlForIndex(index);
	}
	emit importRequested();
}


void ThumbnailPage::slotImportAll() {
	d->mUrlList.clear();
	for (int row = d->mDirModel->rowCount() - 1; row >= 0; --row) {
		QModelIndex index = d->mDirModel->index(row, 0);
		d->mUrlList << d->mDirModel->urlForIndex(index);
	}
	emit importRequested();
}


void ThumbnailPage::updateImportButtons() {
	bool hasEvent = !d->mEventComboBox->currentText().isEmpty();
	d->mImportSelectedButton->setEnabled(hasEvent && d->mThumbnailView->selectionModel()->hasSelection());
	d->mImportAllButton->setEnabled(hasEvent && d->mDirModel->hasChildren());
}


void ThumbnailPage::updateDstLabel() {
	if (d->mEventComboBox->currentText().isEmpty()) {
		// FIXME: Use KDE error color instead of 'red'
		d->mDstLabel->setText("<font color='red'>Enter an event name</font>");
	} else {
		QString text = i18n(
			"Pictures will be imported in: <filename>%1</filename>",
			destinationUrl().pathOrUrl()
			);
		d->mDstLabel->setText(text);
	}
}


} // namespace
