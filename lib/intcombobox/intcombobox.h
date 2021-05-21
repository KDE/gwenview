// <one line to give the program's name and a brief idea of what it does.>
// SPDX-FileCopyrightText: 2021 <copyright holder> <email>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#ifndef GWENVIEW_INTCOMBOBOX_H
#define GWENVIEW_INTCOMBOBOX_H

#include <lib/gwenviewlib_export.h>
#include <QComboBox>

namespace Gwenview {

class IntComboBoxPrivate;

/**
 * QComboBox subclass designed to handle integers in a manner similar to QSpinBox
 */
class GWENVIEWLIB_EXPORT IntComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(QString suffix READ suffix WRITE setSuffix NOTIFY suffixChanged)

public:
    explicit IntComboBox(QWidget *parent = nullptr);
    ~IntComboBox() override;

    QString cleanText() const;

    int value() const;
    Q_SLOT void setValue(int value);
    Q_SLOT void setValue(const QString &text);

    int minimum() const;
    void setMinimum(int minimum);

    int maximum() const;
    void setMaximum(int maximum);
    void setRange(int minimum, int maximum);

    QString suffix() const;
    void setSuffix(const QString& suffix);

    int valueFromText(const QString &text) const;
    QString textFromValue(const int value) const;

    QValidator::State validate(QString &input, int &pos) const;
    void fixup(QString &input) const;

Q_SIGNALS:
    void valueChanged(int value);
    void maximumChanged(int maximum);
    void minimumChanged(int minimum);
    void suffixChanged(const QString& suffix);

protected:
    void setValidator(QIntValidator *validator);

private:
    const std::unique_ptr<IntComboBoxPrivate> d_ptr;
    Q_DECLARE_PRIVATE(IntComboBox)
};

}

#endif // GWENVIEW_INTCOMBOBOX_H
