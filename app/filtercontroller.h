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
#ifndef FILTERCONTROLLER_H
#define FILTERCONTROLLER_H

#include <config-gwenview.h>

// Qt
#include <QDate>
#include <QDir>
#include <QImageReader>
#include <QList>
#include <QMimeDatabase>
#include <QMimeType>
#include <QObject>
#include <QWidget>

// KF
#include <KDirModel>
#include <KFileItem>

// Local
#include <lib/datewidget.h>
#include <lib/mimetypeutils.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/timeutils.h>
#include <semanticinfo/semanticinfodirmodel.h>

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
// KF
#include <KRatingWidget>

// Local
#include <lib/semanticinfo/abstractsemanticinfobackend.h>
#include <lib/semanticinfo/tagmodel.h>
#endif

class QAction;
class QFrame;
class QLineEdit;
class QLabel;
class QComboBox;
class KComboBox;

namespace Gwenview
{
class SortedDirModel;

/**
 * An AbstractSortedDirModelFilter which filters on the file names
 */
class NameFilter : public AbstractSortedDirModelFilter
{
public:
    enum Mode {
        Contains,
        DoesNotContain,
    };
    NameFilter(SortedDirModel *model)
        : AbstractSortedDirModelFilter(model)
        , mText()
        , mMode(Contains)
    {
    }

    bool needsSemanticInfo() const override
    {
        return false;
    }

    bool recursiveNameSearch(const QDir &startingDir) const
    {
        if (!startingDir.exists()) {
            return false;
        }

        const QStringList children = startingDir.entryList(QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::AllEntries);
        const QFileInfoList childrenData = startingDir.entryInfoList(QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::AllEntries);
        const QStringList acceptedMimeTypes = MimeTypeUtils::imageMimeTypes();
        // check if current directory has a match
        const QMimeDatabase db;
        for (int i = 0, count = childrenData.count(); i < count; ++i) {
            if ((children[i].contains(mText, Qt::CaseInsensitive) == (mMode == Contains))) {
                const QMimeType mime = db.mimeTypeForFile(childrenData[i]);
                if (acceptedMimeTypes.contains(mime.name())) {
                    return true;
                }
            }
        }
        // if not check for subdirectories and search those
        // call this function again if there are any directories in the children
        for (int i = 0, count = childrenData.count(); i < count; ++i) {
            if (childrenData[i].isDir() && recursiveNameSearch(QDir(childrenData[i].absoluteFilePath()))) {
                // if we have found a match, there is no need to search adjacent folders
                return true;
            }
        }
        return false;
    }

    bool acceptsIndex(const QModelIndex &index) const override
    {
        if (mText.isEmpty()) {
            return true;
        }
        const SemanticInfoDirModel *currentModel = static_cast<const SemanticInfoDirModel *>(index.model());
        const KFileItem fileItem = currentModel->itemForIndex(index);
        switch (mMode) {
        case Contains:
            if (currentModel->hasChildren(index)) {
                const QDir currentDir = QDir(fileItem.localPath());
                return recursiveNameSearch(currentDir);
            }
            return index.data().toString().contains(mText, Qt::CaseInsensitive);
        default: /*DoesNotContain:*/
            if (currentModel->hasChildren(index)) {
                const QDir currentDir = QDir(fileItem.localPath());
                return recursiveNameSearch(currentDir);
            }
            return !index.data().toString().contains(mText, Qt::CaseInsensitive);
        }
    }

    void setText(const QString &text)
    {
        mText = text;
        model()->applyFilters();
    }

    void setMode(Mode mode)
    {
        mMode = mode;
        model()->applyFilters();
    }

private:
    QString mText;
    Mode mMode;
};

class NameFilterWidget : public QWidget
{
    Q_OBJECT
public:
    NameFilterWidget(SortedDirModel *);
    ~NameFilterWidget() override;

private Q_SLOTS:
    void applyNameFilter();

private:
    QPointer<NameFilter> mFilter;
    KComboBox *mModeComboBox;
    QLineEdit *mLineEdit;
};

/**
 * An AbstractSortedDirModelFilter which filters on the file dates
 */
class DateFilter : public AbstractSortedDirModelFilter
{
public:
    enum Mode {
        GreaterOrEqual,
        Equal,
        LessOrEqual,
    };
    DateFilter(SortedDirModel *model)
        : AbstractSortedDirModelFilter(model)
        , mMode(GreaterOrEqual)
    {
    }

