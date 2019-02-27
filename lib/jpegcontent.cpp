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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "jpegcontent.h"

// System
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include <jpeglib.h>
#include "transupp.h"
}

// Qt
#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QImageWriter>
#include <QTransform>
#include <QDebug>

// KDE
#include <KLocalizedString>

// Exiv2
#include <exiv2/exiv2.hpp>

// Local
#include "jpegerrormanager.h"
#include "iodevicejpegsourcemanager.h"
#include "exiv2imageloader.h"
#include "gwenviewconfig.h"

namespace Gwenview
{

const int INMEM_DST_DELTA = 4096;

//-----------------------------------------------
//
// In-memory data destination manager for libjpeg
//
//-----------------------------------------------
struct inmem_dest_mgr : public jpeg_destination_mgr
{
    QByteArray* mOutput;

    void dump()
    {
        qDebug() << "dest_mgr:\n";
        qDebug() << "- next_output_byte: " << next_output_byte;
        qDebug() << "- free_in_buffer: " << free_in_buffer;
        qDebug() << "- output size: " << mOutput->size();
    }
};

void inmem_init_destination(j_compress_ptr cinfo)
{
    inmem_dest_mgr* dest = (inmem_dest_mgr*)(cinfo->dest);
    if (dest->mOutput->size() == 0) {
        dest->mOutput->resize(INMEM_DST_DELTA);
    }
    dest->free_in_buffer = dest->mOutput->size();
    dest->next_output_byte = (JOCTET*)(dest->mOutput->data());
}

boolean inmem_empty_output_buffer(j_compress_ptr cinfo)
{
    inmem_dest_mgr* dest = (inmem_dest_mgr*)(cinfo->dest);
    dest->mOutput->resize(dest->mOutput->size() + INMEM_DST_DELTA);
    dest->next_output_byte = (JOCTET*)(dest->mOutput->data() + dest->mOutput->size() - INMEM_DST_DELTA);
    dest->free_in_buffer = INMEM_DST_DELTA;

    return true;
}

void inmem_term_destination(j_compress_ptr cinfo)
{
    inmem_dest_mgr* dest = (inmem_dest_mgr*)(cinfo->dest);
    int finalSize = dest->next_output_byte - (JOCTET*)(dest->mOutput->data());
    Q_ASSERT(finalSize >= 0);
    dest->mOutput->resize(finalSize);
}

//---------------------
//
// JpegContent::Private
//
//---------------------
struct JpegContent::Private
{
    // JpegContent usually stores the image pixels as compressed JPEG data in
    // mRawData. However if the image is set with setImage() because the user
    // performed a lossy image manipulation, mRawData is cleared and the image
    // pixels are kept in mImage until updateRawDataFromImage() is called.
    QImage mImage;
    QByteArray mRawData;
    QSize mSize;
    QString mComment;
    bool mPendingTransformation;
    QTransform mTransformMatrix;
    Exiv2::ExifData mExifData;
    QString mErrorString;

    Private()
    {
        mPendingTransformation = false;
    }

    void setupInmemDestination(j_compress_ptr cinfo, QByteArray* outputData)
    {
        Q_ASSERT(!cinfo->dest);
        inmem_dest_mgr* dest = (inmem_dest_mgr*)
                               (*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                       sizeof(inmem_dest_mgr));
        cinfo->dest = (struct jpeg_destination_mgr*)(dest);

        dest->init_destination = inmem_init_destination;
        dest->empty_output_buffer = inmem_empty_output_buffer;
        dest->term_destination = inmem_term_destination;

        dest->mOutput = outputData;
    }
    bool readSize()
    {
        struct jpeg_decompress_struct srcinfo;

        // Init JPEG structs
        JPEGErrorManager errorManager;

        // Initialize the JPEG decompression object
        srcinfo.err = &errorManager;
        jpeg_create_decompress(&srcinfo);
        if (setjmp(errorManager.jmp_buffer)) {
            qCritical() << "libjpeg fatal error\n";
            return false;
        }

        // Specify data source for decompression
        QBuffer buffer(&mRawData);
        buffer.open(QIODevice::ReadOnly);
        IODeviceJpegSourceManager::setup(&srcinfo, &buffer);

        // Read the header
        jcopy_markers_setup(&srcinfo, JCOPYOPT_ALL);
        int result = jpeg_read_header(&srcinfo, true);
        if (result != JPEG_HEADER_OK) {
            qCritical() << "Could not read jpeg header\n";
            jpeg_destroy_decompress(&srcinfo);
            return false;
        }
        mSize = QSize(srcinfo.image_width, srcinfo.image_height);

        jpeg_destroy_decompress(&srcinfo);
        return true;
    }

