/***************************************************************************
Gwenview: an image viewer

                          fitsimage.cpp  -  FITS Image
                             -------------------
    begin                : Tue Feb 24 2004
    copyright            : (C) 2004 by Jasem Mutlaq
    copyright            : (C) 2017 by Csaba Kertesz
    email                : mutlaqja@ikarustech.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Some code fragments were adapted from Peter Kirchgessner's FITS plugin*
 *   See http://members.aol.com/pkirchg for more details.                  *
 ***************************************************************************/

#include "fitsdata.h"

// Qt
#include <QImage>

// STL
#include <cmath>

FITSData::FITSData()
{
    mode = FITS_NORMAL;
    debayerParams.method = DC1394_BAYER_METHOD_NEAREST;
    debayerParams.filter = DC1394_COLOR_FILTER_RGGB;
    debayerParams.offsetX = debayerParams.offsetY = 0;
}

FITSData::~FITSData()
{
    int status = 0;

    clearImageBuffers();

    if (fptr) {
        fits_close_file(fptr, &status);
    }
}

bool FITSData::loadFITS(QIODevice &buffer)
{
    int status = 0, anynull = 0;
    long naxes[3];
    char error_status[512];
    QString errMessage;
    qint64 oldPos = buffer.pos();
    QByteArray imageData;
    char *imageDataBuf = nullptr;
    size_t imageDataSize = 0;

    buffer.seek(0);
    imageData = buffer.readAll();
    imageDataBuf = imageData.data();
    imageDataSize = (size_t)imageData.size();

    if (fptr) {
        fits_close_file(fptr, &status);
    }

    if (fits_open_memfile(&fptr, "", READONLY, reinterpret_cast<void **>(&imageDataBuf), &imageDataSize, 3000, nullptr, &status)) {
        fits_report_error(stderr, status);
        fits_get_errstatus(status, error_status);
        errMessage = QString("Could not open file %1. Error %2").arg(filename, QString::fromUtf8(error_status));
        buffer.seek(oldPos);
        return false;
    }

    if (fits_get_img_param(fptr, 3, &(stats.bitpix), &(stats.ndim), naxes, &status)) {
        fits_report_error(stderr, status);
        fits_get_errstatus(status, error_status);
        errMessage = QString("FITS file open error (fits_get_img_param): %1").arg(QString::fromUtf8(error_status));
        buffer.seek(oldPos);
        return false;
    }

    if (stats.ndim < 2) {
        errMessage = "1D FITS images are not supported.";
        buffer.seek(oldPos);
        return false;
    }

    switch (stats.bitpix) {
    case BYTE_IMG:
        data_type = TBYTE;
        stats.bytesPerPixel = sizeof(uint8_t);
        break;
    case SHORT_IMG:
        // Read SHORT image as USHORT
        data_type = TUSHORT;
        stats.bytesPerPixel = sizeof(int16_t);
        break;
    case USHORT_IMG:
        data_type = TUSHORT;
        stats.bytesPerPixel = sizeof(uint16_t);
        break;
    case LONG_IMG:
        // Read LONG image as ULONG
        data_type = TULONG;
        stats.bytesPerPixel = sizeof(int32_t);
        break;
    case ULONG_IMG:
        data_type = TULONG;
        stats.bytesPerPixel = sizeof(uint32_t);
        break;
    case FLOAT_IMG:
        data_type = TFLOAT;
        stats.bytesPerPixel = sizeof(float);
        break;
    case LONGLONG_IMG:
        data_type = TLONGLONG;
        stats.bytesPerPixel = sizeof(int64_t);
        break;
    case DOUBLE_IMG:
        data_type = TDOUBLE;
        stats.bytesPerPixel = sizeof(double);
        break;
    default:
        errMessage = QString("Bit depth %1 is not supported.").arg(stats.bitpix);
        return false;
        break;
    }

    if (stats.ndim < 3) {
        naxes[2] = 1;
    }

    if (naxes[0] == 0 || naxes[1] == 0) {
        errMessage = QString("Image has invalid dimensions %1x%2").arg(naxes[0], naxes[1]);
        buffer.seek(oldPos);
        return false;
    }

    stats.width = naxes[0];
    stats.height = naxes[1];
    stats.samples_per_channel = stats.width * stats.height;

    clearImageBuffers();

    channels = naxes[2];

    if (channels != 1 && channels != 3) {
        // code in both calculateMinMax and convertToQImage assume that images have either
        // 1 channel or 3. File in bug 482615 has two, so bail out better than crashing
        errMessage = QString("Images with %1 channels are currently not supported.").arg(channels);
        buffer.seek(oldPos);
        return false;
    }

    imageBuffer = new uint8_t[stats.samples_per_channel * channels * stats.bytesPerPixel];

    long nelements = stats.samples_per_channel * channels;

    if (fits_read_img(fptr, data_type, 1, nelements, nullptr, imageBuffer, &anynull, &status)) {
        char errmsg[512];
        fits_get_errstatus(status, errmsg);
        errMessage = QString("Error reading image: %1").arg(errmsg);
        fits_report_error(stderr, status);
        buffer.seek(oldPos);
        return false;
    }

    calculateStats();

    if (checkDebayer()) {
        bayerBuffer = imageBuffer;
        debayer();
    }
    return true;
}

