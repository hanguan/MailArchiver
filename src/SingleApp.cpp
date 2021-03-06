/**************************************************************************
* Mail Archiver - A solution to store and manage offline e-mail files.    *
* Copyright (C) 2015-2016 Carlos Nihelton <carlosnsoliveira@gmail.com>    *
*                                                                         *
* This is a free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Library General Public             *
* License as published by the Free Software Foundation; either            *
* version 2 of the License, or (at your option) any later version.        *
*                                                                         *
* This software  is distributed in the hope that it will be useful,       *
* but WITHOUT ANY WARRANTY; without even the implied warranty of          *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
* GNU Library General Public License for more details.                    *
*                                                                         *
* You should have received a copy of the GNU Library General Public       *
* License along with this library; see the file COPYING.LIB. If not,      *
* write to the Free Software Foundation, Inc., 59 Temple Place,           *
* Suite 330, Boston, MA  02111-1307, USA                                  *
*                                                                         *
**************************************************************************/

#include "SingleApp.h"

QSharedMemory SingleApp::m_sharedMem{"org.olivec.MailArchiverApp"};

SingleApp::SingleApp(int& argc, char** argv) : theApp(argc, argv)
{
    std::signal(SIGSEGV, SingleApp::sigsegvHandler);

    if (!m_sharedMem.create(1)) {
        QMessageBox::critical(nullptr, QObject::tr("Mail Archive: Cannot Start!"),
                              QObject::tr("An instance of this "
                                          "application is running!"));
        exit(0);
    }
}
