//////////////////////////////////////////////////////////////////////
//
// This file is part of BeeBEEP.
//
// BeeBEEP is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// BeeBEEP is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with BeeBEEP.  If not, see <http://www.gnu.org/licenses/>.
//
// Author: Marco Mastroddi <marco.mastroddi(AT)gmail.com>
//
// $Id$
//
//////////////////////////////////////////////////////////////////////

#include "BeeApplication.h"
#include "BeeUtils.h"
#include "BuildFileList.h"
#include "BuildFileShareList.h"
#include "ChatManager.h"
#include "Connection.h"
#include "Core.h"
#include "FileShare.h"
#include "FileTransferPeer.h"
#include "IconManager.h"
#include "Protocol.h"
#include "Settings.h"
#include "UserManager.h"


bool Core::startFileTransferServer()
{
  if( mp_fileTransfer->isActive() )
  {
    qDebug() << "Starting File Transfer server but it is already started";
    return true;
  }

  if( !mp_fileTransfer->startListener() )
  {
    QString icon_html = IconManager::instance().toHtml( "upload.png", "*F*" );
    dispatchSystemMessage( ID_DEFAULT_CHAT, ID_LOCAL_USER,
                           tr( "%1 Unable to start file transfer server: bind address/port failed." ).arg( icon_html ),
                           DispatchToChat, ChatMessage::FileTransfer );
    return false;
  }

  return true;
}

void Core::stopFileTransferServer()
{
  mp_fileTransfer->stopListener();
  Protocol::instance().createFileShareListMessage( FileShare::instance().local(), -1 );
}

bool Core::downloadFile( VNumber user_id, const FileInfo& fi, bool show_message )
{
  if( !mp_fileTransfer->isActive() )
  {
    if( !startFileTransferServer() )
      return false;
  }

  User u = UserManager::instance().findUser( user_id );
  if( !u.isValid() )
  {
    qWarning() << "Unable to find user" << user_id << "to download file" << fi.name();
    return false;
  }

  QString icon_html;
  Chat chat_to_show_message = ChatManager::instance().findChatByPrivateId( fi.chatPrivateId(), false, u.id() );

  if( !isUserConnected( u.id() ) )
  {
    if( show_message )
    {
      icon_html = IconManager::instance().toHtml( "red-ball.png", "*F*" );
      dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, u.id(), tr( "%1 Unable to download %2 from %3: user is offline." ).arg( icon_html, fi.name(), u.name() ),
                             chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
    }
    qWarning() << "Unable to download" << fi.name() << "from" << u.path() << "because user is offline";
    return false;
  }

  QFileInfo file_info( fi.path() );
  QDir folder_path = file_info.absoluteDir();
  if( !folder_path.exists() )
  {
    if( !folder_path.mkpath( "." ) )
    {
      if( show_message )
      {
        icon_html = IconManager::instance().toHtml( "red-ball.png", "*F*" );
        dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, u.id(), tr( "%1 Unable to download %2 from %3: folder %4 cannot be created." )
                               .arg( icon_html, fi.name(), u.name(), folder_path.absolutePath() ),
                               chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
      }
      qWarning() << "Unable to download" << qPrintable( fi.name() ) << "because folder" << qPrintable( folder_path.absolutePath() ) << "can not be created";
      return false;
    }
  }

  if( show_message )
  {
    icon_html = IconManager::instance().toHtml( "download.png", "*F*" );
    dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, u.id(), tr( "%1 Downloading %2 from %3." )
                         .arg( icon_html, fi.name(), u.name() ),
                         chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
  }

  qDebug() << "Downloading file" << qPrintable( fi.path() ) << "from user" << qPrintable( u.path() );
  mp_fileTransfer->downloadFile( u.id(), fi );
  return true;
}

