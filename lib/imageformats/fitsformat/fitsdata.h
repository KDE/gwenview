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

#pragma once

#include "bayer.h"

typedef enum { FITS_NORMAL, FITS_FOCUS, FITS_GUIDE, FITS_CALIBRATE, FITS_ALIGN } FITSMode;

#ifdef WIN32
// This header must be included before fitsio.h to avoid compiler errors with Visual Studio
#include <windows.h>
#endif

#include <fitsio.h>

#include <QIODevice>
#include <QObject>
#include <QRect>
#include <QRectF>


class FITSData
{
  public:
    FITSData();
    ~FITSData();

    /* Loads FITS image, scales it, and displays it in the GUI */
    bool loadFITS(QIODevice &buffer);
    /* Calculate stats */
    void calculateStats(bool refresh = false);

    // Access functions
    void clearImageBuffers();
    uint8_t *getImageBuffer();

    int getDataType() { return data_type; }

    // Stats
    unsigned int getSize() { return stats.samples_per_channel; }
    uint16_t getWidth() { return stats.width; }
    uint16_t getHeight() { return stats.height; }

    // Statistics
    int getNumOfChannels() { return channels; }
    void getMinMax(double *min, double *max, uint8_t channel = 0)
    {
        *min = stats.min[channel];
        *max = stats.max[channel];
    }

    // Debayer
    bool debayer();
    bool debayer_8bit();
    bool debayer_16bit();

    // FITS Record
    int getFITSRecord(QString &recordList, int &nkeys);

    // Create autostretch image from FITS File
    static QImage FITSToImage(QIODevice &buffer);

    QString getLastError() const;

  private:
    int calculateMinMax(bool refresh = false);
    bool checkDebayer();

    // Templated functions
    template <typename T>
    bool debayer();

    template <typename T>
    void calculateMinMax();
    /* Calculate running average & standard deviation using Welford’s method for computing variance */
    template <typename T>
    void runningAverageStdDev();

    template <typename T>
    void convertToQImage(double dataMin, double dataMax, double scale, double zero, QImage &image);

    /// Pointer to CFITSIO FITS file struct
    fitsfile *fptr { nullptr };

    /// FITS image data type (TBYTE, TUSHORT, TINT, TFLOAT, TLONG, TDOUBLE)
    int data_type { 0 };
    /// Number of channels
    int channels { 1 };
    /// Generic data image buffer
    uint8_t *imageBuffer { nullptr };

    /// Our very own file name
    QString filename;
    /// FITS Mode (Normal, WCS, Guide, Focus..etc)
    FITSMode mode;

    uint8_t *bayerBuffer { nullptr };
    /// Bayer parameters
    BayerParams debayerParams;

    /// Stats struct to hold statisical data about the FITS data
    struct
    {
        double min[3], max[3];
        double mean[3];
        double stddev[3];
        double median[3];
        double SNR { 0 };
        int bitpix { 8 };
        int bytesPerPixel { 1 };
        int ndim { 2 };
        uint32_t samples_per_channel { 0 };
        uint16_t width { 0 };
        uint16_t height { 0 };
    } stats;

    QString lastError;
};
