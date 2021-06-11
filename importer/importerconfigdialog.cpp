// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
// Self
#include "importerconfigdialog.h"

// Qt
#include <QTextDocument>
#include <QDateTime>

// KF

// Local
#include "filenameformater.h"
#include "importerconfig.h"
#include "ui_importerconfigdialog.h"

namespace Gwenview
{

static const QString PREVIEW_FILENAME = QStringLiteral("PICT0012.JPG");
static const QDateTime PREVIEW_DATETIME = QDateTime(QDate(2009, 10, 25), QTime(17, 51, 18));

struct ImporterConfigDialogPrivate : public Ui_ImporterConfigDialog
{
    ImporterConfigDialog* q;

    void setupHelpText()
    {
        QString helpText = "<ul>";
        FileNameFormater::HelpMap map = FileNameFormater::helpMap();
        FileNameFormater::HelpMap::ConstIterator
        it = map.constBegin(),
        end = map.constEnd();
        for (; it != end; ++it) {
            QString keyword = '{' + it.key() + '}';
            QString explanation = it.value().toHtmlEscaped();
            QString link = QStringLiteral("<a href='%1'>%1</a>").arg(keyword);
            helpText += "<li>" + i18nc("%1 is the importer keyword, %2 is keyword explanation", "%1: %2", link, explanation) + "</li>";
        }
        helpText += "</ul>";
        mRenameFormatHelpLabel->setText(helpText);

        QObject::connect(mRenameFormatHelpLabel, SIGNAL(linkActivated(QString)),
                         q, SLOT(slotHelpLinkActivated(QString)));
    }
};

ImporterConfigDialog::ImporterConfigDialog(QWidget* parent)
: KConfigDialog(parent, QStringLiteral("Importer Settings"), ImporterConfig::self())
, d(new ImporterConfigDialogPrivate)
{
    d->q = this;
    auto* widget = new QWidget;
    d->setupUi(widget);
    setFaceType(KPageDialog::Plain);
    addPage(widget, QString());

    connect(d->kcfg_AutoRenameFormat, &QLineEdit::textChanged,
            this, &ImporterConfigDialog::updatePreview);

    d->setupHelpText();
    updatePreview();
}

ImporterConfigDialog::~ImporterConfigDialog()
{
    delete d;
}

void ImporterConfigDialog::slotHelpLinkActivated(const QString& keyword)
{
    d->kcfg_AutoRenameFormat->insert(keyword);
}

void ImporterConfigDialog::updatePreview()
{
    FileNameFormater formater(d->kcfg_AutoRenameFormat->text());
    d->mPreviewOutputLabel->setText(formater.format(QUrl::fromLocalFile('/' + PREVIEW_FILENAME), PREVIEW_DATETIME));
}

} // namespace