void FITSData::clearImageBuffers()
{
    delete[] imageBuffer;
    imageBuffer = nullptr;
    bayerBuffer = nullptr;
}

void FITSData::calculateStats(bool refresh)
{
    // Calculate min max
    calculateMinMax(refresh);

    // Get standard deviation and mean in one run
    switch (data_type) {
    case TBYTE:
        runningAverageStdDev<uint8_t>();
        break;

    case TSHORT:
        runningAverageStdDev<int16_t>();
        break;

    case TUSHORT:
        runningAverageStdDev<uint16_t>();
        break;

    case TLONG:
        runningAverageStdDev<int32_t>();
        break;

    case TULONG:
        runningAverageStdDev<uint32_t>();
        break;

    case TFLOAT:
        runningAverageStdDev<float>();
        break;

    case TLONGLONG:
        runningAverageStdDev<int64_t>();
        break;

    case TDOUBLE:
        runningAverageStdDev<double>();
        break;

    default:
        return;
    }

    stats.SNR = stats.mean[0] / stats.stddev[0];
}

int FITSData::calculateMinMax(bool refresh)
{
    int status, nfound = 0;

    status = 0;

    if (fptr && refresh == false) {
        if (fits_read_key_dbl(fptr, "DATAMIN", &(stats.min[0]), nullptr, &status) == 0) {
            nfound++;
        }

        if (fits_read_key_dbl(fptr, "DATAMAX", &(stats.max[0]), nullptr, &status) == 0) {
            nfound++;
        }

        // If we found both keywords, no need to calculate them, unless they are both zeros
        if (nfound == 2 && !(stats.min[0] == 0 && stats.max[0] == 0)) {
            return 0;
        }
    }

    stats.min[0] = 1.0E30;
    stats.max[0] = -1.0E30;

    stats.min[1] = 1.0E30;
    stats.max[1] = -1.0E30;

    stats.min[2] = 1.0E30;
    stats.max[2] = -1.0E30;

    switch (data_type) {
    case TBYTE:
        calculateMinMax<uint8_t>();
        break;

    case TSHORT:
        calculateMinMax<int16_t>();
        break;

    case TUSHORT:
        calculateMinMax<uint16_t>();
        break;

    case TLONG:
        calculateMinMax<int32_t>();
        break;

    case TULONG:
        calculateMinMax<uint32_t>();
        break;

    case TFLOAT:
        calculateMinMax<float>();
        break;

    case TLONGLONG:
        calculateMinMax<int64_t>();
        break;

    case TDOUBLE:
        calculateMinMax<double>();
        break;

    default:
        break;
    }

    // qCDebug(GWENVIEW_LIB_LOG) << "DATAMIN: " << stats.min << " - DATAMAX: " << stats.max;
    return 0;
}