    bool updateRawDataFromImage()
    {
        QBuffer buffer;
        QImageWriter writer(&buffer, "jpeg");
        if (!writer.write(mImage)) {
            mErrorString = writer.errorString();
            return false;
        }
        mRawData = buffer.data();
        mImage = QImage();
        return true;
    }
};

//------------
//
// JpegContent
//
//------------
JpegContent::JpegContent()
{
    d = new JpegContent::Private();
}

JpegContent::~JpegContent()
{
    delete d;
}

bool JpegContent::load(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Could not open '" << path << "' for reading\n";
        return false;
    }
    return loadFromData(file.readAll());
}

bool JpegContent::loadFromData(const QByteArray& data)
{
    std::unique_ptr<Exiv2::Image> image;
    Exiv2ImageLoader loader;
    if (!loader.load(data)) {
        qCritical() << "Could not load image with Exiv2, reported error:" << loader.errorMessage();
    }
    image.reset(loader.popImage().release());

    return loadFromData(data, image.get());
}

bool JpegContent::loadFromData(const QByteArray& data, Exiv2::Image* exiv2Image)
{
    d->mPendingTransformation = false;
    d->mTransformMatrix.reset();

    d->mRawData = data;
    if (d->mRawData.size() == 0) {
        qCritical() << "No data\n";
        return false;
    }

    if (!d->readSize()) return false;

    d->mExifData = exiv2Image->exifData();
    d->mComment = QString::fromUtf8(exiv2Image->comment().c_str());

    if (!GwenviewConfig::applyExifOrientation()) {
        return true;
    }

    // Adjust the size according to the orientation
    switch (orientation()) {
    case TRANSPOSE:
    case ROT_90:
    case TRANSVERSE:
    case ROT_270:
        d->mSize.transpose();
        break;
    default:
        break;
    }

    return true;
}

QByteArray JpegContent::rawData() const
{
    return d->mRawData;
}

Orientation JpegContent::orientation() const
{
    Exiv2::ExifKey key("Exif.Image.Orientation");
    Exiv2::ExifData::iterator it = d->mExifData.findKey(key);

    // We do the same checks as in libexiv2's src/crwimage.cpp:
    // http://dev.exiv2.org/projects/exiv2/repository/entry/trunk/src/crwimage.cpp?rev=2681#L1336
    if (it == d->mExifData.end() || it->count() == 0 || it->typeId() != Exiv2::unsignedShort) {
        return NOT_AVAILABLE;
    }
    return Orientation(it->toLong());
}

int JpegContent::dotsPerMeterX() const
{
    return dotsPerMeter(QStringLiteral("XResolution"));
}

int JpegContent::dotsPerMeterY() const
{
    return dotsPerMeter(QStringLiteral("YResolution"));
}

int JpegContent::dotsPerMeter(const QString& keyName) const
{
    Exiv2::ExifKey keyResUnit("Exif.Image.ResolutionUnit");
    Exiv2::ExifData::iterator it = d->mExifData.findKey(keyResUnit);
    if (it == d->mExifData.end()) {
        return 0;
    }
    int res = it->toLong();
    QString keyVal = QStringLiteral("Exif.Image.") + keyName;
    Exiv2::ExifKey keyResolution(keyVal.toLocal8Bit().data());
    it = d->mExifData.findKey(keyResolution);
    if (it == d->mExifData.end()) {
        return 0;
    }
    // The unit for measuring XResolution and YResolution. The same unit is used for both XResolution and YResolution.
    //     If the image resolution in unknown, 2 (inches) is designated.
    //         Default = 2
    //         2 = inches
    //         3 = centimeters
    //         Other = reserved
    const float INCHESPERMETER = (100. / 2.54);
    switch (res) {
    case 3:  // dots per cm
        return int(it->toLong() * 100);
    default:  // dots per inch
        return int(it->toLong() * INCHESPERMETER);
    }

    return 0;
}

void JpegContent::resetOrientation()
{
    Exiv2::ExifKey key("Exif.Image.Orientation");
    Exiv2::ExifData::iterator it = d->mExifData.findKey(key);
    if (it == d->mExifData.end()) {
        return;
    }

    *it = uint16_t(NORMAL);
}

QSize JpegContent::size() const
{
    return d->mSize;
}

QString JpegContent::comment() const
{
    return d->mComment;
}

void JpegContent::setComment(const QString& comment)
{
    d->mComment = comment;
}

static QTransform createRotMatrix(int angle)
{
    QTransform matrix;
    matrix.rotate(angle);
    return matrix;
}

static QTransform createScaleMatrix(int dx, int dy)
{
    QTransform matrix;
    matrix.scale(dx, dy);
    return matrix;
}

