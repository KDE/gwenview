/*
 * This file is part of the KDE libraries
 * Copyright (C) 2003 Fredrik Höglund <fredrik@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __KCPUINFO_H
#define __KCPUINFO_H


/**
 * This class provides a means for applications to obtain information at
 * runtime about processor support for certain architecture extensions,
 * such as MMX, SSE, 3DNow and AltiVec.
 *
 * @since 3.2
 */
class KCPUInfo
{
    public:
       /**
         * This enum contains the list of architecture extensions you
         * can query.
         */
        enum Extensions {
            IntelMMX        = 1 << 0,  //!< Intel's MMX instructions.
            IntelSSE        = 1 << 1,  //!< Intel's SSE instructions.
            IntelSSE2       = 1 << 2,  //!< Intel's SSE2 instructions.
            AMD3DNOW        = 1 << 3,  //!< AMD 3DNOW instructions
            AltiVec         = 1 << 4   //!< Motorola AltiVec instructions
        };

        /**
         * Returns true if the processor supports @p extension,
         * and false otherwise.
         *
         * @param   extension the feature to query.
         * @return  If true, the processor supports @p extension.
         * @see     Extensions
         */
        static bool haveExtension( unsigned int extension )
        { return s_features & extension; }

    private:
        static unsigned int s_features;
};

#endif