void Core::checkFileTransferMessage( VNumber peer_id, VNumber user_id, const FileInfo& fi, const QString& msg )
{
  User u = UserManager::instance().findUser( user_id );
  if( !u.isValid() )
  {
    qWarning() << "Unable to find user" << user_id << "for the file transfer" << qPrintable( fi.name() );
    return;
  }

  QString icon_html = IconManager::instance().toHtml( fi.isDownload() ? "download.png" : "upload.png", "*F*" );
  QString action_txt = fi.isDownload() ? tr( "Download" ) : tr( "Upload" );
  QString sys_msg = QString( "%1 %2 %3 %4 %5: %6." ).arg( icon_html, action_txt, fi.name(), fi.isDownload() ? tr( "from") : tr( "to" ), u.name(), msg );
  QString sys_msg_img_preview = "";
  QString sys_msg_open_file = "";

  FileTransferPeer *peer = mp_fileTransfer->peer( peer_id );
  if( peer )
  {
    if( peer->isTransferCompleted() )
    {
      if( fi.isDownload() )
        FileShare::instance().addDownloadedFile( fi );
      QUrl file_url = QUrl::fromLocalFile( fi.path() );
      if( Bee::isFileTypeImage( fi.suffix() ) )
      {
        QImage img;
        QImageReader img_reader( fi.path() );
        img_reader.setAutoDetectImageFormat( true );
        int img_preview_height = Settings::instance().imagePreviewHeight();
        QString img_preview_path = fi.path();
        if( img_reader.read( &img ) )
        {
          if( img.height() > img_preview_height )
          {
            // PNG for transparency (always)
            QString img_file_name = QString( "beeimgtmp-%1-%2.png" ).arg( Bee::dateTimeStringSuffix( QDateTime::currentDateTime() ) ).arg( fi.id() );
            QString img_file_path = Bee::convertToNativeFolderSeparator( QString( "%1/%2" ).arg( Settings::instance().dataFolder() ).arg( img_file_name ) );
            QImage img_scaled = img.scaledToHeight( Settings::instance().imagePreviewHeight(), Qt::SmoothTransformation );
            if( img_scaled.save( img_file_path, "png" ) )
            {
              Settings::instance().addTemporaryFilePath( img_file_path );
              img_preview_path = img_file_path;
            }
          }
          else
            img_preview_height = img.height();

          // I have to add <br> to center the image on the first display (maybe Qt bug)
          sys_msg_img_preview = QString( "<br><div align=center><a href=\"%1\"><img src=\"%2\" height=\"%3\" /></a></div>" )
                  .arg( file_url.toString(), QUrl::fromLocalFile( img_preview_path ).toString() ).arg( img_preview_height );
        }
        else
          qWarning() << "Unable to show image preview of the file" << img_preview_path;
      }

      QString s_open = tr( "Open" );
      sys_msg_open_file = QString( "%1" ).arg( icon_html );
      sys_msg_open_file += QString( " %1 <a href=\"%2\">%3</a>." ).arg( s_open, file_url.toString(), fi.name() );
      file_url.setScheme( QLatin1String( "beeshowfileinfolder" ) );
      sys_msg_open_file += QString( " %1 <a href=\"%2\">%3</a>." ).arg( s_open, file_url.toString(), tr( "folder" ) );
    }
  }

  Chat chat_to_show_message = ChatManager::instance().findChatByPrivateId( fi.chatPrivateId(), false, u.id() );
  if( chat_to_show_message.isValid() && fi.isDownload() )
  {
    chat_to_show_message.addUnreadMessage();
    ChatManager::instance().setChat( chat_to_show_message );
  }

  dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, u.id(), sys_msg, chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
  if( !sys_msg_img_preview.isEmpty() )
    dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, u.id(), sys_msg_img_preview, chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::ImagePreview );
  if( !sys_msg_open_file.isEmpty() )
    dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, u.id(), sys_msg_open_file, chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );

  emit fileTransferMessage( peer_id, u, fi, msg );
}

void Core::checkFileTransferProgress( VNumber peer_id, VNumber user_id, const FileInfo& fi, FileSizeType bytes )
{
  User u = UserManager::instance().findUser( user_id );
  if( !u.isValid() )
  {
    qWarning() << "Unable to find user" << user_id << "for the file transfer" << fi.name();
    return;
  }
  emit fileTransferProgress( peer_id, u, fi, bytes );
}

