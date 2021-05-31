// <one line to give the program's name and a brief idea of what it does.>
// SPDX-FileCopyrightText: 2021 <copyright holder> <email>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#ifndef GWENVIEW_ZOOMCOMBOBOX_P_H
#define GWENVIEW_ZOOMCOMBOBOX_P_H

#include "zoomcombobox.h"

namespace Gwenview {

class ZoomValidator;

class ZoomComboBoxPrivate
{
    Q_DECLARE_PUBLIC(ZoomComboBox)
public:
    ZoomComboBoxPrivate(ZoomComboBox *q);
    ZoomComboBox *const q_ptr;

    void updateCursorPosition();
    void updateText();
    void updateLineEdit();
    int currentTextIndex() const;

    int value = 0;
    QLineEdit *lineEdit = nullptr;
    ZoomValidator *validator = nullptr;
};

class ZoomValidator : public QValidator
{
    Q_OBJECT
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY(QString percentSymbol READ percentSymbol NOTIFY percentSymbolChanged)
public:
    explicit ZoomValidator(int minimum, int maximum, ZoomComboBox *q, ZoomComboBoxPrivate *d, QWidget* parent = nullptr);
    ~ZoomValidator() override;

    int minimum() const;
    void setMinimum(const int minimum);

    int maximum() const;
    void setMaximum(const int maximum);

    QString percentSymbol() const;

    QValidator::State validate(QString &input, int &pos) const override;
    void fixup(QString &input) const override;

Q_SIGNALS:
    void minimumChanged(int minimum);
    void maximumChanged(int maximum);
    void percentSymbolChanged(const QString& percentSymbol);

protected:
    bool event(QEvent *event) override;

private:
    int m_minimum;
    int m_maximum;
    // Using QString because QLocale::percent() will return a QString in Qt 6.
    QString m_percentSymbol;
    ZoomComboBox *m_zoomComboBox;
    ZoomComboBoxPrivate *m_zoomComboBoxPrivate;
    Q_DISABLE_COPY(ZoomValidator)
};

}

#endif // GWENVIEW_ZOOMCOMBOBOX_P_H
