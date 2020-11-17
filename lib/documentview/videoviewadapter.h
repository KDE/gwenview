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
#ifndef VIDEOVIEWADAPTER_H
#define VIDEOVIEWADAPTER_H

#include <lib/gwenviewlib_export.h>

// Qt

// KF

// Local
#include <lib/documentview/abstractdocumentviewadapter.h>

namespace Gwenview
{

struct VideoViewAdapterPrivate;
class GWENVIEWLIB_EXPORT VideoViewAdapter : public AbstractDocumentViewAdapter
{
    Q_OBJECT
public:
    VideoViewAdapter();
    ~VideoViewAdapter() override;

    MimeTypeUtils::Kind kind() const override
    {
        return MimeTypeUtils::KIND_VIDEO;
    }

    Document::Ptr document() const override;

    void setDocument(const Document::Ptr &) override;

Q_SIGNALS:
    void videoFinished();

protected:
    bool eventFilter(QObject*, QEvent*) override;

private Q_SLOTS:
    void slotPlayPauseClicked();
    void updatePlayUi();
    void slotMuteClicked();
    void updateMuteAction();
    void slotVolumeSliderChanged(int);
    void slotOutputVolumeChanged(qreal);
    void slotSeekSliderActionTriggered(int);
    void slotTicked(qint64);

private:
    friend struct VideoViewAdapterPrivate;
    VideoViewAdapterPrivate* const d;
};

} // namespace

#endif /* VIDEOVIEWADAPTER_H */
