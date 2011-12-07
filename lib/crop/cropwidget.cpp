// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
#include "cropwidget.moc"

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QPushButton>

// KDE
#include <KDebug>
#include <KDialog>
#include <KGlobalSettings>
#include <KLineEdit>
#include <KLocale>

// Local
#include <lib/documentview/rasterimageview.h>
#include "croptool.h"
#include "signalblocker.h"
#include "ui_cropwidget.h"

namespace Gwenview
{

// Euclidean algorithm to compute the greatest common divisor of two integers.
// Found at:
// http://en.wikipedia.org/wiki/Euclidean_algorithm
static int gcd(int a, int b)
{
    return b == 0 ? a : gcd(b, a % b);
}

static QSize screenRatio()
{
    const QRect rect = QApplication::desktop()->screenGeometry();
    const int width = rect.width();
    const int height = rect.height();
    const int divisor = gcd(width, height);
    return QSize(width / divisor, height / divisor);
}

struct CropWidgetPrivate : public Ui_CropWidget {
    CropWidget* q;

    Document::Ptr mDocument;
    CropTool* mCropTool;
    bool mUpdatingFromCropTool;

    bool ratioIsConstrained() const {
        return cropRatio() > 0;
    }

    double cropRatio() const {
        const QStringList lst = ratioComboBox->currentText().split(':');
        if (lst.size() != 2) {
            return 0;
        }

        bool ok;
        const double width = lst[0].toDouble(&ok);
        if (!ok) {
            return 0;
        }
        const double height = lst[1].toDouble(&ok);
        if (!ok) {
            return 0;
        }

        return height / width;
    }

    void addRatioToComboBox(const QSize& size, const QString& label = QString())
    {
        QString text = QString("%1:%2").arg(size.width()).arg(size.height());
        if (!label.isEmpty()) {
            text += QString(" (%1)").arg(label);
        }
        ratioComboBox->addItem(text, QVariant(size));
    }

    void addSeparatorToComboBox()
    {
        ratioComboBox->insertSeparator(ratioComboBox->count());
    }

    void initRatioComboBox()
    {
        QList<QSize> ratioList;
        ratioList
                << QSize(3, 2)
                << QSize(4, 3)
                << QSize(5, 4)
                << QSize(6, 4)
                << QSize(7, 5);

        addRatioToComboBox(QSize(1, 1), i18n("Square"));
        addRatioToComboBox(screenRatio(), i18n("This Screen"));
        addSeparatorToComboBox();

        Q_FOREACH(const QSize & size, ratioList) {
            addRatioToComboBox(size);
        }
        addSeparatorToComboBox();
        Q_FOREACH(QSize size, ratioList) {
            size.transpose();
            addRatioToComboBox(size);
        }

        ratioComboBox->setMaxVisibleItems(ratioComboBox->count());
        ratioComboBox->setEditText(QString());

        KLineEdit* edit = qobject_cast<KLineEdit*>(ratioComboBox->lineEdit());
        Q_ASSERT(edit);
        // Do not use i18n("%1:%2") because ':' should not be translated, it is
        // used to parse the ratio string.
        edit->setClickMessage(QString("%1:%2").arg(i18n("Width")).arg(i18n("Height")));
    }

    QRect cropRect() const {
        QRect rect(
            leftSpinBox->value(),
            topSpinBox->value(),
            widthSpinBox->value(),
            heightSpinBox->value()
        );
        return rect;
    }

    void initSpinBoxes()
    {
        QSize size = mDocument->size();
        leftSpinBox->setMaximum(size.width());
        widthSpinBox->setMaximum(size.width());
        topSpinBox->setMaximum(size.height());
        heightSpinBox->setMaximum(size.height());
    }

