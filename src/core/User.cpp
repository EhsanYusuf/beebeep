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

#include "User.h"


User::User()
  : m_id( ID_INVALID ), m_vCard(), m_networkAddress(), m_status( User::Offline ),
    m_statusDescription( "" ), m_color( "#000000" ), m_accountName( "" ), m_version( "" ),
    m_hash( "" ), m_isFavorite( false ), m_qtVersion( "0" ), m_protocolVersion( 0 ),
    m_statusChangedIn()
{
}

User::User( VNumber new_id )
  : m_id( new_id ), m_vCard(), m_networkAddress( QHostAddress( "127.0.0.1" ), DEFAULT_LISTENER_PORT ),
    m_status( User::Offline ), m_statusDescription( "" ), m_color( "#000000" ), m_accountName( "" ),
    m_version( "" ), m_hash( "" ), m_isFavorite( false ), m_qtVersion( "0" ), m_protocolVersion( 0 ),
    m_statusChangedIn()
{
  setName( QString( "Bee%1" ).arg( QString::number( new_id ) ) );
}

User::User( const User& u )
{
  (void)operator=( u );
}

User& User::operator=( const User& u )
{
  if( this != &u )
  {
    m_id = u.m_id;
    m_vCard = u.m_vCard;
    m_networkAddress = u.m_networkAddress;
    m_status = u.m_status;
    m_statusDescription = u.m_statusDescription;
    m_color = u.m_color;
    m_accountName = u.m_accountName;
    m_version = u.m_version;
    m_hash = u.m_hash;
    m_isFavorite = u.m_isFavorite;
    m_qtVersion = u.m_qtVersion;
    m_protocolVersion = u.m_protocolVersion;
    m_statusChangedIn = u.m_statusChangedIn;
  }
  return *this;
}

QString User::nameFromPath( const QString& user_path )
{
  if( !user_path.contains( "@" ) )
    return user_path;

  QStringList sl = user_path.split( "@" );
  if( sl.size() > 2 )
  {
    sl.removeLast();
    return sl.join( "@" );
  }
  else
    return sl.first();
}

QString User::hostAddressAndPortFromPath( const QString& user_path )
{
  QStringList sl = user_path.split( "@" );
  if( sl.size() < 2 )
    return QString();
  else
    return sl.last();
}

QHostAddress User::parseHostAddressFromPath( const QString& user_path )
{
  QString host_and_port = hostAddressAndPortFromPath( user_path );
  QHostAddress host_address;
  if( !host_and_port.isEmpty() )
  {
    if( host_and_port.contains( QLatin1String( ":" ) ) )
    {
      QStringList sl = host_and_port.split( QLatin1String( ":" ) );
      if( !sl.isEmpty() )
        sl.removeLast();
      if( !sl.isEmpty() )
        host_address = QHostAddress( sl.join( QLatin1String( ":" ) ) );
    }
  }
  else
    host_address = QHostAddress( user_path );

  return host_address;
}

bool User::operator<( const User& u ) const
{
  if( isLocal() )
    return false;
  if( u.isLocal() )
    return true;

  if( u.isStatusConnected() && !isStatusConnected() )
    return false;

  if( !u.isStatusConnected() && isStatusConnected() )
    return true;

  if( u.isFavorite() && !isFavorite() )
    return false;

  if( !u.isFavorite() && isFavorite() )
    return true;

  return name() < u.name();
}

bool User::isBirthDay() const
{
  if( m_vCard.birthday().isNull() )
    return false;

  QDate d_today = QDate::currentDate();
  return m_vCard.birthday().day() == d_today.day() && m_vCard.birthday().month() == d_today.month();
}
