// SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

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
 *
 * This class is structured in a way so that changes to the zoom done through
 * user interaction are signalled to the outside. On the other hand changes
 * done to the visual state of this class without user interaction will not
 * lead to emitted zoom changes/signals from this class.
 */
class GWENVIEWLIB_EXPORT ZoomComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(qreal value READ value WRITE setValue)
    Q_PROPERTY(qreal minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(qreal maximum READ maximum WRITE setMaximum)

public:
    explicit ZoomComboBox(QWidget *parent = nullptr);
    ~ZoomComboBox() override;

    void setActions(QAction *zoomToFitAction, QAction *zoomToFillAction, QAction *actualSizeAction);

    qreal value() const;
    /**
     * Called when the value that is being displayed should change.
     * Calling this method doesn't affect the zoom of the currently viewed image.
     */
    void setValue(const qreal value);

    qreal minimum() const;
    void setMinimum(const qreal minimum);

    qreal maximum() const;
    void setMaximum(const qreal maximum);

    /// Gets an integer value from text.
    qreal valueFromText(const QString &text, bool *ok = nullptr) const;

    /// Gets appropriately decorated text from an integer value.
    QString textFromValue(const qreal value) const;

    void updateDisplayedText();

Q_SIGNALS:
    void zoomChanged(qreal zoom);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

private Q_SLOTS:
    /**
     * This method changes the zoom mode or value of the currently displayed image in response to
     * user interaction with the Combobox' dropdown menu.
     * @param index of the zoom mode or value that the user interacted with.
     */
    void changeZoomTo(int index);

    /**
     * Sets the index as the fallback when the popup menu is closed without selection in the
     * future and then calls changeZoomTo().
     * @see changeZoomTo()
     */
    void activateAndChangeZoomTo(int index);

private:
    const std::unique_ptr<ZoomComboBoxPrivate> d_ptr;
    Q_DECLARE_PRIVATE(ZoomComboBox)
    Q_DISABLE_COPY(ZoomComboBox)
};

}

#endif // GWENVIEW_ZOOMCOMBOBOX_H
