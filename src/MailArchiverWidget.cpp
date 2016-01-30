/****************************************************************************
* Mail Archiver - A solution to store and manage offline e-mail files.      *
* Copyright (C) 2015-2016 Carlos Nihelton <carlosnsoliveira@gmail.com>      *
*                                                                           *
*   This is a free software; you can redistribute it and/or                 *
*   modify it under the terms of the GNU Library General Public             *
*   License as published by the Free Software Foundation; either            *
*   version 2 of the License, or (at your option) any later version.        *
*                                                                           *
*   This software  is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
*   GNU Library General Public License for more details.                    *
*                                                                           *
*   You should have received a copy of the GNU Library General Public       *
*   License along with this library; see the file COPYING.LIB. If not,      *
*   write to the Free Software Foundation, Inc., 59 Temple Place,           *
*   Suite 330, Boston, MA  02111-1307, USA                                  *
*                                                                           *
****************************************************************************/
//Qt
#include <QDebug>
#include <QSqlRecord>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStandardItemModel>
#include <QSqlQueryModel>
#include <QDirIterator>

//std
#include <future>

//Local
#include "msg.h"
#include "MailListModel.h"
#include "MailArchive.h"
#include "MailListDelegate.h"
#include "MailArchiverWidget.h"
#include "ui_MailArchiverWidget.h"

MailArchiverWidget::MailArchiverWidget() : ui(new Ui::MailArchiverWidget), delegate(new MailListDelegate()), model(nullptr)
{
    ui->setupUi(this);
    ui->more->setChecked(false);
    ui->mailListView->setItemDelegate(delegate);
    //ui->mailListView->setModel(model);
    ui->mailListView->show();
    
    //Connecting actions.
    connect(ui->actionOpenArchive, &QAction::triggered, this, &MailArchiverWidget::onOpenArchive);
    connect(ui->actionNewArchive, &QAction::triggered, this, &MailArchiverWidget::onNewArchive);
    connect(ui->actionArchiveEmails, &QAction::triggered, this, &MailArchiverWidget::onArchiveEmails);
    connect(ui->actionArchiveEntireFolder, &QAction::triggered, this, &MailArchiverWidget::onArchiveEntireFolder);
    
    //Connecting other widget reactors.
    connect(ui->mailListView, &QListView::doubleClicked, this, &MailArchiverWidget::onListViewDoubleClicked);
    
    emit(ui->more->toggled(false));
}

MailArchiverWidget::~MailArchiverWidget()
{
    delete ui;
}


// Defining the slots.

void MailArchiverWidget::onOpenArchive()
{
    QString fn = QFileDialog::getOpenFileName(this, tr("Open Mail archives"), QStandardPaths::displayName(QStandardPaths::HomeLocation),
        tr("Mail Archives (*.mar, *.mad, *.sqlite)"));
    if(!fn.isEmpty()){
        auto f = std::async([this, &fn](){
            QApplication::setOverrideCursor(Qt::WaitCursor);
            archive=new MailArchive(fn);
            if(model) delete model;
            model = archive->emails();
            ui->mailListView->setModel(model);
            QApplication::restoreOverrideCursor();
        });
        f.get();
    }
        
}

void MailArchiverWidget::onNewArchive()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save new mail archive"), QStandardPaths::displayName(QStandardPaths::HomeLocation), tr("Mail archives (*.mar, *.mad, *.sqlite)"));
    if(!fn.isEmpty()){
        auto f = std::async([this, &fn](){
            QApplication::setOverrideCursor(Qt::WaitCursor);
            archive=new MailArchive(fn);
            if(model) delete model;
            model = archive->emails();
            ui->mailListView->setModel(model);
            QApplication::restoreOverrideCursor();
        });
        f.get();
    }
}

void MailArchiverWidget::onArchiveEmails()
{
    QDir tmp(QDir::currentPath() + "/mailArchiverTmp");
    QDesktopServices::openUrl(QUrl::fromLocalFile(tmp.path()));
    int res = QMessageBox::information(this, tr("How to archive emails"), tr("Drag and drop your emails into the opened folder and, when finished, hit this OK button."));
    
    if(res == QMessageBox::Ok)
    {
        archive->archiveFolder(tmp.path());
    }
    archive->refresh();
}

void MailArchiverWidget::onArchiveEntireFolder()
{
    QString folder = QFileDialog::getExistingDirectory();
    auto f = std::async([this, &folder](){
        QApplication::setOverrideCursor(Qt::WaitCursor);
        archive->archiveFolder(folder);
        QApplication::restoreOverrideCursor();
    });
    f.get();
}

void MailArchiverWidget::onListViewDoubleClicked(const QModelIndex& index)
{
    const QAbstractItemModel* model(index.model());
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString id = model->data(model->index(index.row(),0), MailListModel::messageIdRole).toString();
    archive->saveMsgAsFile(id,id+".msg");
    QApplication::restoreOverrideCursor();
}
