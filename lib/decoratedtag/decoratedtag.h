// SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef GWENVIEW_DECORATEDTAG_H
#define GWENVIEW_DECORATEDTAG_H

#include <QLabel>
#include <lib/gwenviewlib_export.h>

namespace Gwenview
{
class DecoratedTagPrivate;

/**
 * A label with a custom background under it.
 *
 * TODO: Turn this into a more interactive control and make it look like Manuel's mockup.
 * Should probably be turned into a QAbstractButton subclass or something.
 */
class GWENVIEWLIB_EXPORT DecoratedTag : public QLabel
{
    Q_OBJECT
public:
    explicit DecoratedTag(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    explicit DecoratedTag(const QString &text, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~DecoratedTag() override;

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    const std::unique_ptr<DecoratedTagPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DecoratedTag)
};

}

#endif // GWENVIEW_DECORATEDTAG_H