bool Core::sendFile( VNumber user_id, const QString& file_path, const QString& share_folder, bool to_share_box, VNumber chat_id )
{
  QString icon_html = IconManager::instance().toHtml( "red-ball.png", "*F*" );

  if( !Settings::instance().enableFileTransfer() )
  {
    dispatchSystemMessage( chat_id != ID_INVALID ? chat_id : ID_DEFAULT_CHAT, user_id, tr( "%1 Unable to send %2. File transfer is disabled." ).arg( icon_html, file_path ),
                           chat_id != ID_INVALID ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
    return false;
  }

  if( !mp_fileTransfer->isActive() )
  {
    if( !startFileTransferServer() )
      return false;
  }

  User u = UserManager::instance().findUser( user_id );
  if( !u.isValid() )
  {
    qWarning() << "Unable to find user" << user_id << "to send file" << file_path;
    return false;
  }

  if( u.isLocal() )
    return false;

  if( !isUserConnected( u.id() ) )
  {
    dispatchSystemMessage( chat_id != ID_INVALID ? chat_id : ID_DEFAULT_CHAT, u.id(), tr( "%1 Unable to send %2 to %3: user is offline." ).arg( icon_html, file_path, u.name() ),
                           chat_id != ID_INVALID ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
    qWarning() << "Unable to send" << file_path << "to" << u.path() << "because user is offline";
    return false;
  }

  QFileInfo file( file_path );
  if( !file.exists() )
  {
    dispatchSystemMessage( chat_id != ID_INVALID ? chat_id : ID_DEFAULT_CHAT, u.id(), tr( "%1 %2: file not found." ).arg( icon_html, file_path ),
                           chat_id != ID_INVALID ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
    return false;
  }

  Chat chat_selected;
  if( chat_id != ID_INVALID )
    chat_selected = ChatManager::instance().chat( chat_id );

  if( file.isDir() )
  {
    if( sendFolder( u, file, chat_selected.privateId() ) )
      return true;

    dispatchSystemMessage( chat_id != ID_INVALID ? chat_id : ID_DEFAULT_CHAT, u.id(), tr( "%1 %2 is a folder. You can share it." ).arg( icon_html, file.fileName() ),
                           chat_id != ID_INVALID ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
    return false;
  }

  FileInfo fi = mp_fileTransfer->addFile( file, share_folder, to_share_box, chat_selected.privateId() );

#ifdef BEEBEEP_DEBUG
  qDebug() << "File path" << fi.path() << "is added to file transfer list";
#endif

  Connection* c = connection( u.id() );
  if( c )
  {
    icon_html = IconManager::instance().toHtml( "upload.png", "*F*" );
    Message m = Protocol::instance().fileInfoToMessage( fi );
    if( c->sendMessage( m ) )
    {
      qDebug() << "File Transfer: sending" << fi.path() << "to" << u.path();
      dispatchSystemMessage( chat_id != ID_INVALID ? chat_id : ID_DEFAULT_CHAT, u.id(), tr( "%1 You send %2 to %3." ).arg( icon_html, fi.name(), u.name() ),
                             chat_id != ID_INVALID ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
      return true;
    }
  }

  dispatchSystemMessage( chat_id != ID_INVALID ? chat_id : ID_DEFAULT_CHAT, u.id(), tr( "%1 Unable to send %2: %3 is not connected." )
                         .arg( icon_html ).arg( fi.name() ).arg( u.name() ),
                         chat_id != ID_INVALID ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
  return false;

}

void Core::cancelFileTransfer( VNumber peer_id )
{
  qDebug() << "Core received a request for cancelling file transfer" << peer_id;
  mp_fileTransfer->cancelTransfer( peer_id );
}

void Core::refuseToDownloadFile( VNumber user_id, const FileInfo& fi )
{
  User u = UserManager::instance().findUser( user_id );

  if( !u.isValid() )
  {
    qWarning() << "Unable to find user" << user_id << "to refuse file" << qPrintable( fi.name() );
    return;
  }

  qDebug() << "Download refused:" << qPrintable( fi.name() ) << "from" << qPrintable( u.path() );

  Chat chat_to_show_message = ChatManager::instance().findChatByPrivateId( fi.chatPrivateId(), false, user_id );
  dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, u.id(), tr( "%1 You have refused to download %2 from %3." )
                         .arg( IconManager::instance().toHtml( "download.png", "*F*" ), fi.name(), u.name() ),
                         chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );

  Message m = Protocol::instance().fileInfoRefusedToMessage( fi );

  Connection* c = connection( u.id() );
  if( !c )
  {
    qDebug() << qPrintable( u.path() ) << "is not connected. Unable to send file download refused message";
    return;
  }

  if( c->protoVersion() > 1 )
    c->sendMessage( m );
}

void Core::refuseToDownloadFolder( VNumber user_id, const QString& folder_name, const QString& chat_private_id )
{
  User u = UserManager::instance().findUser( user_id );

  if( !u.isValid() )
  {
    qWarning() << "Unable to find user" << user_id << "to refuse folder" << folder_name;
    return;
  }

  Chat chat_to_show_message = ChatManager::instance().findChatByPrivateId( chat_private_id, false, user_id );
  dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, u.id(), tr( "%1 You have refused to download folder %2 from %3." )
                         .arg( IconManager::instance().toHtml( "download.png", "*F*" ), folder_name, u.name() ),
                         chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );

  Message m = Protocol::instance().folderRefusedToMessage( folder_name, chat_private_id );

  Connection* c = connection( u.id() );
  if( !c )
  {
    qDebug() << u.path() << "is not connected. Unable to send folder download refused message";
    return;
  }

  if( c->protoVersion() > 1 )
    c->sendMessage( m );
}

void Core::onFileTransferServerListening()
{
  createLocalShareMessage();
  buildLocalShareList();
}

void Core::sendFileShareRequestToAll()
{
  Message file_share_request_message = Protocol::instance().fileShareRequestMessage();
  foreach( Connection* c, m_connections )
  {
    if( !FileShare::instance().userHasFileShareList( c->userId() ) )
      c->sendMessage( file_share_request_message );
  }
}

void Core::sendFileShareListTo( VNumber user_id )
{
  Connection* c = connection( user_id );
  if( c )
    c->sendMessage( Protocol::instance().fileShareListMessage() );
  else
    qWarning() << user_id << "is not a valid user id. Unable to send share list message";
}

void Core::sendFileShareListToAll()
{
  if( !isConnected() )
    return;

  Message share_list_message = Protocol::instance().fileShareListMessage();

  foreach( Connection* c, m_connections )
    c->sendMessage( share_list_message );
}

void Core::addPathToShare( const QString& share_path )
{
  QFileInfo fi( share_path );
  if( fi.isDir() )
  {
    BuildFileShareList *bfsl = new BuildFileShareList;
    bfsl->setFolderPath( Bee::convertToNativeFolderSeparator( share_path ) );
    connect( bfsl, SIGNAL( listCompleted() ), this, SLOT( addListToLocalShare() ) );
    if( beeApp )
      beeApp->addJob( bfsl );
    QMetaObject::invokeMethod( bfsl, "buildList", Qt::QueuedConnection );
  }
  else
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "Adding to file sharing" << share_path;
#endif
    FileInfo file_info = Protocol::instance().fileInfo( fi, "", false, "" );
    FileShare::instance().addToLocal( file_info );

    if( m_shareListToBuild > 0 )
      m_shareListToBuild--;

    if( m_shareListToBuild == 0 )
    {
      createLocalShareMessage();
      emit localShareListAvailable();
    }
  }
}

void Core::addListToLocalShare()
{
  if( m_shareListToBuild > 0 )
    m_shareListToBuild--;

  BuildFileShareList *bfsl = qobject_cast<BuildFileShareList*>( sender() );
  if( !bfsl )
  {
    qWarning() << "Core received a signal from invalid BuildFileShareList instance";
    emit localShareListAvailable();
    return;
  }

  int num_files = FileShare::instance().addToLocal( bfsl->folderPath(), bfsl->shareList() );

  QString share_status;

  if( num_files != bfsl->shareList().size() )
    share_status = tr( "%1 is added to file sharing with only %2 of %3 files (%4 limit reached)" )
                     .arg( bfsl->folderPath() )
                     .arg( num_files )
                     .arg( bfsl->shareList().size() )
                     .arg( Settings::instance().maxFileShared() );
  else if( bfsl->shareList().size() < 2 )
    share_status = tr( "%1 is added to file sharing (%2)" ).arg( bfsl->folderPath(), Bee::bytesToString( bfsl->shareSize() ) );
  else
    share_status = tr( "%1 is added to file sharing with %2 files, %3" )
                     .arg( bfsl->folderPath() )
                     .arg( bfsl->shareList().size() )
                     .arg( Bee::bytesToString( bfsl->shareSize() ) );

  dispatchSystemMessage( ID_DEFAULT_CHAT, ID_LOCAL_USER, QString( "%1 %2." ).arg( IconManager::instance().toHtml( "upload.png", "*F*" ), share_status ),
                         DispatchToChat, ChatMessage::FileTransfer );

  if( beeApp )
    beeApp->removeJob( bfsl );

  if( m_shareListToBuild == 0 )
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "Building local share list completed";
#endif
    createLocalShareMessage();
    emit localShareListAvailable();
  }

  bfsl->deleteLater();
}

void Core::removeAllPathsFromShare()
{
  FileShare::instance().clearLocal();
  QStringList local_share = Settings::instance().localShare();
  local_share.clear();
  Settings::instance().setLocalShare( local_share );
  qDebug() << "All paths are removed from local shares";

  QString share_status = tr( "All paths are removed from file sharing" );

  dispatchSystemMessage( ID_DEFAULT_CHAT, ID_LOCAL_USER, QString( "%1 %2." ).arg( IconManager::instance().toHtml( "upload.png", "*F*" ), share_status ),
                         DispatchToChat, ChatMessage::FileTransfer );

  createLocalShareMessage();
  sendFileShareListToAll();

  emit localShareListAvailable();
}

void Core::removePathFromShare( const QString& share_path )
{
  int num_files = FileShare::instance().removePath( share_path );
  QStringList local_share = Settings::instance().localShare();
  if( local_share.removeOne( share_path ) )
    Settings::instance().setLocalShare( local_share );
  qDebug() << "Path" << share_path << "is removed from local shares";

  QString share_status;
  if( num_files < 2 )
    share_status = tr( "%1 is removed from file sharing" ).arg( share_path );
  else
    share_status = tr( "%1 is removed from file sharing with %2 files" ).arg( share_path ).arg( num_files );

  dispatchSystemMessage( ID_DEFAULT_CHAT, ID_LOCAL_USER, QString( "%1 %2." ).arg( IconManager::instance().toHtml( "upload.png", "*F*" ), share_status ),
                         DispatchToChat, ChatMessage::FileTransfer );

  if( num_files > 0 )
  {
    createLocalShareMessage();
    sendFileShareListToAll();
  }

  emit localShareListAvailable();
}

void Core::createLocalShareMessage()
{
  if( mp_fileTransfer->isActive() )
    Protocol::instance().createFileShareListMessage( FileShare::instance().local(), mp_fileTransfer->serverPort() );
  else
    Protocol::instance().createFileShareListMessage( FileShare::instance().local(), -1 );
}

void Core::buildLocalShareList()
{
  if( !FileShare::instance().local().isEmpty() )
  {
    FileShare::instance().clearLocal();
    createLocalShareMessage();
    sendFileShareListToAll();
  }

  if( !Settings::instance().enableFileTransfer() )
  {
    qWarning() << "Skip building local file share list: file transfer is disabled";
    return;
  }

  if( !Settings::instance().enableFileSharing() )
  {
    qWarning() << "Skip building local file share list: file sharing is disabled";
    return;
  }

  if( Settings::instance().localShare().isEmpty() )
  {
    qDebug() << "Skip building local file share list: there are not shared files";
    return;
  }

#ifdef BEEBEEP_DEBUG
  qDebug() << "Building local file share list";
#endif

  m_shareListToBuild = Settings::instance().localShare().size();

  foreach( QString share_path, Settings::instance().localShare() )
    addPathToShare( share_path );
}

bool Core::sendFolder( const User& u, const QFileInfo& file_info, const QString& chat_private_id )
{
  QString icon_html = IconManager::instance().toHtml( "upload.png", "*F*" );

  Chat chat_to_show_message = ChatManager::instance().findChatByPrivateId( chat_private_id, false, u.id() );

  dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, u.id(), tr( "%1 You are about to send %2 to %3. Checking folder..." )
                         .arg( icon_html, file_info.fileName(), u.name() ),
                         chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );

  BuildFileShareList *bfsl = new BuildFileShareList;
  bfsl->setFolderPath( Bee::convertToNativeFolderSeparator( file_info.absoluteFilePath() ) );
  bfsl->setUserId( u.id() );
  bfsl->setChatPrivateId( chat_private_id );
  connect( bfsl, SIGNAL( listCompleted() ), this, SLOT( addFolderToFileTransfer() ) );
  if( beeApp )
    beeApp->addJob( bfsl );
  QMetaObject::invokeMethod( bfsl, "buildList", Qt::QueuedConnection );
  return true;
}

void Core::addFolderToFileTransfer()
{
  BuildFileShareList *bfsl = qobject_cast<BuildFileShareList*>( sender() );
  if( !bfsl )
  {
    qWarning() << "Core received a signal from invalid BuildFileShareList instance";
    return;
  }

  if( beeApp )
    beeApp->removeJob( bfsl );
  QString folder_name = bfsl->folderName();
  QList<FileInfo> file_info_list = bfsl->shareList();
  VNumber user_id = bfsl->userId();
  QString chat_private_id = bfsl->chatPrivateId();
  bfsl->deleteLater();

  if( folder_name.isEmpty() )
  {
    qWarning() << "Unable to send folder. Internal error. Folder is empty";
    return;
  }

  Chat chat_to_show_message = ChatManager::instance().findChatByPrivateId( chat_private_id, false, user_id );
  User u = UserManager::instance().findUser( user_id );
  QString sys_header = tr( "%1 Unable to send folder %2" ).arg( IconManager::instance().toHtml( "red-ball.png", "*F*" ) )
                                                          .arg( folder_name ) + QString( ": " );

  if( !u.isValid() )
  {
    dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, ID_LOCAL_USER, sys_header + tr( "invalid user #%1." ).arg( user_id ),
                           chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
    return;
  }

  if( file_info_list.isEmpty() )
  {
    dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, ID_LOCAL_USER, sys_header + tr( "the folder is empty." ),
                           chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
    return;
  }

  if( !mp_fileTransfer->isActive() )
  {
    dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, ID_LOCAL_USER, sys_header + tr( "file transfer is not working." ),
                           chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
    return;
  }

  Connection* c = connection( u.id() );
  if( !c )
  {
    dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, ID_LOCAL_USER, sys_header + tr( "%1 is not connected." ).arg( u.name() ),
                           chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
    return;
  }

  qDebug() << "File Transfer: sending folder" << folder_name << "to" << u.path();

  mp_fileTransfer->addFileInfoList( file_info_list );

  QString icon_html = IconManager::instance().toHtml( "upload.png", "*F*" );
  Message m = Protocol::instance().createFolderMessage( folder_name, file_info_list, mp_fileTransfer->serverPort() );

  if( !m.isValid() )
  {
    dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, u.id(), sys_header + tr( "internal error." ),
                           chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );
    return;
  }

  dispatchSystemMessage( chat_to_show_message.isValid() ? chat_to_show_message.id() : ID_DEFAULT_CHAT, u.id(), tr( "%1 You send folder %2 to %3." )
                         .arg( icon_html, folder_name, u.name() ),
                         chat_to_show_message.isValid() ? DispatchToChat : DispatchToDefaultAndPrivateChat, ChatMessage::FileTransfer );

  c->sendMessage( m );

}