    bool needsSemanticInfo() const override
    {
        return false;
    }

    bool acceptsIndex(const QModelIndex &index) const override
    {
        if (!mDate.isValid()) {
            return true;
        }
        KFileItem fileItem = model()->itemForSourceIndex(index);
        QDate date = TimeUtils::dateTimeForFileItem(fileItem).date();
        switch (mMode) {
        case GreaterOrEqual:
            return date >= mDate;
        case Equal:
            return date == mDate;
        default: /* LessOrEqual */
            return date <= mDate;
        }
    }

    void setDate(const QDate &date)
    {
        mDate = date;
        model()->applyFilters();
    }

    void setMode(Mode mode)
    {
        mMode = mode;
        model()->applyFilters();
    }

private:
    QDate mDate;
    Mode mMode;
};

class DateFilterWidget : public QWidget
{
    Q_OBJECT
public:
    DateFilterWidget(SortedDirModel *);
    ~DateFilterWidget() override;

private Q_SLOTS:
    void applyDateFilter();

private:
    QPointer<DateFilter> mFilter;
    KComboBox *mModeComboBox;
    DateWidget *mDateWidget;
};

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
/**
 * An AbstractSortedDirModelFilter which filters on file ratings
 */
class RatingFilter : public AbstractSortedDirModelFilter
{
public:
    enum Mode {
        GreaterOrEqual,
        Equal,
        LessOrEqual,
    };

    RatingFilter(SortedDirModel *model)
        : AbstractSortedDirModelFilter(model)
        , mRating(0)
        , mMode(GreaterOrEqual)
    {
    }

    bool needsSemanticInfo() const override
    {
        return true;
    }

    bool acceptsIndex(const QModelIndex &index) const override
    {
        SemanticInfo info = model()->semanticInfoForSourceIndex(index);
        switch (mMode) {
        case GreaterOrEqual:
            return info.mRating >= mRating;
        case Equal:
            return info.mRating == mRating;
        default: /* LessOrEqual */
            return info.mRating <= mRating;
        }
    }

    void setRating(int value)
    {
        mRating = value;
        model()->applyFilters();
    }

    void setMode(Mode mode)
    {
        mMode = mode;
        model()->applyFilters();
    }

private:
    int mRating;
    Mode mMode;
};

class RatingFilterWidget : public QWidget
{
    Q_OBJECT
public:
    RatingFilterWidget(SortedDirModel *);
    ~RatingFilterWidget() override;

private Q_SLOTS:
    void slotRatingChanged(int value);
    void updateFilterMode();

private:
    KComboBox *mModeComboBox;
    KRatingWidget *mRatingWidget;
    QPointer<RatingFilter> mFilter;
};

/**
 * An AbstractSortedDirModelFilter which filters on associated tags
 */
class TagFilter : public AbstractSortedDirModelFilter
{
public:
    TagFilter(SortedDirModel *model)
        : AbstractSortedDirModelFilter(model)
        , mWantMatchingTag(true)
    {
    }

    bool needsSemanticInfo() const override
    {
        return true;
    }

    bool acceptsIndex(const QModelIndex &index) const override
    {
        if (mTag.isEmpty()) {
            return true;
        }
        SemanticInfo info = model()->semanticInfoForSourceIndex(index);
        if (mWantMatchingTag) {
            return info.mTags.contains(mTag);
        } else {
            return !info.mTags.contains(mTag);
        }
    }

    void setTag(const SemanticInfoTag &tag)
    {
        mTag = tag;
        model()->applyFilters();
    }