template<typename T>
void FITSData::calculateMinMax()
{
    T *buffer = reinterpret_cast<T *>(imageBuffer);

    if (channels == 1) {
        for (unsigned int i = 0; i < stats.samples_per_channel; i++) {
            if (buffer[i] < stats.min[0]) {
                stats.min[0] = buffer[i];
            } else if (buffer[i] > stats.max[0]) {
                stats.max[0] = buffer[i];
            }
        }
    } else {
        int g_offset = stats.samples_per_channel;
        int b_offset = stats.samples_per_channel * 2;

        for (unsigned int i = 0; i < stats.samples_per_channel; i++) {
            if (buffer[i] < stats.min[0]) {
                stats.min[0] = buffer[i];
            } else if (buffer[i] > stats.max[0]) {
                stats.max[0] = buffer[i];
            }

            if (buffer[i + g_offset] < stats.min[1]) {
                stats.min[1] = buffer[i + g_offset];
            } else if (buffer[i + g_offset] > stats.max[1]) {
                stats.max[1] = buffer[i + g_offset];
            }

            if (buffer[i + b_offset] < stats.min[2]) {
                stats.min[2] = buffer[i + b_offset];
            } else if (buffer[i + b_offset] > stats.max[2]) {
                stats.max[2] = buffer[i + b_offset];
            }
        }
    }
}

template<typename T>
void FITSData::runningAverageStdDev()
{
    T *buffer = reinterpret_cast<T *>(imageBuffer);
    int m_n = 2;
    double m_oldM = 0, m_newM = 0, m_oldS = 0, m_newS = 0;

    m_oldM = m_newM = buffer[0];
    for (unsigned int i = 1; i < stats.samples_per_channel; i++) {
        m_newM = m_oldM + (buffer[i] - m_oldM) / m_n;
        m_newS = m_oldS + (buffer[i] - m_oldM) * (buffer[i] - m_newM);

        m_oldM = m_newM;
        m_oldS = m_newS;
        m_n++;
    }

    double variance = (m_n == 2 ? 0 : m_newS / (m_n - 2));

    stats.mean[0] = m_newM;
    stats.stddev[0] = sqrt(variance);
}

int FITSData::getFITSRecord(QString &recordList, int &nkeys)
{
    char *header = nullptr;
    int status = 0;

    if (fits_hdr2str(fptr, 0, nullptr, 0, &header, &nkeys, &status)) {
        fits_report_error(stderr, status);
        free(header);
        return -1;
    }

    recordList = QString(header);

    free(header);

    return 0;
}

uint8_t *FITSData::getImageBuffer()
{
    return imageBuffer;
}

bool FITSData::checkDebayer()
{
    int status = 0;
    char bayerPattern[64];

    // Let's search for BAYERPAT keyword, if it's not found we return as there is no bayer pattern in this image
    if (fits_read_keyword(fptr, "BAYERPAT", bayerPattern, nullptr, &status)) {
        return false;
    }

    if (stats.bitpix != 16 && stats.bitpix != 8) {
        return false;
    }
    QString pattern(bayerPattern);
    pattern = pattern.remove('\'').trimmed();

    if (pattern == "RGGB") {
        debayerParams.filter = DC1394_COLOR_FILTER_RGGB;
    } else if (pattern == "GBRG") {
        debayerParams.filter = DC1394_COLOR_FILTER_GBRG;
    } else if (pattern == "GRBG") {
        debayerParams.filter = DC1394_COLOR_FILTER_GRBG;
    } else if (pattern == "BGGR") {
        debayerParams.filter = DC1394_COLOR_FILTER_BGGR;
        // We return unless we find a valid pattern
    } else {
        return false;
    }

    fits_read_key(fptr, TINT, "XBAYROFF", &debayerParams.offsetX, nullptr, &status);
    fits_read_key(fptr, TINT, "YBAYROFF", &debayerParams.offsetY, nullptr, &status);

    return true;
}

