// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "imagemetainfomodel.h"
#include "config-gwenview.h"

// Qt
#include <QSize>
#include <QDebug>
#include <QLocale>

// KDE
#include <KFileItem>
#include <KLocalizedString>
#include <KFormat>

// Exiv2
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>
#include <exiv2/iptc.hpp>

// Local
#ifdef HAVE_FITS
#include "imageformats/fitsformat/fitsdata.h"
#include "urlutils.h"
#endif

namespace Gwenview
{

enum GroupRow {
    NoGroupSpace = -2,
    NoGroup = -1,
    GeneralGroup,
    ExifGroup,
#ifdef HAVE_FITS
    FitsGroup,
#endif
    IptcGroup,
    XmpGroup
};

class MetaInfoGroup
{
public:
    enum {
        InvalidRow = -1
    };

    class Entry
    {
    public:
        Entry(const QString& key, const QString& label, const QString& value)
            : mKey(key), mLabel(label.trimmed()), mValue(value.trimmed())
        {}

        QString key() const
        {
            return mKey;
        }
        QString label() const
        {
            return mLabel;
        }

        QString value() const
        {
            return mValue;
        }
        void setValue(const QString& value)
        {
            mValue = value.trimmed();
        }

        void appendValue(const QString& value)
        {
            if (!mValue.isEmpty()) {
                mValue += QLatin1Char('\n');
            }
            mValue += value.trimmed();
        }

    private:
        QString mKey;
        QString mLabel;
        QString mValue;
    };

    MetaInfoGroup(const QString& label)
        : mLabel(label)
        {}

    ~MetaInfoGroup()
    {
        qDeleteAll(mList);
    }

    void clear()
    {
        qDeleteAll(mList);
        mList.clear();
        mRowForKey.clear();
    }

    void addEntry(const QString& key, const QString& label, const QString& value)
    {
        addEntry(new Entry(key, label, value));
    }

    void addEntry(Entry* entry)
    {
        mList << entry;
        mRowForKey[entry->key()] = mList.size() - 1;
    }

    void getInfoForKey(const QString& key, QString* label, QString* value) const
    {
        Entry* entry = getEntryForKey(key);
        if (entry) {
            *label = entry->label();
            *value = entry->value();
        }
    }

    QString getKeyAt(int row) const
    {
        Q_ASSERT(row < mList.size());
        return mList[row]->key();
    }

    QString getLabelForKeyAt(int row) const
    {
        Q_ASSERT(row < mList.size());
        return mList[row]->label();
    }

    QString getValueForKeyAt(int row) const
    {
        Q_ASSERT(row < mList.size());
        return mList[row]->value();
    }

    void setValueForKeyAt(int row, const QString& value)
    {
        Q_ASSERT(row < mList.size());
        mList[row]->setValue(value);
    }

    int getRowForKey(const QString& key) const
    {
        return mRowForKey.value(key, InvalidRow);
    }

    int size() const
    {
        return mList.size();
    }

    QString label() const
    {
        return mLabel;
    }

    const QList<Entry*>& entryList() const {
        return mList;
    }

private:
    Entry* getEntryForKey(const QString& key) const
    {
        int row = getRowForKey(key);
        if (row == InvalidRow) {
            return nullptr;
        }
        return mList[row];
    }

    QList<Entry*> mList;
    QHash<QString, int> mRowForKey;
    QString mLabel;
};

struct ImageMetaInfoModelPrivate
{
    QVector<MetaInfoGroup*> mMetaInfoGroupVector;
    ImageMetaInfoModel* q;

    void clearGroup(MetaInfoGroup* group, const QModelIndex& parent)
    {
        if (group->size() > 0) {
            q->beginRemoveRows(parent, 0, group->size() - 1);
            group->clear();
            q->endRemoveRows();
        }
    }

    void setGroupEntryValue(GroupRow groupRow, const QString& key, const QString& value)
    {
        MetaInfoGroup* group = mMetaInfoGroupVector[groupRow];
        int entryRow = group->getRowForKey(key);
        if (entryRow == MetaInfoGroup::InvalidRow) {
            qWarning() << "No row for key" << key;
            return;
        }
        group->setValueForKeyAt(entryRow, value);
        QModelIndex groupIndex = q->index(groupRow, 0);
        QModelIndex entryIndex = q->index(entryRow, 1, groupIndex);
        emit q->dataChanged(entryIndex, entryIndex);
    }