struct OrientationInfo
{
    OrientationInfo()
    : orientation(NOT_AVAILABLE)
    , jxform(JXFORM_NONE)
    {}

    OrientationInfo(Orientation o, const QTransform &m, JXFORM_CODE j)
    : orientation(o), matrix(m), jxform(j)
    {}

    Orientation orientation;
    QTransform matrix;
    JXFORM_CODE jxform;
};
typedef QList<OrientationInfo> OrientationInfoList;

static const OrientationInfoList& orientationInfoList()
{
    static OrientationInfoList list;
    if (list.size() == 0) {
        QTransform rot90 = createRotMatrix(90);
        QTransform hflip = createScaleMatrix(-1, 1);
        QTransform vflip = createScaleMatrix(1, -1);

        list
                << OrientationInfo()
                << OrientationInfo(NORMAL, QTransform(), JXFORM_NONE)
                << OrientationInfo(HFLIP, hflip, JXFORM_FLIP_H)
                << OrientationInfo(ROT_180, createRotMatrix(180), JXFORM_ROT_180)
                << OrientationInfo(VFLIP, vflip, JXFORM_FLIP_V)
                << OrientationInfo(TRANSPOSE, hflip * rot90, JXFORM_TRANSPOSE)
                << OrientationInfo(ROT_90, rot90, JXFORM_ROT_90)
                << OrientationInfo(TRANSVERSE, vflip * rot90, JXFORM_TRANSVERSE)
                << OrientationInfo(ROT_270, createRotMatrix(270), JXFORM_ROT_270)
                ;
    }
    return list;
}

void JpegContent::transform(Orientation orientation)
{
    if (orientation != NOT_AVAILABLE && orientation != NORMAL) {
        d->mPendingTransformation = true;
        OrientationInfoList::ConstIterator it(orientationInfoList().begin()), end(orientationInfoList().end());
        for (; it != end; ++it) {
            if ((*it).orientation == orientation) {
                d->mTransformMatrix = (*it).matrix * d->mTransformMatrix;
                break;
            }
        }
        if (it == end) {
            qWarning() << "Could not find matrix for orientation\n";
        }
    }
}

#if 0
static void dumpMatrix(const QTransform& matrix)
{
    qDebug() << "matrix | " << matrix.m11() << ", " << matrix.m12() << " |\n";
    qDebug() << "       | " << matrix.m21() << ", " << matrix.m22() << " |\n";
    qDebug() << "       ( " << matrix.dx()  << ", " << matrix.dy()  << " )\n";
}
#endif

static bool matricesAreSame(const QTransform& m1, const QTransform& m2, double tolerance)
{
    return fabs(m1.m11() - m2.m11()) < tolerance
           && fabs(m1.m12() - m2.m12()) < tolerance
           && fabs(m1.m21() - m2.m21()) < tolerance
           && fabs(m1.m22() - m2.m22()) < tolerance
           && fabs(m1.dx()  - m2.dx()) < tolerance
           && fabs(m1.dy()  - m2.dy()) < tolerance;
}

static JXFORM_CODE findJxform(const QTransform& matrix)
{
    OrientationInfoList::ConstIterator it(orientationInfoList().begin()), end(orientationInfoList().end());
    for (; it != end; ++it) {
        if (matricesAreSame((*it).matrix, matrix, 0.001)) {
            return (*it).jxform;
        }
    }
    qWarning() << "findJxform: failed\n";
    return JXFORM_NONE;
}

