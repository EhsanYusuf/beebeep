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

#include "FileTransferDownload.h"
#include "Protocol.h"


FileTransferDownload::FileTransferDownload( VNumber peer_id, const FileInfo& fi, QObject *parent )
  : FileTransferPeer( peer_id, parent )
{
  setFileInfo( fi );
#if defined( BEEBEEP_DEBUG )
  qDebug() << "Download the file" << m_fileInfo.name() << "from address" << m_fileInfo.hostAddress().toString() << m_fileInfo.hostPort();
#endif
  connect( this, SIGNAL( userAuthenticated() ), this, SLOT( sendData() ) );
}

void FileTransferDownload::sendData()
{
  switch( m_state )
  {
  case FileTransferPeer::Request:
    sendDownlodRequest();
    break;
  case FileTransferPeer::Transferring:
    sendDataConfirmation();
    break;
  default:
    // do_nothing
    break;
  }
}

void FileTransferDownload::sendDownlodRequest()
{
#if defined( BEEBEEP_DEBUG )
  qDebug() << "Sending REQUEST:" << m_fileInfo.password();
#endif
  m_socket.sendData( Protocol::instance().fromMessage( Protocol::instance().fileInfoToMessage( m_fileInfo ) ) );
  m_state = FileTransferPeer::Transferring;
}

void FileTransferDownload::sendDataConfirmation()
{
#if defined( BEEBEEP_DEBUG )
  qDebug() << "Download corfirmation for" << m_bytesTransferred << "bytes";
#endif
  m_socket.sendData( QByteArray::number( m_bytesTransferred ) );
}

void FileTransferDownload::checkData( const QByteArray& byte_array )
{
  m_bytesTransferred = byte_array.size();
  m_totalBytesTransferred += m_bytesTransferred;

  if( !m_file.isOpen() )
  {
    if( !m_file.open( QIODevice::WriteOnly ) )
    {
      setError( tr( "Unable to open file") );
      return;
    }
  }

  showProgress();

  if( m_file.write( byte_array ) == m_bytesTransferred )
  {
    sendData();
  }
  else
  {
    setError( tr( "Unable to write in the file" ) );
    return;
  }

  if( m_totalBytesTransferred == m_fileInfo.size() )
    setTransferCompleted();
}
