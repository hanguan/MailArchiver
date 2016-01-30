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
* ****************************************************************************/

//std
#include <fstream>
#include <sstream>
//Qt
#include <QString>
#include <QUrl>
#include <QDirIterator>
#include <QSqlQuery>
#include <QFile>
#include <QBuffer>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QDebug>
#include <QSqlError>
//local
#include "MailListModel.h"
#include "MailArchive.h"
#include "KCompressionDevice"

MailArchive::MailArchive(const QString& filename)
{
    openFile(filename);
    m_Folders = new QSqlQueryModel;
    m_Folders->setQuery("SELECT * FROM MailFolders", db);
    m_Tags = new QSqlQueryModel;
    m_Tags->setQuery("SELECT * FROM MailTags", db);
    m_Emails = new MailListModel;
    m_Emails->setQuery("SELECT * FROM MailArchive", db);
}

void MailArchive::openFile(const QString& filename)
{
    QUrl name(filename);
    baseFileName=name.fileName();
    m_Path = name.path();

    db = QSqlDatabase::addDatabase("QSQLITE", baseFileName);
    db.setDatabaseName(filename);

    if(db.open()) {
        QSqlQuery q(db);

        q.exec("CREATE TABLE IF NOT EXISTS MailArchive ("
               "MESSAGEID VARCHAR(32) PRIMARY KEY NOT NULL, "
               "FROM_NAME TEXT, FROM_ADDR TEXT, "
               "TO_NAME TEXT, TO_ADDR TEXT, "
               "CC TEXT, BCC TEXT, SUBJECT TEXT, "
               "CWHEN DATE, CONTENT TEXT, "
               "COMPRESSED BLOB, HASATTACH BOOL)");

        q.exec("CREATE TABLE IF NOT EXISTS MailFolders (FID INTEGER PRIMARY KEY NOT NULL, FNAME TEXT NOT NULL)");
        q.exec("CREATE TABLE IF NOT EXISTS MailTags (TID INTEGER PRIMARY KEY NO NULL, TNAME TEXT NOT NULL)");
        q.exec("CREATE TABLE IF NOT EXISTS FolderRels (FRELID INTEGER PRIMARY KEY NOT NULL, FID INTEGER NOT NULL, MID VARCHAR(32) NOT NULL)");
        q.exec("CREATE TABLE IF NOT EXISTS TagRels (TRELID INTEGER PRIMARY KEY NOT NULL, TID INTEGER NOT NULL, MID VARCHAR(32) NOT NULL)");
    }

}

MailArchive::~MailArchive() {
    delete m_Emails;
    delete m_Folders;
    delete m_Tags;

    db.close();
}

void MailArchive::setActiveFolder(const QString& af)
{
    m_ActiveFolder = af;
}

void MailArchive::setActiveTag(const QString& at)
{
    m_ActiveTag = at;
}

void MailArchive::archiveFolder(const QString& folder)
{
    QDirIterator it(folder, QStringList()<<"*.msg", QDir::Files);
    while(it.hasNext())
    {
        QString mes = it.next();
        qDebug() << mes;
        Core::Msg msg(mes.toStdString());
        archiveMsg(msg);
    }
}

void MailArchive::archiveMsg(Core::Msg& msgFile)
{
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(FROM_NAME) FROM MailArchive WHERE MESSAGEID=?");
    q.addBindValue(msgFile.hash().c_str());
    q.exec();

    if(q.value(0).toInt()==0)
    {

        q.prepare("INSERT INTO MailArchive (MESSAGEID, FROM_NAME, FROM_ADDR, TO_NAME, TO_ADDR, CC, BCC, SUBJECT, CWHEN, CONTENT, COMPRESSED, HASATTACH) VALUES (?,?,?,?,?,?,?,?,?,?,?,?)");
        q.addBindValue(msgFile.hash().c_str());
        q.addBindValue(msgFile.senderName().c_str());
        q.addBindValue(msgFile.senderAddress().c_str());
        q.addBindValue(msgFile.receiversNames().c_str());
        q.addBindValue(msgFile.receiversAddresses().c_str());
        q.addBindValue(msgFile.CCs().c_str());
        q.addBindValue(msgFile.Bccs().c_str());
        q.addBindValue(msgFile.subject().c_str());
        q.addBindValue(msgFile.date().c_str());
        q.addBindValue(msgFile.body().c_str());
        QByteArray array(msgFile.selfCompress().data(), msgFile.selfCompress().size());
        qDebug() << array.size();
        q.addBindValue(array, QSql::In | QSql::Binary);
        q.addBindValue(msgFile.hasAttachments()?'1':'0');

        q.exec();
        qDebug() << q.lastQuery();
        qDebug() << q.lastError().text();
    }
}

Core::Msg MailArchive::retrieveMsg(const QString& messageId)
{
    saveMsgAsFile(messageId, messageId);
    Core::Msg msg(messageId.toStdString());
    return msg;
}

void MailArchive::saveMsgAsFile(const QString& messageId, const QString& fileName)
{
    QSqlQuery q(db);
    q.prepare("SELECT COMPRESSED FROM MailArchive WHERE MESSAGEID=?");
    q.addBindValue(messageId);
    q.exec();
    if(q.next()) {
        QByteArray array(q.value(0).toByteArray());
        std::string input(array.data(), array.size());
        std::ofstream outFile(fileName.toUtf8().data(), std::ios::binary);
        std::string output(Utils::decompress_string(input));
        outFile.write(output.data(), output.size());
    }

}

void MailArchive::refresh()
{
    m_Emails->setQuery(m_Emails->query());
    m_Folders->setQuery(m_Folders->query());
}