    QVariant displayData(const QModelIndex& index) const
    {
        if (index.internalId() == NoGroup) {
            if (index.column() != 0) {
                return QVariant();
            }
            QString label = mMetaInfoGroupVector[index.row()]->label();
            return QVariant(label);
        }

        if (index.internalId() == NoGroupSpace) {
            return QString();
        }

        MetaInfoGroup* group = mMetaInfoGroupVector[index.internalId()];
        if (index.column() == 0) {
            return group->getLabelForKeyAt(index.row());
        } else {
            return group->getValueForKeyAt(index.row());
        }
    }

    void initGeneralGroup()
    {
        MetaInfoGroup* group = mMetaInfoGroupVector[GeneralGroup];
        group->addEntry(QStringLiteral("General.Name"), i18nc("@item:intable Image file name", "Name"), QString());
        group->addEntry(QStringLiteral("General.Size"), i18nc("@item:intable", "File Size"), QString());
        group->addEntry(QStringLiteral("General.Time"), i18nc("@item:intable", "File Time"), QString());
        group->addEntry(QStringLiteral("General.ImageSize"), i18nc("@item:intable", "Image Size"), QString());
        group->addEntry(QStringLiteral("General.Comment"), i18nc("@item:intable", "Comment"), QString());
    }

    template <class Container, class Iterator>
    void fillExivGroup(const QModelIndex& parent, MetaInfoGroup* group, const Container& container)
    {
        // key aren't always unique (for example, "Iptc.Application2.Keywords"
        // may appear multiple times) so we can't know how many rows we will
        // insert before going through them. That's why we create a hash
        // before.
        typedef QHash<QString, MetaInfoGroup::Entry*> EntryHash;
        EntryHash hash;

        Iterator
        it = container.begin(),
        end = container.end();

        for (; it != end; ++it) {
            try {
                // Skip metadatum if its tag is an hex number
                if (it->tagName().substr(0, 2) == "0x") {
                    continue;
                }
                QString key = QString::fromUtf8(it->key().c_str());
                QString label = QString::fromLocal8Bit(it->tagLabel().c_str());
                std::ostringstream stream;
                stream << *it;
                QString value = QString::fromLocal8Bit(stream.str().c_str());

                EntryHash::iterator hashIt = hash.find(key);
                if (hashIt != hash.end()) {
                    hashIt.value()->appendValue(value);
                } else {
                    hash.insert(key, new MetaInfoGroup::Entry(key, label, value));
                }
            } catch (const Exiv2::Error& error) {
                qWarning() << "Failed to read some meta info:" << error.what();
            }
        }

        if (hash.isEmpty()) {
            return;
        }
        q->beginInsertRows(parent, 0, hash.size() - 1);
        Q_FOREACH(MetaInfoGroup::Entry * entry, hash) {
            group->addEntry(entry);
        }
        q->endInsertRows();
    }
};

ImageMetaInfoModel::ImageMetaInfoModel()
: d(new ImageMetaInfoModelPrivate)
{
    d->q = this;
#ifdef HAVE_FITS
    d->mMetaInfoGroupVector.resize(5);
#else
    d->mMetaInfoGroupVector.resize(4);
#endif
    d->mMetaInfoGroupVector[GeneralGroup] = new MetaInfoGroup(i18nc("@title:group General info about the image", "General"));
    d->mMetaInfoGroupVector[ExifGroup] = new MetaInfoGroup(QStringLiteral("EXIF"));
#ifdef HAVE_FITS
    d->mMetaInfoGroupVector[FitsGroup] = new MetaInfoGroup(QStringLiteral("FITS"));
#endif
    d->mMetaInfoGroupVector[IptcGroup] = new MetaInfoGroup(QStringLiteral("IPTC"));
    d->mMetaInfoGroupVector[XmpGroup]  = new MetaInfoGroup(QStringLiteral("XMP"));
    d->initGeneralGroup();
}

ImageMetaInfoModel::~ImageMetaInfoModel()
{
    qDeleteAll(d->mMetaInfoGroupVector);
    delete d;
}

void ImageMetaInfoModel::setUrl(const QUrl &url)
{
    KFileItem item(url);
    const QString sizeString = KFormat().formatByteSize(item.size());
    const QString timeString = QLocale().toString(item.time(KFileItem::ModificationTime), QLocale::LongFormat);

    d->setGroupEntryValue(GeneralGroup, QStringLiteral("General.Name"), item.name());
    d->setGroupEntryValue(GeneralGroup, QStringLiteral("General.Size"), sizeString);
    d->setGroupEntryValue(GeneralGroup, QStringLiteral("General.Time"), timeString);

#ifdef HAVE_FITS
    if (UrlUtils::urlIsFastLocalFile(url) && (url.fileName().endsWith(QStringLiteral(".fit"), Qt::CaseInsensitive) ||
        url.fileName().endsWith(QStringLiteral(".fits"), Qt::CaseInsensitive))) {
        FITSData fitsLoader;
        MetaInfoGroup* group = d->mMetaInfoGroupVector[FitsGroup];
        QFile file(url.toLocalFile());

        if (!file.open(QIODevice::ReadOnly)) {
            return;
        }

        if (fitsLoader.loadFITS(file)) {
            QString recordList;
            int nkeys = 0;

            fitsLoader.getFITSRecord(recordList, nkeys);
            for (int i = 0; i < nkeys; i++)
            {
                QString record = recordList.mid(i * 80, 80);
                QString key;
                QString keyStr;
                QString value;

                if (!record.contains(QStringLiteral("="))) {
                    key = record.section(QLatin1Char(' '), 0, 0).simplified();
                    keyStr = key;
                    value = record.section(QLatin1Char(' '), 1, -1).simplified();
                } else {
                    key = record.section(QLatin1Char('='), 0, 0).simplified();
                    if (record.contains(QLatin1Char('/')) {
                        keyStr = record.section(QLatin1Char('/'), -1, -1).simplified();
                        value = record.section(QLatin1Char('='), 1, -1).section(QLatin1Char('/'), 0, 0);
                    } else {
                        keyStr = key;
                        value = record.section(QLatin1Char('='), 1, -1);
                    }
                    value.remove(QStringLiteral("\'"));
                    value = value.simplified();
                }
                if (value.isEmpty()) {
                    continue;
                }

                // Check if the value is a number and make it more readable
                bool ok = false;
                float number = value.toFloat(&ok);

                if (ok) {
                    value = QString::number(number);
                }

                group->addEntry(QStringLiteral("Fits.")+key, keyStr, value);
            }
        }
    }
#endif
}

void ImageMetaInfoModel::setImageSize(const QSize& size)
{
    QString imageSize;
    if (size.isValid()) {
        imageSize = i18nc(
                        "@item:intable %1 is image width, %2 is image height",
                        "%1x%2", size.width(), size.height());

        double megaPixels = size.width() * size.height() / 1000000.;
        if (megaPixels > 0.1) {
            QString megaPixelsString = QString::number(megaPixels, 'f', 1);
            imageSize += QLatin1Char(' ');
            imageSize += i18nc(
                             "@item:intable %1 is number of millions of pixels in image",
                             "(%1MP)", megaPixelsString);
        }
    } else {
        imageSize = QLatin1Char('-');
    }
    d->setGroupEntryValue(GeneralGroup, QStringLiteral("General.ImageSize"), imageSize);
}

void ImageMetaInfoModel::setExiv2Image(const Exiv2::Image* image)
{
    MetaInfoGroup* exifGroup = d->mMetaInfoGroupVector[ExifGroup];
    MetaInfoGroup* iptcGroup = d->mMetaInfoGroupVector[IptcGroup];
    MetaInfoGroup* xmpGroup  = d->mMetaInfoGroupVector[XmpGroup];
    QModelIndex exifIndex = index(ExifGroup, 0);
    QModelIndex iptcIndex = index(IptcGroup, 0);
    QModelIndex xmpIndex  = index(XmpGroup, 0);
    d->clearGroup(exifGroup, exifIndex);
    d->clearGroup(iptcGroup, iptcIndex);
    d->clearGroup(xmpGroup,  xmpIndex);

    if (!image) {
        return;
    }

    d->setGroupEntryValue(GeneralGroup, QStringLiteral("General.Comment"), QString::fromUtf8(image->comment().c_str()));

    if (image->checkMode(Exiv2::mdExif) & Exiv2::amRead) {
        const Exiv2::ExifData& exifData = image->exifData();
        d->fillExivGroup<Exiv2::ExifData, Exiv2::ExifData::const_iterator>(exifIndex, exifGroup, exifData);
    }

    if (image->checkMode(Exiv2::mdIptc) & Exiv2::amRead) {
        const Exiv2::IptcData& iptcData = image->iptcData();
        d->fillExivGroup<Exiv2::IptcData, Exiv2::IptcData::const_iterator>(iptcIndex, iptcGroup, iptcData);
    }

    if (image->checkMode(Exiv2::mdXmp) & Exiv2::amRead) {
        const Exiv2::XmpData& xmpData = image->xmpData();
        d->fillExivGroup<Exiv2::XmpData, Exiv2::XmpData::const_iterator>(xmpIndex, xmpGroup, xmpData);
    }
}

void ImageMetaInfoModel::getInfoForKey(const QString& key, QString* label, QString* value) const
{
    MetaInfoGroup* group;
    if (key.startsWith(QLatin1String("General"))) {
        group = d->mMetaInfoGroupVector[GeneralGroup];
    } else if (key.startsWith(QLatin1String("Exif"))) {
        group = d->mMetaInfoGroupVector[ExifGroup];
#ifdef HAVE_FITS
    } else if (key.startsWith(QLatin1String("Fits"))) {
        group = d->mMetaInfoGroupVector[FitsGroup];
#endif
    } else if (key.startsWith(QLatin1String("Iptc"))) {
        group = d->mMetaInfoGroupVector[IptcGroup];
    } else if (key.startsWith(QLatin1String("Xmp"))) {
        group = d->mMetaInfoGroupVector[XmpGroup];
    } else {
        qWarning() << "Unknown metainfo key" << key;
        return;
    }
    group->getInfoForKey(key, label, value);
}

QString ImageMetaInfoModel::getValueForKey(const QString& key) const
{
    QString label, value;
    getInfoForKey(key, &label, &value);
    return value;
}

QString ImageMetaInfoModel::keyForIndex(const QModelIndex& index) const
{
    if (index.internalId() == NoGroup) {
        return QString();
    }
    MetaInfoGroup* group = d->mMetaInfoGroupVector[index.internalId()];
    return group->getKeyAt(index.row());
}

QModelIndex ImageMetaInfoModel::index(int row, int col, const QModelIndex& parent) const
{
    if (col < 0 || col > 1) {
        return QModelIndex();
    }
    if (!parent.isValid()) {
        // This is a group
        if (row < 0 || row >= d->mMetaInfoGroupVector.size()) {
            return QModelIndex();
        }
        return createIndex(row, col, col == 0 ? NoGroup : NoGroupSpace);
    } else {
        // This is an entry
        int group = parent.row();
        if (row < 0 || row >= d->mMetaInfoGroupVector[group]->size()) {
            return QModelIndex();
        }
        return createIndex(row, col, group);
    }
}

QModelIndex ImageMetaInfoModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    if (index.internalId() == NoGroup || index.internalId() == NoGroupSpace) {
        return QModelIndex();
    } else {
        return createIndex(index.internalId(), 0, NoGroup);
    }
}

int ImageMetaInfoModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return d->mMetaInfoGroupVector.size();
    } else if (parent.internalId() == NoGroup) {
        return d->mMetaInfoGroupVector[parent.row()]->size();
    } else {
        return 0;
    }
}

int ImageMetaInfoModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 2;
}

QVariant ImageMetaInfoModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        return d->displayData(index);
    default:
        return QVariant();
    }
}

QVariant ImageMetaInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole) {
        return QVariant();
    }

    QString caption;
    if (section == 0) {
        caption = i18nc("@title:column", "Property");
    } else if (section == 1) {
        caption = i18nc("@title:column", "Value");
    } else {
        qWarning() << "Unknown section" << section;
    }

    return QVariant(caption);
}

} // namespace
