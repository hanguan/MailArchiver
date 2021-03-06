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
// std
#include <future>

// Qt
#include <QKeyEvent>
#include <QCloseEvent>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QFileDialog>
#include <QMessageBox>
#include <QDirIterator>
#include <QStandardPaths>
#include <QSqlQueryModel>
#include <QDesktopServices>
#include <QStandardItemModel>

// Local
#include "msg.h"
#include "MailArchive.h"
#include "MailListModel.h"
#include "ArchiveManager.h"
#include "MailListDelegate.h"
#include "MailArchiverWidget.h"
#include "ui_MailArchiverWidget.h"

MailArchiverWidget::MailArchiverWidget()
    : ctxMenu(new QMenu(this)), ui(new Ui::MailArchiverWidget), delegate(new MailListDelegate()),
      archiveMgr(&ArchiveManager::instance())
{
    ui->setupUi(this);
    ui->mailListView->setItemDelegate(delegate);
    createCtxMenu();
    createConnections();
}

void MailArchiverWidget::createCtxMenu()
{
    // Context menu for mailListView
    ctxMenu->addAction(ui->actionViewSelected);
    ctxMenu->addAction(ui->actionExportSelected);
    ctxMenu->addSeparator();
    ctxMenu->addAction(ui->actionMoveToFolder);
    ctxMenu->addAction(ui->actionRemoveFromArchive);
}

void MailArchiverWidget::createConnections()
{
    // Connecting actions.
    connect(ui->actionOpenArchive, &QAction::triggered, this, &MailArchiverWidget::onOpenArchive);
    connect(ui->actionNewArchive, &QAction::triggered, this, &MailArchiverWidget::onNewArchive);
    connect(ui->actionArchiveEmails, &QAction::triggered, this, &MailArchiverWidget::onArchiveEmails);
    connect(ui->actionArchiveEntireFolder, &QAction::triggered, this,
            &MailArchiverWidget::onArchiveEntireFolder);

    connect(ui->mailListView, &QListView::customContextMenuRequested, this,
            &MailArchiverWidget::onCustomCtxMenuRequested);
    connect(ui->actionViewSelected, &QAction::triggered, this, &MailArchiverWidget::onActionViewSelected);
    connect(ui->actionExportSelected, &QAction::triggered, this, &MailArchiverWidget::onActionExportSelected);
    connect(ui->actionMoveToFolder, &QAction::triggered, this, &MailArchiverWidget::onActionMoveToFolder);
    connect(ui->actionRemoveFromArchive, &QAction::triggered, this,
            &MailArchiverWidget::onActionRemoveFromArchive);
    // Connecting other widget reactors.
    connect(ui->mailListView, &QListView::doubleClicked, this, &MailArchiverWidget::onActionViewSelected);
    connect(ui->archivesListView, &QListView::clicked, this, &MailArchiverWidget::onSelectedOpenedArchive);
    connect(ui->foldersTreeView, &QListView::clicked, this,
            &MailArchiverWidget::onSelectedFolderOnCurrentArchive);
    connect(ui->searchEdit, &QLineEdit::textChanged, this, &MailArchiverWidget::onSearchLineChanged);
    connect(ui->searchButton, &QPushButton::clicked, this, &MailArchiverWidget::onSearchButtonClicked);
    connect(ui->buttonGroup, SIGNAL(buttonPressed(int)), this, SLOT(onButtonGroupPressed(int)));
}

MailArchiverWidget::~MailArchiverWidget()
{
    delete ui;
}

void MailArchiverWidget::updateViews()
{
    ui->mailListView->setModel(archiveMgr->current().emails());
    ui->archivesListView->setModel(archiveMgr->model());
    ui->tabWidget->setTabText(0, archiveMgr->currentName());
}

void MailArchiverWidget::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event);
    archiveMgr->hardCloseAll();
}

// TODO: Subclass QListView for handling this event by itself;
void MailArchiverWidget::keyPressEvent(QKeyEvent* event)
{
    if (ui->mailListView->hasFocus()) {
        switch (event->key()) {
        case Qt::Key_Delete:
            auto selected = ui->mailListView->selectionModel();
            auto model = selected->model();
            if (selected->currentIndex().isValid()) {
                int currentRow = selected->currentIndex().row();
                QString id =
                    model->data(model->index(currentRow, 0), MailListModel::messageIdRole).toString();
                archiveMgr->current().deleteMsg(id);
            }
            break;
        }

    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MailArchiverWidget::onOpenArchive()
{
    QString fn = QFileDialog::getOpenFileName(this, tr("Open Mail archives"),
                                              QStandardPaths::displayName(QStandardPaths::HomeLocation),
                                              tr("Mail Archives (*.mar *.mad *.sqlite)"));
    QString baseName = QFileInfo(fn).baseName();
    if (!fn.isEmpty()) {
        auto f = std::async([this, &fn, &baseName]() {
            QApplication::setOverrideCursor(Qt::WaitCursor);
            archiveMgr->openArchive(fn, baseName);
            updateViews();
            QApplication::restoreOverrideCursor();
        });
        f.get();
    }
}

void MailArchiverWidget::onNewArchive()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save new mail archive"),
                                              QStandardPaths::displayName(QStandardPaths::HomeLocation),
                                              tr("Mail archives (*.mar *.mad *.sqlite)"));
    QString baseName = QFileInfo(fn).baseName();
    if (!fn.isEmpty()) {
        auto f = std::async([this, &fn, &baseName]() {
            QApplication::setOverrideCursor(Qt::WaitCursor);
            archiveMgr->openArchive(fn, baseName);
            updateViews();
            QApplication::restoreOverrideCursor();
        });
        f.get();
    }
}

