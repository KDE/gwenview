// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2010 Aurélien Gâteau <agateau@kde.org>

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
#include "resizeimagedialog.h"

// Qt
#include <QBuffer>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QPushButton>
#include <QTimer>

// KF
#include <KFormat>
#include <KGuiItem>
#include <KLocalizedString>

// Local
#include <lib/gwenviewconfig.h>
#include <ui_resizeimagewidget.h>

namespace Gwenview
{
struct ResizeImageDialogPrivate : public Ui_ResizeImageWidget {
    bool mUpdateFromRatio;
    bool mUpdateFromSizeOrPercentage;
    QSize mOriginalSize;
};

ResizeImageDialog::ResizeImageDialog(QWidget *parent)
    : QDialog(parent)
    , d(new ResizeImageDialogPrivate)
{
    d->mUpdateFromRatio = false;
    d->mUpdateFromSizeOrPercentage = false;

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    auto *content = new QWidget(this);
    d->setupUi(content);
    mainLayout->addWidget(content);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ResizeImageDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ResizeImageDialog::reject);
    mainLayout->addWidget(buttonBox);

    content->layout()->setContentsMargins(0, 0, 0, 0);
    KGuiItem::assign(okButton, KGuiItem(i18n("Resize"), QStringLiteral("transform-scale")));
    setWindowTitle(content->windowTitle());
    d->mWidthSpinBox->setFocus();

    d->mEstimatedSizeLabel->setToolTip(i18nc("@action",
                                             "Assuming that the image format won't be changed and\nfor lossy images using the "
                                             "value set in 'Lossy image save quality'."));

    connect(d->mWidthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ResizeImageDialog::slotWidthChanged);
    connect(d->mHeightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ResizeImageDialog::slotHeightChanged);
    connect(d->mWidthPercentSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ResizeImageDialog::slotWidthPercentChanged);
    connect(d->mHeightPercentSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ResizeImageDialog::slotHeightPercentChanged);
    connect(d->mWidthSpinBox, &QSpinBox::editingFinished, this, &ResizeImageDialog::slotCalculateImageSize);
    connect(d->mHeightSpinBox, &QSpinBox::editingFinished, this, &ResizeImageDialog::slotCalculateImageSize);
    connect(d->mWidthPercentSpinBox, &QSpinBox::editingFinished, this, &ResizeImageDialog::slotCalculateImageSize);
    connect(d->mHeightPercentSpinBox, &QSpinBox::editingFinished, this, &ResizeImageDialog::slotCalculateImageSize);
    connect(d->mKeepAspectCheckBox, &QCheckBox::toggled, this, &ResizeImageDialog::slotKeepAspectChanged);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&ResizeImageDialog::slotCalculateImageSize));
    timer->start(2000);
}

ResizeImageDialog::~ResizeImageDialog()
{
    delete d;
}

void ResizeImageDialog::setOriginalSize(const QSize &size)
{
    d->mOriginalSize = size;
    d->mOriginalWidthLabel->setText(QString::number(size.width()) + QStringLiteral(" px"));
    d->mOriginalHeightLabel->setText(QString::number(size.height()) + QStringLiteral(" px"));
    d->mWidthSpinBox->setValue(size.width());
    d->mHeightSpinBox->setValue(size.height());
}

void ResizeImageDialog::setCurrentImageUrl(QUrl imageUrl)
{
    mCurrentImageUrl = imageUrl;

    // mCurrentSize->setText has to be set after the mCurrentImageUrl has been set, otherwise it's -1
    QFileInfo fileInfo(mCurrentImageUrl.toLocalFile());
    d->mCurrentSize->setText(KFormat().formatByteSize(fileInfo.size()));
    mValueChanged = false;
}

QSize ResizeImageDialog::size() const
{
    return QSize(d->mWidthSpinBox->value(), d->mHeightSpinBox->value());
}

void ResizeImageDialog::slotWidthChanged(int width)
{
    mValueChanged = true;
    // Update width percentage to match width, only if this was a manual adjustment
    if (!d->mUpdateFromSizeOrPercentage && !d->mUpdateFromRatio) {
        d->mUpdateFromSizeOrPercentage = true;
        d->mWidthPercentSpinBox->setValue((double(width) / d->mOriginalSize.width()) * 100);
        d->mUpdateFromSizeOrPercentage = false;
    }

    if (!d->mKeepAspectCheckBox->isChecked() || d->mUpdateFromRatio) {
        return;
    }

    // Update height to keep ratio, only if ratio locked and this was a manual adjustment
    d->mUpdateFromRatio = true;
    d->mHeightSpinBox->setValue(d->mOriginalSize.height() * width / d->mOriginalSize.width());
    d->mUpdateFromRatio = false;
}

