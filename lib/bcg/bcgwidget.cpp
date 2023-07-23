/*
Gwenview: an image viewer
Copyright 2022 Ilya Pominov <ipominov@astralinux.ru>

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
// Self
#include "bcgwidget.h"

// Qt
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

// KF
#include <KLocalizedString>

// Local
#include "gwenview_lib_debug.h"

namespace Gwenview
{
struct BCGWidgetPrivate {
    void setupWidgets()
    {
        mCompressor = new QTimer(q);
        mCompressor->setInterval(10);
        mCompressor->setSingleShot(true);
        QObject::connect(mCompressor, &QTimer::timeout, q, [this]() {
            Q_EMIT q->bcgChanged();
        });

        auto sliderLayout = new QFormLayout;
        sliderLayout->addRow(i18n("Brightness:"), createSlider(&mBrightness));
        sliderLayout->addRow(i18n("Contrast:"), createSlider(&mContrast));
        sliderLayout->addRow(i18n("Gamma:"), createSlider(&mGamma));

        auto layout = new QVBoxLayout;
        layout->addLayout(sliderLayout);

        auto btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset);
        mOkBtn = btnBox->button(QDialogButtonBox::Ok);
        mOkBtn->setEnabled(false);
        mResetBtn = btnBox->button(QDialogButtonBox::Reset);
        mResetBtn->setEnabled(false);
        QObject::connect(btnBox, &QDialogButtonBox::rejected, q, [this]() {
            Q_EMIT q->done(false);
        });
        QObject::connect(btnBox, &QDialogButtonBox::accepted, q, [this]() {
            Q_EMIT q->done(true);
        });
        QObject::connect(btnBox->button(QDialogButtonBox::Reset), &QAbstractButton::clicked, q, &BCGWidget::reset);
        layout->addWidget(btnBox);

        q->setLayout(layout);
    }

    QLayout *createSlider(int *field)
    {
        auto slider = new QSlider(Qt::Horizontal);
        slider->setMinimum(-100);
        slider->setMaximum(100);
        auto spinBox = new QSpinBox;
        spinBox->setMinimum(-100);
        spinBox->setMaximum(100);

        QObject::connect(slider, &QSlider::valueChanged, spinBox, &QSpinBox::setValue);
        QObject::connect(spinBox, qOverload<int>(&QSpinBox::valueChanged), q, [this, slider, field](int value) {
            slider->blockSignals(true);
            slider->setValue(value);
            slider->blockSignals(false);
            *field = value;
            bool bcgChanged = mBrightness != 0 || mContrast != 0 || mGamma != 0;
            mOkBtn->setEnabled(bcgChanged);
            mResetBtn->setEnabled(bcgChanged);
            mCompressor->start();
        });
        QObject::connect(q, &BCGWidget::reset, q, [slider]() {
            slider->setValue(0);
        });

        auto layout = new QHBoxLayout;
        layout->addWidget(slider);
        layout->addWidget(spinBox);
        return layout;
    }

    BCGWidget *q = nullptr;
    QTimer *mCompressor = nullptr;
    QAbstractButton *mOkBtn = nullptr;
    QAbstractButton *mResetBtn = nullptr;
    int mBrightness = 0;
    int mContrast = 0;
    int mGamma = 0;
};

BCGWidget::BCGWidget()
    : QWidget(nullptr)
    , d(new BCGWidgetPrivate)
{
    setWindowFlags(Qt::Tool);

    d->q = this;
    d->setupWidgets();
}

BCGWidget::~BCGWidget()
{
    delete d;
}

int BCGWidget::brightness() const
{
    return d->mBrightness;
}

int BCGWidget::contrast() const
{
    return d->mContrast;
}

int BCGWidget::gamma() const
{
    return d->mGamma;
}

} // namespace

#include "moc_bcgwidget.cpp"