void JpegContent::applyPendingTransformation()
{
    if (d->mRawData.size() == 0) {
        qCritical() << "No data loaded\n";
        return;
    }

    // The following code is inspired by jpegtran.c from the libjpeg

    // Init JPEG structs
    struct jpeg_decompress_struct srcinfo;
    struct jpeg_compress_struct dstinfo;
    jvirt_barray_ptr * src_coef_arrays;
    jvirt_barray_ptr * dst_coef_arrays;

    // Initialize the JPEG decompression object
    JPEGErrorManager srcErrorManager;
    srcinfo.err = &srcErrorManager;
    jpeg_create_decompress(&srcinfo);
    if (setjmp(srcErrorManager.jmp_buffer)) {
        qCritical() << "libjpeg error in src\n";
        return;
    }

    // Initialize the JPEG compression object
    JPEGErrorManager dstErrorManager;
    dstinfo.err = &dstErrorManager;
    jpeg_create_compress(&dstinfo);
    if (setjmp(dstErrorManager.jmp_buffer)) {
        qCritical() << "libjpeg error in dst\n";
        return;
    }

    // Specify data source for decompression
    QBuffer buffer(&d->mRawData);
    buffer.open(QIODevice::ReadOnly);
    IODeviceJpegSourceManager::setup(&srcinfo, &buffer);

    // Enable saving of extra markers that we want to copy
    jcopy_markers_setup(&srcinfo, JCOPYOPT_ALL);

    (void) jpeg_read_header(&srcinfo, true);

    // Init transformation
    jpeg_transform_info transformoption;
    memset(&transformoption, 0, sizeof(jpeg_transform_info));
    transformoption.transform = findJxform(d->mTransformMatrix);
    jtransform_request_workspace(&srcinfo, &transformoption);

    /* Read source file as DCT coefficients */
    src_coef_arrays = jpeg_read_coefficients(&srcinfo);

    /* Initialize destination compression parameters from source values */
    jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

    /* Adjust destination parameters if required by transform options;
    * also find out which set of coefficient arrays will hold the output.
    */
    dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo,
                      src_coef_arrays,
                      &transformoption);

    /* Specify data destination for compression */
    QByteArray output;
    output.resize(d->mRawData.size());
    d->setupInmemDestination(&dstinfo, &output);

    /* Start compressor (note no image data is actually written here) */
    jpeg_write_coefficients(&dstinfo, dst_coef_arrays);

    /* Copy to the output file any extra markers that we want to preserve */
    jcopy_markers_execute(&srcinfo, &dstinfo, JCOPYOPT_ALL);

    /* Execute image transformation, if any */
    jtransform_execute_transformation(&srcinfo, &dstinfo,
                                      src_coef_arrays,
                                      &transformoption);

    /* Finish compression and release memory */
    jpeg_finish_compress(&dstinfo);
    jpeg_destroy_compress(&dstinfo);
    (void) jpeg_finish_decompress(&srcinfo);
    jpeg_destroy_decompress(&srcinfo);

    // Set rawData to our new JPEG
    d->mRawData = output;
}

QImage JpegContent::thumbnail() const
{
    QImage image;
    if (!d->mExifData.empty()) {
#if(EXIV2_TEST_VERSION(0,17,91))
        Exiv2::ExifThumbC thumb(d->mExifData);
        Exiv2::DataBuf thumbnail = thumb.copy();
#else
        Exiv2::DataBuf thumbnail = d->mExifData.copyThumbnail();
#endif
        image.loadFromData(thumbnail.pData_, thumbnail.size_);
    }
    return image;
}

void JpegContent::setThumbnail(const QImage& thumbnail)
{
    if (d->mExifData.empty()) {
        return;
    }

    QByteArray array;
    QBuffer buffer(&array);
    buffer.open(QIODevice::WriteOnly);
    QImageWriter writer(&buffer, "JPEG");
    if (!writer.write(thumbnail)) {
        qCritical() << "Could not write thumbnail\n";
        return;
    }

#if (EXIV2_TEST_VERSION(0,17,91))
    Exiv2::ExifThumb thumb(d->mExifData);
    thumb.setJpegThumbnail((unsigned char*)array.data(), array.size());
#else
    d->mExifData.setJpegThumbnail((unsigned char*)array.data(), array.size());
#endif
}

bool JpegContent::save(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        d->mErrorString = i18nc("@info", "Could not open file for writing.");
        return false;
    }

    return save(&file);
}

bool JpegContent::save(QIODevice* device)
{
    if (!d->mImage.isNull()) {
        if (!d->updateRawDataFromImage()) {
            return false;
        }
    }

    if (d->mRawData.size() == 0) {
        d->mErrorString = i18nc("@info", "No data to store.");
        return false;
    }

    if (d->mPendingTransformation) {
        applyPendingTransformation();
        d->mPendingTransformation = false;
    }

    std::unique_ptr<Exiv2::Image> image;
    image.reset(Exiv2::ImageFactory::open((unsigned char*)d->mRawData.data(), d->mRawData.size()).release());

    // Store Exif info
    image->setExifData(d->mExifData);
    image->setComment(d->mComment.toUtf8().toStdString());
    image->writeMetadata();

    // Update mRawData
    Exiv2::BasicIo& io = image->io();
    d->mRawData.resize(io.size());
    io.read((unsigned char*)d->mRawData.data(), io.size());

    QDataStream stream(device);
    stream.writeRawData(d->mRawData.data(), d->mRawData.size());

    // Make sure we are up to date
    loadFromData(d->mRawData);
    return true;
}

QString JpegContent::errorString() const
{
    return d->mErrorString;
}

void JpegContent::setImage(const QImage& image)
{
    d->mRawData.clear();
    d->mImage = image;
    d->mSize = image.size();
    d->mExifData["Exif.Photo.PixelXDimension"] = image.width();
    d->mExifData["Exif.Photo.PixelYDimension"] = image.height();
    resetOrientation();

    d->mPendingTransformation = false;
    d->mTransformMatrix = QTransform();
}

} // namespace