void Core::sendShareBoxRequest( VNumber user_id, const QString& folder_name, bool create_folder )
{
  if( user_id == ID_INVALID )
  {
    qWarning() << "Invalid user id found in Core::sendShareBoxRequest(...)";
  }
  else if( user_id != ID_LOCAL_USER )
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "ShareBox sends request to user" << user_id << "for folder" << qPrintable( folder_name );
#endif
    Message m = Protocol::instance().shareBoxRequestPathList( folder_name, create_folder );
    Connection* c = connection( user_id );
    if( c && c->sendMessage( m ) )
      return;

    User u = UserManager::instance().findUser( user_id );
    qWarning() << "Unable to send share box request to offline user" << qPrintable( u.path() );
    emit shareBoxUnavailable( u, folder_name );
  }
  else
  {
    if( Settings::instance().useShareBox() && !Settings::instance().shareBoxPath().isEmpty() )
      buildShareBoxFileList( Settings::instance().localUser(), folder_name, create_folder );
    else
      emit shareBoxUnavailable( Settings::instance().localUser(), folder_name );
  }
}

void Core::buildShareBoxFileList( const User& u, const QString& folder_name, bool create_folder )
{
#ifdef BEEBEEP_DEBUG
  qDebug() << "Sharebox builds file list in folder" << qPrintable( folder_name ) << "for user" << qPrintable( u.path() );
#endif
  QString folder_path = QString( "%1%2%3" ).arg( Settings::instance().shareBoxPath() ).arg( QDir::separator() ).arg( folder_name );
  if( create_folder )
  {
    QDir share_box_folder( Settings::instance().shareBoxPath() );
    if( !share_box_folder.mkpath( folder_path ) )
    {
      qWarning() << "ShareBox is unable to create folder" << qPrintable( folder_path ) << "for user" << qPrintable( u.path() );
      if( u.isLocal() )
        emit shareBoxUnavailable( u, folder_name );
      else
        sendMessageToLocalNetwork( u, Protocol::instance().refuseToShareBoxPath( folder_name, true ) );
      return;
    }
    else
      qDebug() << "ShareBox creates folder" << qPrintable( folder_path ) << "for user" << qPrintable( u.path() );
  }

  BuildFileList *bfl = new BuildFileList;
  bfl->init( folder_name, folder_path, u.id() );
  connect( bfl, SIGNAL( listCompleted() ), this, SLOT( sendShareBoxList() ) );
  if( beeApp )
    beeApp->addJob( bfl );
  QMetaObject::invokeMethod( bfl, "buildList", Qt::QueuedConnection );
}

