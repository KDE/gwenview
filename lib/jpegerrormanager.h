// vim: set tabstop=4 shiftwidth=4 expandtab
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
#ifndef JPEGERRORMANAGER_H
#define JPEGERRORMANAGER_H

#include <setjmp.h>

extern "C" {
#define XMD_H
#include <jpeglib.h>
#undef const
}

namespace Gwenview
{

/**
 * A simple error manager which overrides jpeg_error_mgr.error_exit to avoid
 * calls to exit(). It uses setjmp, which I don't like, but I don't fill like
 * introducing exceptions to the code base for now.
 *
 * In order to use it, give an instance of it to jpeg_decompress_struct.err,
 * then call setjmp(errorManager.jmp_buffer)
 */
struct JPEGErrorManager : public jpeg_error_mgr
{
    JPEGErrorManager()
    : jpeg_error_mgr()
    {
        jpeg_std_error(this);
        error_exit = errorExitCallBack;
    }

    jmp_buf jmp_buffer;

    static void errorExitCallBack(j_common_ptr cinfo)
    {
        JPEGErrorManager* myerr = static_cast<JPEGErrorManager*>(cinfo->err);
        char buffer[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message)(cinfo, buffer);
        qCWarning(GWENVIEW_LIB_LOG) << buffer ;
        longjmp(myerr->jmp_buffer, 1);
    }
};

} // namespace

#endif /* JPEGERRORMANAGER_H */