bool FITSData::debayer()
{
    if (!bayerBuffer) {
        int anynull = 0, status = 0;

        bayerBuffer = imageBuffer;

        if (fits_read_img(fptr, data_type, 1, stats.samples_per_channel, nullptr, bayerBuffer, &anynull, &status)) {
            char errmsg[512];
            fits_get_errstatus(status, errmsg);
            return false;
        }
    }

    switch (data_type) {
    case TBYTE:
        return debayer_8bit();

    case TUSHORT:
        return debayer_16bit();

    default:
        return false;
    }

    return false;
}

bool FITSData::debayer_8bit()
{
    dc1394error_t error_code;

    int rgb_size = stats.samples_per_channel * 3 * stats.bytesPerPixel;
    uint8_t *destinationBuffer = new uint8_t[rgb_size];

    int ds1394_height = stats.height;
    uint8_t *dc1394_source = bayerBuffer;

    if (debayerParams.offsetY == 1) {
        dc1394_source += stats.width;
        ds1394_height--;
    }

    if (debayerParams.offsetX == 1) {
        dc1394_source++;
    }

    error_code = dc1394_bayer_decoding_8bit(dc1394_source, destinationBuffer, stats.width, ds1394_height, debayerParams.filter, debayerParams.method);

    if (error_code != DC1394_SUCCESS) {
        channels = 1;
        delete[] destinationBuffer;
        return false;
    }

    if (channels == 1) {
        delete[] imageBuffer;
        imageBuffer = new uint8_t[rgb_size];
    }

    // Data in R1G1B1, we need to copy them into 3 layers for FITS

    uint8_t *rBuff = imageBuffer;
    uint8_t *gBuff = imageBuffer + (stats.width * stats.height);
    uint8_t *bBuff = imageBuffer + (stats.width * stats.height * 2);

    int imax = stats.samples_per_channel * 3 - 3;
    for (int i = 0; i <= imax; i += 3) {
        *rBuff++ = destinationBuffer[i];
        *gBuff++ = destinationBuffer[i + 1];
        *bBuff++ = destinationBuffer[i + 2];
    }

    channels = 3;
    delete[] destinationBuffer;
    bayerBuffer = nullptr;
    return true;
}

bool FITSData::debayer_16bit()
{
    dc1394error_t error_code;

    int rgb_size = stats.samples_per_channel * 3 * stats.bytesPerPixel;
    uint8_t *destinationBuffer = new uint8_t[rgb_size];

    uint16_t *buffer = reinterpret_cast<uint16_t *>(bayerBuffer);
    uint16_t *dstBuffer = reinterpret_cast<uint16_t *>(destinationBuffer);

    int ds1394_height = stats.height;
    uint16_t *dc1394_source = buffer;

    if (debayerParams.offsetY == 1) {
        dc1394_source += stats.width;
        ds1394_height--;
    }

    if (debayerParams.offsetX == 1) {
        dc1394_source++;
    }

    error_code = dc1394_bayer_decoding_16bit(dc1394_source, dstBuffer, stats.width, ds1394_height, debayerParams.filter, debayerParams.method, 16);

    if (error_code != DC1394_SUCCESS) {
        channels = 1;
        delete[] destinationBuffer;
        return false;
    }

    if (channels == 1) {
        delete[] imageBuffer;
        imageBuffer = new uint8_t[rgb_size];

        if (!imageBuffer) {
            delete[] destinationBuffer;
            return false;
        }
    }

    buffer = reinterpret_cast<uint16_t *>(imageBuffer);

    // Data in R1G1B1, we need to copy them into 3 layers for FITS

    uint16_t *rBuff = buffer;
    uint16_t *gBuff = buffer + (stats.width * stats.height);
    uint16_t *bBuff = buffer + (stats.width * stats.height * 2);

    int imax = stats.samples_per_channel * 3 - 3;
    for (int i = 0; i <= imax; i += 3) {
        *rBuff++ = dstBuffer[i];
        *gBuff++ = dstBuffer[i + 1];
        *bBuff++ = dstBuffer[i + 2];
    }

    channels = 3;
    delete[] destinationBuffer;
    bayerBuffer = nullptr;
    return true;
}

