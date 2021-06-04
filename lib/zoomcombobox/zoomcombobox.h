// <one line to give the program's name and a brief idea of what it does.>
// SPDX-FileCopyrightText: 2021 <copyright holder> <email>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#ifndef GWENVIEW_ZOOMCOMBOBOX_H
#define GWENVIEW_ZOOMCOMBOBOX_H

#include <lib/gwenviewlib_export.h>
#include <QComboBox>

namespace Gwenview {

class ZoomComboBoxPrivate;

/**
 * QComboBox subclass designed to be somewhat similar to QSpinBox.
 * Allows the user to use non-integer combobox list items,
 * but only accepts integers as custom input.
 */
class GWENVIEWLIB_EXPORT ZoomComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
    explicit ZoomComboBox(QWidget *parent = nullptr);
    ~ZoomComboBox() override;

    int value() const;
    void setValue(const int value);

    int minimum() const;
    void setMinimum(const int minimum);

    int maximum() const;
    void setMaximum(const int maximum);

//     void setCurrentText(const QString &text);

    /// Gets an integer value from text.
    int valueFromText(const QString &text, bool *ok = nullptr) const;

    /// Gets appropriately decorated text from an integer value.
    QString textFromValue(const int value) const;

    QValidator::State validate(QString &input, int &pos) const;
    void fixup(QString &input) const;

Q_SIGNALS:
    void valueChanged(int value);
    void minimumChanged(int minimum);
    void maximumChanged(int maximum);

protected:
//     void changeEvent(QEvent *event) override;
//     void keyPressEvent(QKeyEvent * event) override;

private:
    const std::unique_ptr<ZoomComboBoxPrivate> d_ptr;
    Q_DECLARE_PRIVATE(ZoomComboBox)
    Q_DISABLE_COPY(ZoomComboBox)
};

}

#endif // GWENVIEW_ZOOMCOMBOBOX_H
