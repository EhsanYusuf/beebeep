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

#include "Connection.h"
#include "Protocol.h"
#include "Settings.h"

#undef CONNECTION_PING_PONG_DEBUG



Connection::Connection( QObject *parent )
  : ConnectionSocket( parent )
{
  m_pingTimer.setInterval( Settings::instance().pingInterval() );
  connect( this, SIGNAL( dataReceived( const QByteArray& ) ), this, SLOT( parseData( const QByteArray& ) ) );
  connect( this, SIGNAL( disconnected() ), &m_pingTimer, SLOT( stop() ) );
  connect( &m_pingTimer, SIGNAL( timeout() ), this, SLOT( sendPing() ) );
}

bool Connection::sendMessage( const Message& m )
{
  QByteArray message_data = Protocol::instance().fromMessage( m );
  if( message_data.isEmpty() )
  {
    qWarning() << "Connection has received an invalid message to send";
    return false;
  }
  return sendData( message_data );
}

void Connection::parseData( const QByteArray& message_data )
{
  Message m = Protocol::instance().toMessage( message_data );
  if( !m.isValid() )
  {
    qWarning() << "Connection from" << peerAddress().toString() << peerPort() << "has received an invalid message data";
    return;
  }

  switch( m.type() )
  {
  case Message::Ping:
#if defined( CONNECTION_PING_PONG_DEBUG )
    qDebug() << "PING received from" << peerAddress().toString() << peerPort();
#endif
    sendPong();
    break;
  case Message::Pong:
#if defined( CONNECTION_PING_PONG_DEBUG )
    qDebug() << "PONG received from" << peerAddress().toString() << peerPort();
#endif
    m_pongTime.restart();
    break;
  default:
    emit newMessage( userId(), m );
    break;
  }
}

void Connection::setReadyForUse( VNumber user_id )
{
  qDebug() << "Connection is ready for use by the user" << user_id;
  setUserId( user_id );
  m_pingTimer.start();
  m_pongTime.start();
}

void Connection::sendPing()
{
  if( m_pongTime.elapsed() > Settings::instance().pongTimeout() )
  {
    abort();
    return;
  }

#if defined( CONNECTION_PING_PONG_DEBUG )
  qDebug() << "Sending PING to" << peerAddress().toString() << peerPort();
#endif
  if( !sendData( Protocol::instance().pingMessage() ) )
    qWarning() << "Unable to send PING to" << peerAddress().toString() << peerPort();
}

void Connection::sendPong()
{
#if defined( CONNECTION_PING_PONG_DEBUG )
  qDebug() << "Sending PONG to" << peerAddress().toString() << peerPort();
#endif
  if( !sendData( Protocol::instance().pongMessage() ) )
    qWarning() << "Unable to send PONG to" << peerAddress().toString() << peerPort();
}
