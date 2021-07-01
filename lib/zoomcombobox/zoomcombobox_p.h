// SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#ifndef GWENVIEW_ZOOMCOMBOBOX_P_H
#define GWENVIEW_ZOOMCOMBOBOX_P_H

#include "zoomcombobox.h"

namespace Gwenview {

class ZoomValidator : public QValidator
{
    Q_OBJECT
    Q_PROPERTY(qreal minimum READ minimum WRITE setMinimum NOTIFY changed)
    Q_PROPERTY(qreal maximum READ maximum WRITE setMaximum NOTIFY changed)
public:
    explicit ZoomValidator(qreal minimum, qreal maximum, ZoomComboBox *q, ZoomComboBoxPrivate *d, QWidget* parent = nullptr);
    ~ZoomValidator() override;

    qreal minimum() const;
    void setMinimum(const qreal minimum);

    qreal maximum() const;
    void setMaximum(const qreal maximum);

    QValidator::State validate(QString &input, int &pos) const override;

private:
    qreal m_minimum;
    qreal m_maximum;
    ZoomComboBox *m_zoomComboBox;
    ZoomComboBoxPrivate *m_zoomComboBoxPrivate;
    Q_DISABLE_COPY(ZoomValidator)
};

class ZoomComboBoxPrivate
{
    Q_DECLARE_PUBLIC(ZoomComboBox)

public:
    ZoomComboBoxPrivate(ZoomComboBox *q);

    void setActions(QAction *zoomToFitAction, QAction *zoomToFillAction, QAction *actualSizeAction);

public:
    ZoomComboBox *const q_ptr;

    QAction *mZoomToFitAction = nullptr;
    QAction *mZoomToFillAction = nullptr;
    QAction *mActualSizeAction = nullptr;

    qreal value = 1.0;
    ZoomValidator *validator = nullptr;
    int lastSelectedIndex = 0;
    qreal lastCustomZoomValue = 1.0;
};

}

#endif // GWENVIEW_ZOOMCOMBOBOX_P_H
