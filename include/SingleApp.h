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

#ifndef SINGLEAPP_H
#define SINGLEAPP_H

#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>

#include <csignal>

class SingleApp
{

  public:
    SingleApp(int& argc, char** argv);
    int exec() { return theApp.exec(); }
    static void sigsegvHandler(int signum)
    {
        if (signum == SIGSEGV) {
            m_sharedMem.detach();
        }
        exit(signum);
    }
    ~SingleApp() = default;

    // Disable copy and move.
    SingleApp(const SingleApp&) = delete;
    SingleApp& operator=(const SingleApp&) = delete;
    SingleApp(SingleApp&&) = delete;
    SingleApp& operator=(SingleApp&&) = delete;

  private:
    static QSharedMemory m_sharedMem;
    QApplication theApp;
};

#endif // SINGLEAPP_H