    void setWantMatchingTag(bool value)
    {
        mWantMatchingTag = value;
        model()->applyFilters();
    }

private:
    SemanticInfoTag mTag;
    bool mWantMatchingTag;
};

class TagFilterWidget : public QWidget
{
    Q_OBJECT
public:
    TagFilterWidget(SortedDirModel *);
    ~TagFilterWidget() override;

private Q_SLOTS:
    void updateTagSetFilter();

private:
    KComboBox *mModeComboBox;
    QComboBox *mTagComboBox;
    QPointer<TagFilter> mFilter;
};
#endif

/**
 * An AbstractSortedDirModelFilter which filters on the image dimensions
 */
class ImageDimensions : public AbstractSortedDirModelFilter
{
public:
    static const uint32_t minImageSizePx = 0; /**< Used to constraint the QLineEdit of the UI  */
    static const uint32_t maxImageSizePx = 100000; /**< Used to constraint the QLineEdit of the UI  */

    enum Mode {
        GreaterOrEqual,
        Equal,
        LessOrEqual,
    };

    ImageDimensions(SortedDirModel *model)
        : AbstractSortedDirModelFilter(model)
        , mMode(GreaterOrEqual)
        , mHeight(0)
        , mWidth(0)
    {
    }

    bool needsSemanticInfo() const override
    {
        return false;
    }

    bool acceptsIndex(const QModelIndex &index) const override
    {
        KFileItem fileItem = model()->itemForSourceIndex(index);
        QImageReader reader(fileItem.localPath());
        const QSize sizeOfImage = reader.size();
        const uint32_t height = sizeOfImage.height();
        const uint32_t width = sizeOfImage.width();

        switch (mMode) {
        case GreaterOrEqual: {
            return width >= mWidth && height >= mHeight;
        }
        case Equal: {
            return ((mWidth == 0) || (mWidth == width)) && ((mHeight == 0) || (mHeight == height));
        }
        default: /* LessOrEqual */
        {
            return ((mWidth == 0) || (width <= mWidth)) && ((mHeight == 0) || (height <= mHeight));
        }
        }

        return true;
    }

    /**
     * Sets the filtering mode
     */
    void setMode(Mode mode)
    {
        mMode = mode;
        model()->applyFilters();
    }

    /**
     * Sets the image dimesions that the filter will use
     */
    void setSizes(uint32_t width, uint32_t height)
    {
        mWidth = width;
        mHeight = height;
        model()->applyFilters();
    }

private:
    Mode mMode; /**< Contains the filtering mode of the image dimension */
    uint32_t mHeight; /**< Holds the height that the filter will check */
    uint32_t mWidth; /**< Holds the width that the filter will check */
};

class ImageDimensionsWidget : public QWidget
{
    Q_OBJECT
public:
    ImageDimensionsWidget(SortedDirModel *);
    ~ImageDimensionsWidget() override;

private Q_SLOTS:
    void applyImageDimensions();

private:
    QPointer<ImageDimensions> mFilter;
    KComboBox *mModeComboBox;
    KComboBox *mDimensionComboBox;
    QLineEdit *mLineEditWidth;
    QLineEdit *mLineEditHeight;
    QLabel *mLabelWidth;
    QLabel *mLabelHeight;
};

/**
 * This class manages the filter widgets in the filter frame and assign the
 * corresponding filters to the SortedDirModel
 */
class FilterController : public QObject
{
    Q_OBJECT
public:
    FilterController(QFrame *filterFrame, SortedDirModel *model);

    QList<QAction *> actionList() const;

private Q_SLOTS:
    void addFilterByName();
    void addFilterByDate();
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    void addFilterByRating();
    void addFilterByTag();
#endif
    void addFilterByImageDimensions();
    void slotFilterWidgetClosed();

private:
    void addAction(const QString &text, const char *slot, const QKeySequence &shortcut);
    void addFilter(QWidget *widget);

    FilterController *q;
    QFrame *mFrame;
    SortedDirModel *mDirModel;
    QList<QAction *> mActionList;

    int mFilterWidgetCount; /**< How many filter widgets are in mFrame */
};

} // namespace

#endif /* FILTERCONTROLLER_H */