QString FITSData::getLastError() const
{
    return lastError;
}

template<typename T>
void FITSData::convertToQImage(double dataMin, double dataMax, double scale, double zero, QImage &image)
{
    T *buffer = (T *)getImageBuffer();
    const T limit = std::numeric_limits<T>::max();
    T bMin = dataMin < 0 ? 0 : dataMin;
    T bMax = dataMax > limit ? limit : dataMax;
    uint16_t w = getWidth();
    uint16_t h = getHeight();
    uint32_t size = getSize();
    double val;

    if (getNumOfChannels() == 1) {
        /* Fill in pixel values using indexed map, linear scale */
        for (int j = 0; j < h; j++) {
            unsigned char *scanLine = image.scanLine(j);

            for (int i = 0; i < w; i++) {
                val = qBound(bMin, buffer[j * w + i], bMax);
                val = val * scale + zero;
                scanLine[i] = qBound<unsigned char>(0, (unsigned char)val, 255);
            }
        }
    } else {
        double rval = 0, gval = 0, bval = 0;
        QRgb value;
        /* Fill in pixel values using indexed map, linear scale */
        for (int j = 0; j < h; j++) {
            QRgb *scanLine = reinterpret_cast<QRgb *>((image.scanLine(j)));

            for (int i = 0; i < w; i++) {
                rval = qBound(bMin, buffer[j * w + i], bMax);
                gval = qBound(bMin, buffer[j * w + i + size], bMax);
                bval = qBound(bMin, buffer[j * w + i + size * 2], bMax);

                value = qRgb(rval * scale + zero, gval * scale + zero, bval * scale + zero);

                scanLine[i] = value;
            }
        }
    }
}

QImage FITSData::FITSToImage(QIODevice &buffer)
{
    QImage fitsImage;
    double min, max;
    FITSData data;

    bool rc = data.loadFITS(buffer);

    if (rc == false) {
        return fitsImage;
    }

    data.getMinMax(&min, &max);

    if (min == max) {
        fitsImage.fill(Qt::white);
        return fitsImage;
    }

    if (data.getNumOfChannels() == 1) {
        fitsImage = QImage(data.getWidth(), data.getHeight(), QImage::Format_Indexed8);

        fitsImage.setColorCount(256);
        for (int i = 0; i < 256; i++) {
            fitsImage.setColor(i, qRgb(i, i, i));
        }
    } else {
        fitsImage = QImage(data.getWidth(), data.getHeight(), QImage::Format_RGB32);
    }

    double dataMin = data.stats.mean[0] - data.stats.stddev[0];
    double dataMax = data.stats.mean[0] + data.stats.stddev[0] * 3;

    double bscale = 255. / (dataMax - dataMin);
    double bzero = (-dataMin) * (255. / (dataMax - dataMin));

    // Long way to do this since we do not want to use templated functions here
    switch (data.getDataType()) {
    case TBYTE:
        data.convertToQImage<uint8_t>(dataMin, dataMax, bscale, bzero, fitsImage);
        break;

    case TSHORT:
        data.convertToQImage<int16_t>(dataMin, dataMax, bscale, bzero, fitsImage);
        break;

    case TUSHORT:
        data.convertToQImage<uint16_t>(dataMin, dataMax, bscale, bzero, fitsImage);
        break;

    case TLONG:
        data.convertToQImage<int32_t>(dataMin, dataMax, bscale, bzero, fitsImage);
        break;

    case TULONG:
        data.convertToQImage<uint32_t>(dataMin, dataMax, bscale, bzero, fitsImage);
        break;

    case TFLOAT:
        data.convertToQImage<float>(dataMin, dataMax, bscale, bzero, fitsImage);
        break;

    case TLONGLONG:
        data.convertToQImage<int64_t>(dataMin, dataMax, bscale, bzero, fitsImage);
        break;

    case TDOUBLE:
        data.convertToQImage<double>(dataMin, dataMax, bscale, bzero, fitsImage);
        break;

    default:
        break;
    }

    return fitsImage;
}