    void initDialogButtonBox()
    {
        QPushButton* cropButton = dialogButtonBox->button(QDialogButtonBox::Ok);
        cropButton->setIcon(KIcon("transform-crop-and-resize"));
        cropButton->setText(i18n("Crop"));

        QObject::connect(dialogButtonBox, SIGNAL(accepted()),
                         q, SIGNAL(cropRequested()));
        QObject::connect(dialogButtonBox, SIGNAL(rejected()),
                         q, SIGNAL(done()));
    }
};

CropWidget::CropWidget(QWidget* parent, RasterImageView* imageView, CropTool* cropTool)
: QWidget(parent)
, d(new CropWidgetPrivate)
{
    setWindowFlags(Qt::Tool);
    d->q = this;
    d->mDocument = imageView->document();
    d->mUpdatingFromCropTool = false;
    d->mCropTool = cropTool;
    d->setupUi(this);
    setFont(KGlobalSettings::smallestReadableFont());
    layout()->setMargin(KDialog::marginHint());
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(d->advancedCheckBox, SIGNAL(toggled(bool)),
            d->advancedWidget, SLOT(setVisible(bool)));
    d->advancedWidget->setVisible(false);
    d->advancedWidget->layout()->setMargin(0);

    d->initRatioComboBox();

    connect(d->mCropTool, SIGNAL(rectUpdated(QRect)),
            SLOT(setCropRect(QRect)));

    connect(d->leftSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotPositionChanged()));
    connect(d->topSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotPositionChanged()));
    connect(d->widthSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotWidthChanged()));
    connect(d->heightSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotHeightChanged()));

    d->initDialogButtonBox();

    connect(d->ratioComboBox, SIGNAL(editTextChanged(QString)),
            SLOT(slotRatioComboBoxEditTextChanged()));
    connect(d->ratioComboBox, SIGNAL(activated(int)),
            SLOT(slotRatioComboBoxActivated()));

    // Don't do this before signals are connected, otherwise the tool won't get
    // initialized
    d->initSpinBoxes();

    setCropRect(d->mCropTool->rect());
}

CropWidget::~CropWidget()
{
    delete d;
}

void CropWidget::setCropRect(const QRect& rect)
{
    d->mUpdatingFromCropTool = true;
    d->leftSpinBox->setValue(rect.left());
    d->topSpinBox->setValue(rect.top());
    d->widthSpinBox->setValue(rect.width());
    d->heightSpinBox->setValue(rect.height());
    d->mUpdatingFromCropTool = false;
}

void CropWidget::slotPositionChanged()
{
    const QSize size = d->mDocument->size();
    d->widthSpinBox->setMaximum(size.width() - d->leftSpinBox->value());
    d->heightSpinBox->setMaximum(size.height() - d->topSpinBox->value());

    if (d->mUpdatingFromCropTool) {
        return;
    }
    d->mCropTool->setRect(d->cropRect());
}

void CropWidget::slotWidthChanged()
{
    d->leftSpinBox->setMaximum(d->mDocument->width() - d->widthSpinBox->value());

    if (d->mUpdatingFromCropTool) {
        return;
    }
    if (d->ratioIsConstrained()) {
        int height = int(d->widthSpinBox->value() * d->cropRatio());
        d->heightSpinBox->setValue(height);
    }
    d->mCropTool->setRect(d->cropRect());
}

void CropWidget::slotHeightChanged()
{
    d->topSpinBox->setMaximum(d->mDocument->height() - d->heightSpinBox->value());

    if (d->mUpdatingFromCropTool) {
        return;
    }
    if (d->ratioIsConstrained()) {
        int width = int(d->heightSpinBox->value() / d->cropRatio());
        d->widthSpinBox->setValue(width);
    }
    d->mCropTool->setRect(d->cropRect());
}

void CropWidget::applyRatioConstraint()
{
    double ratio = d->cropRatio();
    d->mCropTool->setCropRatio(ratio);

    if (!d->ratioIsConstrained()) {
        return;
    }
    QRect rect = d->cropRect();
    rect.setHeight(int(rect.width() * ratio));
    d->mCropTool->setRect(rect);
}

void CropWidget::slotRatioComboBoxEditTextChanged()
{
    applyRatioConstraint();
}

void CropWidget::slotRatioComboBoxActivated()
{
    // If the ratioComboBox contains text like this: "w:h (foo bar)", change it
    // to "w:h" only, so that it's easier to edit for the user.
    QStringList lst = d->ratioComboBox->currentText().split(' ');
    if (lst.size() > 1) {
        SignalBlocker blocker(d->ratioComboBox);
        d->ratioComboBox->setEditText(lst[0]);
        applyRatioConstraint();
    }
}

} // namespace
