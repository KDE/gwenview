// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#ifndef INVISIBLEBUTTONGROUP_H
#define INVISIBLEBUTTONGROUP_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QWidget>

// KDE

// Local

class QAbstractButton;

namespace Gwenview
{

struct InvisibleButtonGroupPrivate;
/**
 * This class makes it possible to create radio buttons without having to put
 * them in a dedicated QGroupBox or KButtonGroup. This is useful when you do not
 * want to add visual frames to create a set of radio buttons.
 *
 * It is a QWidget so that it can support KConfigXT: it is completely
 * invisible and does not require the radio buttons to be its children.
 * The most common way to use it is to create your dialog with Designer,
 * including the radio buttons, and to instantiate an instance of
 * InvisibleButtonGroup from your code.
 *
 * Example:
 *
 * We assume "config" is a KConfigSkeleton object which contains a
 * "ViewMode" key. This key is an int where 1 means "list" and 2 means
 * "detail".
 * We also assume "ui" has been created with Designer and contains two
 * QRadioButton named "listRadioButton" and "detailRadioButton".
 *
 * @code
 * // Prepare the config dialog
 * KConfigDialog* dialog(parent, "Settings", config);
 *
 * // Create a widget in the dialog
 * QWidget* pageWidget = new QWidget;
 * ui->setupUi(pageWidget);
 * dialog->addPage(pageWidget, i18n("Page Title"));
 * @endcode
 *
 * Now we can setup an InvisibleButtonGroup to handle both radio
 * buttons and ensure they follow the "ViewMode" config key.
 *
 * @code
 * InvisibleButtonGroup* group = new InvisibleButtonGroup(pageWidget);
 * group->setObjectName( QLatin1String("kcfg_ViewMode" ));
 * group->addButton(ui->listRadioButton, 1);
 * group->addButton(ui->detailRadioButton, 2);
 * @endcode
 */
class GWENVIEWLIB_EXPORT InvisibleButtonGroup : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int current READ selected WRITE setSelected NOTIFY selectionChanged USER true)
public:
    explicit InvisibleButtonGroup(QWidget* parent = nullptr);
    ~InvisibleButtonGroup() override;

    int selected() const;

    void addButton(QAbstractButton* button, int id);

public Q_SLOTS:
    void setSelected(int id);

Q_SIGNALS:
    void selectionChanged(int id);

private:
    InvisibleButtonGroupPrivate* const d;
};

} // namespace

#endif /* INVISIBLEBUTTONGROUP_H */
