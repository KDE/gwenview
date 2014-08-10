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

// KDE
#include <KLocalizedString>

// Local
#include <ui_resizeimagewidget.h>

namespace Gwenview
{

struct ResizeImageDialogPrivate : public Ui_ResizeImageWidget
{
    bool mUpdateFromRatio;
    QSize mOriginalSize;
};

ResizeImageDialog::ResizeImageDialog(QWidget* parent)
: KDialog(parent)
, d(new ResizeImageDialogPrivate)
{
    d->mUpdateFromRatio = false;

    QWidget* content = new QWidget(this);
    d->setupUi(content);
    content->layout()->setMargin(0);
    setMainWidget(content);
    showButtonSeparator(true);
    setButtonGuiItem(Ok, KGuiItem(i18n("Resize"), "transform-scale"));
    setWindowTitle(content->windowTitle());
    d->mWidthSpinBox->setFocus();

    connect(d->mWidthSpinBox, SIGNAL(valueChanged(int)), SLOT(slotWidthChanged(int)));
    connect(d->mHeightSpinBox, SIGNAL(valueChanged(int)), SLOT(slotHeightChanged(int)));
    connect(d->mKeepAspectCheckBox, SIGNAL(toggled(bool)), SLOT(slotKeepAspectChanged(bool)));
}

ResizeImageDialog::~ResizeImageDialog()
{
    delete d;
}

void ResizeImageDialog::setOriginalSize(const QSize& size)
{
    d->mOriginalSize = size;
    d->mOriginalWidthLabel->setText(QString::number(size.width()));
    d->mOriginalHeightLabel->setText(QString::number(size.height()));
    d->mWidthSpinBox->setValue(size.width());
    d->mHeightSpinBox->setValue(size.height());
}

QSize ResizeImageDialog::size() const
{
    return QSize(
               d->mWidthSpinBox->value(),
               d->mHeightSpinBox->value()
           );
}

void ResizeImageDialog::slotWidthChanged(int width)
{
    if (!d->mKeepAspectCheckBox->isChecked()) {
        return;
    }
    if (d->mUpdateFromRatio) {
        return;
    }
    d->mUpdateFromRatio = true;
    d->mHeightSpinBox->setValue(d->mOriginalSize.height() * width / d->mOriginalSize.width());
    d->mUpdateFromRatio = false;
}

void ResizeImageDialog::slotHeightChanged(int height)
{
    if (!d->mKeepAspectCheckBox->isChecked()) {
        return;
    }
    if (d->mUpdateFromRatio) {
        return;
    }
    d->mUpdateFromRatio = true;
    d->mWidthSpinBox->setValue(d->mOriginalSize.width() * height / d->mOriginalSize.height());
    d->mUpdateFromRatio = false;
}

void ResizeImageDialog::slotKeepAspectChanged(bool value)
{
    if (value) {
        slotWidthChanged(d->mWidthSpinBox->value());
    }
}

} // namespace