void MailArchiverWidget::onArchiveEmails()
{
    QDir tmp(QDir::currentPath() + "/mailArchiverTmp");
    QDesktopServices::openUrl(QUrl::fromLocalFile(tmp.path()));
    int res = QMessageBox::information(this, tr("How to archive emails"),
                                       tr("Drag and drop your emails into the opened "
                                          "folder and, "
                                          "when finished, hit this OK button."));

    if (res == QMessageBox::Ok) {
        archiveMgr->current().archiveFolder(tmp.path());
    }
    updateViews();
}

void MailArchiverWidget::onArchiveEntireFolder()
{
    QString folder = QFileDialog::getExistingDirectory();
    auto f = std::async([this, &folder]() {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        archiveMgr->current().archiveFolder(folder);
        updateViews();
        QApplication::restoreOverrideCursor();
    });
    f.get();
}

void MailArchiverWidget::onSelectedOpenedArchive(const QModelIndex& index)
{
    if (index.isValid()) {
        archiveMgr->setCurrent(index.data().toString());
        updateViews();
    }
}

void MailArchiverWidget::onSearchButtonClicked()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    if (ui->full->isChecked()) {
        archiveMgr->current().setSearchFilter(ui->searchEdit->text(),                                              MailArchive::SearchPattern::FullMessage);
    } else if (ui->body->isChecked()) {
        archiveMgr->current().setSearchFilter(ui->searchEdit->text(), MailArchive::SearchPattern::Body);
    } else if (ui->subject->isChecked()) {
        archiveMgr->current().setSearchFilter(ui->searchEdit->text(), MailArchive::SearchPattern::Subject);
    } else if (ui->from->isChecked()) {
        archiveMgr->current().setSearchFilter(ui->searchEdit->text(), MailArchive::SearchPattern::From);
    } else if (ui->to->isChecked()) {
        archiveMgr->current().setSearchFilter(ui->searchEdit->text(), MailArchive::SearchPattern::To);
    } else {
        ui->searchEdit->setText(QString());
        archiveMgr->current().setSearchFilter(ui->searchEdit->text(), MailArchive::SearchPattern::NoSearch);
    }
    QApplication::restoreOverrideCursor();
}

void MailArchiverWidget::onSearchLineChanged(const QString& text)
{
    if(text.isEmpty()){
        for(auto& m : archiveMgr->archivePool()){
            m.second.setSearchFilter(ui->searchEdit->text(), MailArchive::SearchPattern::NoSearch);
        }        
        ui->searchButton->setEnabled(false);
    } else {
        if (!archiveMgr->currentName().isEmpty())
            ui->searchButton->setEnabled( (ui->buttonGroup->checkedId()!=-1) && (text.length() > 3));
    }
    
}

void MailArchiverWidget::onButtonGroupPressed(int id){
    ui->searchButton->setEnabled( (id=-1) && (ui->searchEdit->text().length() > 3));
}

void MailArchiverWidget::onSelectedFolderOnCurrentArchive(const QModelIndex& index)
{
}

// Context menu for list mailListView
void MailArchiverWidget::onCustomCtxMenuRequested(QPoint pos)
{
    auto index = ui->mailListView->indexAt(pos);
    if (index.isValid()) {
        ctxMenu->popup(ui->mailListView->viewport()->mapToGlobal(pos));
    }
}

// TODO: Implement the next 4 functions.
void MailArchiverWidget::onActionViewSelected()
{
    auto selected = ui->mailListView->selectionModel();
    auto index = selected->currentIndex();
    if (index.isValid()) {
        auto subject = index.data(MailListModel::subjectTextRole).toString();
        auto header = index.data(MailListModel::messageIdRole).toString();
        header.prepend(QStringLiteral("Record Locator: {"));
        header.append(QStringLiteral("}\nFrom: "));
        header.append(index.data(MailListModel::senderTextRole).toString());
        header.append(QStringLiteral("\nTo: "));
        header.append(index.data(MailListModel::receiversRole).toString());
        header.append(QStringLiteral("\nOn: "));
        header.append(index.data(MailListModel::whenTextRole).toString());
        auto body = index.data(MailListModel::bodyTextRole).toString();
        ui->bodyView->setText(body);
        ui->tabWidget->setTabText(1, subject);
        ui->headerView->setText(header);

        ui->tabWidget->setCurrentIndex(1);
    }
}

void MailArchiverWidget::onActionExportSelected()
{
    auto selected = ui->mailListView->selectionModel();
    auto model    = selected->model();

    if (selected->currentIndex().isValid()) {
        int currentRow = selected->currentIndex().row();
        QString id     = model->data(model->index(currentRow, 0), MailListModel::messageIdRole).toString();
        auto fileName = QFileDialog::getSaveFileName(
            this, "Save Message File", QStandardPaths::displayName(QStandardPaths::HomeLocation),
            "Outlook Message Files (*.msg)");
        if (!fileName.isEmpty()) {
            QApplication::setOverrideCursor(Qt::WaitCursor);
            archiveMgr->current().saveMsgAsFile(id, fileName);
            QApplication::restoreOverrideCursor();
        }
    }
}

void MailArchiverWidget::onActionMoveToFolder()
{
}

void MailArchiverWidget::onActionRemoveFromArchive()
{
    auto selected = ui->mailListView->selectionModel();
    auto model    = selected->model();

    if (selected->currentIndex().isValid()) {
        int currentRow = selected->currentIndex().row();
        QString id = model->data(model->index(currentRow, 0), MailListModel::messageIdRole).toString();
        archiveMgr->current().deleteMsg(id);
    }
}
