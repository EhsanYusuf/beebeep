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
// Author: Marco Mastroddi (marco.mastroddi(AT)gmail.com)
//
// $Id$
//
//////////////////////////////////////////////////////////////////////

#include "FileTransferPeer.h"


FileTransferPeer::FileTransferPeer( const FileInfo& fi, QObject *parent )
  : QObject( parent ), m_fileInfo( fi ), m_socket(), m_file(), m_state( FileTransferPeer::Unknown ), m_bytesTransferred( 0 ), m_totalBytesTransferred( 0 )
{
#if defined( BEEBEEP_DEBUG )
  qDebug() << "Peer created for file" << m_fileInfo.name();
#endif
  connect( &m_socket, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( socketError( QAbstractSocket::SocketError ) ) );
  connect( &m_socket, SIGNAL( connected() ), this, SLOT( sendData() ) );
  connect( &m_socket, SIGNAL( dataReceived( const QByteArray& ) ), this, SLOT( checkData( const QByteArray& ) ) );
}

void FileTransferPeer::closeAll()
{
#if defined( BEEBEEP_DEBUG )
  qDebug() << "Making cleanup";
#endif
  if( m_socket.isOpen() )
  {
#if defined( BEEBEEP_DEBUG )
    qDebug() << "Closing socket";
#endif
    m_socket.close();
  }
  if( m_file.isOpen() )
  {
#if defined( BEEBEEP_DEBUG )
    qDebug() << "Closing file";
#endif
    m_file.close();
  }
  emit transferFinished();
}

void FileTransferPeer::startTransfer( int socket_descriptor )
{
#if defined( BEEBEEP_DEBUG )
  qDebug() << "Start transfer with socket" << socket_descriptor;
#endif
  m_state = FileTransferPeer::Auth;
  m_bytesTransferred = 0;
  m_totalBytesTransferred = 0;
  m_file.setFileName( m_fileInfo.path() );
#if defined( BEEBEEP_DEBUG )
  qDebug() << "Init the file" << m_file.fileName();
#endif
  if( socket_descriptor )
  {
    m_socket.setSocketDescriptor( socket_descriptor );
#if defined( BEEBEEP_DEBUG )
    qDebug() << "Using socket descriptor" << socket_descriptor;
#endif
  }
  else
  {
    m_socket.connectToHost( m_fileInfo.hostAddress(), m_fileInfo.hostPort() );
#if defined( BEEBEEP_DEBUG )
    qDebug() << "Connecting to" << m_fileInfo.hostAddress() << ":" << m_fileInfo.hostPort();
#endif
  }
}

void FileTransferPeer::setTransferCompleted()
{
#if defined( BEEBEEP_DEBUG )
  qDebug() << m_fileInfo.name() << ": transfer completed";
#endif
  m_state = FileTransferPeer::Completed;
  emit transferMessage( m_fileInfo, tr( "file transfer completed") );
  closeAll();
}

void FileTransferPeer::socketError( QAbstractSocket::SocketError se )
{
  setError( tr( "connection error #%1" ).arg( QString::number( se ) ) );
}

void FileTransferPeer::setError( const QString& str_err )
{
  m_state = FileTransferPeer::Error;
  QString s = tr( "file transfer error occurred. %1" ).arg( str_err );
  qWarning() << s;
  emit transferMessage( m_fileInfo, str_err );
  closeAll();
}
