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
#ifndef MESSAGEVIEWADAPTER_H
#define MESSAGEVIEWADAPTER_H

// Qt

// KDE

// Local
#include <lib/documentview/abstractdocumentviewadapter.h>

namespace Gwenview
{

struct MessageViewAdapterPrivate;
class MessageViewAdapter : public AbstractDocumentViewAdapter
{
    Q_OBJECT
public:
    MessageViewAdapter();
    ~MessageViewAdapter();

    virtual MimeTypeUtils::Kind kind() const Q_DECL_OVERRIDE
    {
        return MimeTypeUtils::KIND_UNKNOWN;
    }

    virtual Document::Ptr document() const Q_DECL_OVERRIDE;

    virtual void setDocument(Document::Ptr) Q_DECL_OVERRIDE;

    void setInfoMessage(const QString&);

    void setErrorMessage(const QString& main, const QString& detail = QString());

protected:
    bool eventFilter(QObject*, QEvent*) Q_DECL_OVERRIDE;

private:
    MessageViewAdapterPrivate* const d;
};

} // namespace

#endif /* MESSAGEVIEWADAPTER_H */