void Core::sendShareBoxList()
{
  BuildFileList *bfl = qobject_cast<BuildFileList*>( sender() );
  if( !bfl )
  {
    qWarning() << "Core received a signal from invalid BuildFileList instance";
    return;
  }

  if( beeApp )
    beeApp->removeJob( bfl );
  QString folder_name = bfl->folderName();
  VNumber to_user_id = bfl->toUserId();
  bool error_found = bfl->errorFound();
  QList<FileInfo> file_info_list = bfl->fileList();
  bfl->deleteLater();

#ifdef BEEBEEP_DEBUG
  qDebug() << "Sharebox has found" << file_info_list.size() << "files in folder" << qPrintable( folder_name );
#endif

  if( to_user_id != ID_LOCAL_USER )
  {
    if( !mp_fileTransfer->isActive() )
      error_found = true;

    Message m;
    if( error_found )
      m = Protocol::instance().refuseToShareBoxPath( folder_name, false );
    else
      m = Protocol::instance().acceptToShareBoxPath( folder_name, file_info_list, mp_fileTransfer->serverPort() );

    Connection* c = connection( to_user_id );
    if( c )
      c->sendMessage( m );
#ifdef BEEBEEP_DEBUG
    else
      qWarning() << "Unable to find connection for user" << to_user_id << "to send share box message";
#endif
  }
  else
    emit shareBoxAvailable( Settings::instance().localUser(), folder_name, file_info_list );
}

