/* SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef BACKGROUNDCOLORWIDGET_H
#define BACKGROUNDCOLORWIDGET_H

#include <lib/gwenviewlib_export.h>
#include <QWidget>

namespace Gwenview
{

class BackgroundColorWidgetPrivate;

class GWENVIEWLIB_EXPORT BackgroundColorWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(ColorMode colorMode READ colorMode WRITE setColorMode NOTIFY colorModeChanged)

public:
    explicit BackgroundColorWidget(QWidget *parent = nullptr);
    ~BackgroundColorWidget() override;

    enum ColorMode {
        Auto = 0,
        Light = 1,
        Neutral = 2,
        Dark = 3
    };
    Q_ENUM(ColorMode)

    ColorMode colorMode() const;
    void setColorMode(ColorMode colorMode);

    void setActions(QAction* autoMode, QAction* lightMode, QAction* neutralMode, QAction* darkMode);

    static bool usingLightTheme();

Q_SIGNALS:
    void colorModeChanged(ColorMode colorMode);

private:
    const std::unique_ptr<BackgroundColorWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(BackgroundColorWidget)
    Q_DISABLE_COPY(BackgroundColorWidget)
};

}

#endif // BACKGROUNDCOLORWIDGET_H