void ResizeImageDialog::slotHeightChanged(int height)
{
    mValueChanged = true;
    // Update height percentage to match height, only if this was a manual adjustment
    if (!d->mUpdateFromSizeOrPercentage && !d->mUpdateFromRatio) {
        d->mUpdateFromSizeOrPercentage = true;
        d->mHeightPercentSpinBox->setValue((double(height) / d->mOriginalSize.height()) * 100);
        d->mUpdateFromSizeOrPercentage = false;
    }

    if (!d->mKeepAspectCheckBox->isChecked() || d->mUpdateFromRatio) {
        return;
    }

    // Update width to keep ratio, only if ratio locked and this was a manual adjustment
    d->mUpdateFromRatio = true;
    d->mWidthSpinBox->setValue(d->mOriginalSize.width() * height / d->mOriginalSize.height());
    d->mUpdateFromRatio = false;
}

void ResizeImageDialog::slotWidthPercentChanged(double widthPercent)
{
    mValueChanged = true;
    // Update width to match width percentage, only if this was a manual adjustment
    if (!d->mUpdateFromSizeOrPercentage && !d->mUpdateFromRatio) {
        d->mUpdateFromSizeOrPercentage = true;
        d->mWidthSpinBox->setValue((widthPercent / 100) * d->mOriginalSize.width());
        d->mUpdateFromSizeOrPercentage = false;
    }

    if (!d->mKeepAspectCheckBox->isChecked() || d->mUpdateFromRatio) {
        return;
    }

    // Keep height percentage in sync with width percentage, only if ratio locked and this was a manual adjustment
    d->mUpdateFromRatio = true;
    d->mHeightPercentSpinBox->setValue(d->mWidthPercentSpinBox->value());
    d->mUpdateFromRatio = false;
}

void ResizeImageDialog::slotHeightPercentChanged(double heightPercent)
{
    mValueChanged = true;
    // Update height to match height percentage, only if this was a manual adjustment
    if (!d->mUpdateFromSizeOrPercentage && !d->mUpdateFromRatio) {
        d->mUpdateFromSizeOrPercentage = true;
        d->mHeightSpinBox->setValue((heightPercent / 100) * d->mOriginalSize.height());
        d->mUpdateFromSizeOrPercentage = false;
    }

    if (!d->mKeepAspectCheckBox->isChecked() || d->mUpdateFromRatio) {
        return;
    }

    // Keep height percentage in sync with width percentage, only if ratio locked and this was a manual adjustment
    d->mUpdateFromRatio = true;
    d->mWidthPercentSpinBox->setValue(d->mHeightPercentSpinBox->value());
    d->mUpdateFromRatio = false;
}

void ResizeImageDialog::slotKeepAspectChanged(bool value)
{
    if (value) {
        d->mUpdateFromSizeOrPercentage = true;
        slotWidthChanged(d->mWidthSpinBox->value());
        slotWidthPercentChanged(d->mWidthPercentSpinBox->value());
        d->mUpdateFromSizeOrPercentage = false;
    }
}

void ResizeImageDialog::slotCalculateImageSize()
{
    qint64 size = calculateEstimatedImageSize();
    if (size == -1) {
        return;
    } else if (size == -2) {
        d->mEstimatedSize->setText(i18n("error"));
    } else {
        d->mEstimatedSize->setText(KFormat().formatByteSize(size));
    }
}

qint64 ResizeImageDialog::calculateEstimatedImageSize()
{
    if (mValueChanged) {
        QImage image(mCurrentImageUrl.toLocalFile());
        if (image.isNull()) {
            return -2;
        }

        QByteArray ba;
        QBuffer buffer(&ba);
        QFileInfo fileInfo(mCurrentImageUrl.toLocalFile());
        QString suffix = fileInfo.suffix();

        buffer.open(QIODevice::ReadWrite);
        image = image.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        if (QString::compare(suffix, "jpg", Qt::CaseInsensitive) == 0 || QString::compare(suffix, "jpeg", Qt::CaseInsensitive) == 0
            || QString::compare(suffix, "avif", Qt::CaseInsensitive) == 0 || QString::compare(suffix, "heic", Qt::CaseInsensitive) == 0
            || QString::compare(suffix, "webp", Qt::CaseInsensitive) == 0) {
            image.save(&buffer, suffix.toStdString().c_str(), GwenviewConfig::jPEGQuality());
        } else {
            image.save(&buffer, suffix.toStdString().c_str());
        }

        qint64 size = buffer.size();
        buffer.close();

        mValueChanged = false;
        return size;
    }
    return -1;
}

} // namespace