void Core::downloadFromShareBox( VNumber from_user_id, const FileInfo& fi, const QString& to_path )
{
#ifdef BEEBEEP_DEBUG
  QString from_path = fi.shareFolder().isEmpty() ? fi.name() : QString( "%1/%2" ).arg( fi.shareFolder(), fi.name() );
  qDebug() << "Download path" << from_path << "from user" << from_user_id << "to path" << to_path
           << "from server" << qPrintable( fi.networkAddress().toString() );
#else
  Q_UNUSED( from_user_id );
#endif

  FileInfo download_file_info = fi;
  download_file_info.setPath( to_path );
  mp_fileTransfer->downloadFile( from_user_id, download_file_info );
}

void Core::uploadToShareBox( VNumber to_user_id, const FileInfo& fi, const QString& to_path )
{
#ifdef BEEBEEP_DEBUG
  qDebug() << "Upload file" << fi.path() << "to path" << to_path << "of user" << to_user_id;
#endif
  sendFile( to_user_id, fi.path(), to_path, true, ID_INVALID );
}

void Core::onFileTransferCompleted( VNumber peer_id, VNumber user_id, const FileInfo& fi )
{
#ifdef BEEBEEP_DEBUG
  qDebug() << "Transfer completed of file" << qPrintable( QDir::toNativeSeparators( fi.path() ) ) << "and send signals";
#endif

  User u = UserManager::instance().findUser( user_id );
  if( !u.isValid() )
  {
    qWarning() << "Invalid user" << user_id << "found on file transfer completed";
    return;
  }

  if( fi.isInShareBox() )
  {
    if( fi.isDownload() )
      emit shareBoxDownloadCompleted( user_id, fi );
    else
      emit shareBoxUploadCompleted( user_id, fi );
  }
  else
    emit fileTransferCompleted( peer_id, u, fi );
}
