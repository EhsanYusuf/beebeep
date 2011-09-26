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

#include "BeeUtils.h"
#include "Connection.h"
#include "Core.h"
#include "Broadcaster.h"
#include "Protocol.h"
#include "Settings.h"
#include "UserManager.h"


void Core::setLocalUserStatus( int new_status )
{
  if( Settings::instance().localUser().status() == new_status )
    return;
  User u = Settings::instance().localUser();
  u.setStatus( new_status );
  Settings::instance().setLocalUser( u );
  sendLocalUserStatus();
  showUserStatusChanged( u );
}

void Core::setLocalUserStatusDescription( const QString& new_status_description )
{
  if( Settings::instance().localUser().statusDescription() == new_status_description )
    return;
  User u = Settings::instance().localUser();
  u.setStatusDescription( new_status_description );
  Settings::instance().setLocalUser( u );
  sendLocalUserStatus();
  showUserStatusChanged( u );
}

void Core::showUserStatusChanged( const User& u )
{
  // Before signal is emitted, so chat is created in gui ... FIXME ??? ...
  emit userChanged( u );

  QString sHtmlMsg = Bee::iconToHtml( Bee::userStatusIconFileName( u.status() ), "*S*" ) + QString( " " );
  if( u.isLocal() )
    sHtmlMsg += tr( "You are" );
  else
    sHtmlMsg += tr( "%1 is" ).arg( Settings::instance().showOnlyUsername() ? u.name() : u.path() );
   sHtmlMsg += QString( " %1%2." ).arg( Bee::userStatusToString( u.status() ) )
                            .arg( (u.statusDescription().isEmpty() || u.status() == User::Offline) ? "" : QString( ": %1").arg( u.statusDescription() ) );
  dispatchSystemMessage( ID_DEFAULT_CHAT, u.id(), sHtmlMsg, DispatchToAllChatsWithUser );
}

void Core::showUserNameChanged( const User& u, const QString& old_user_name )
{
  QString sHtmlMsg = Bee::iconToHtml( ":/images/profile.png", "*N*" ) + QString( " " );
  if( u.isLocal() )
    sHtmlMsg += tr( "You have changed your nickname from %1 to %2." ).arg( old_user_name, u.name() );
  else
    sHtmlMsg += tr( "%1 has changed the nickname in %2." ).arg( old_user_name, u.name() );

  dispatchSystemMessage( ID_DEFAULT_CHAT, u.id(), sHtmlMsg, DispatchToAllChatsWithUser );
  emit userChanged( u );
}

void Core::showUserVCardChanged( const User& u )
{
  QString sHtmlMsg = Bee::iconToHtml( ":/images/profile.png", "*V*" ) + QString( " " );
  if( u.isLocal() )
    sHtmlMsg += tr( "You have changed your profile." );
  else
    sHtmlMsg += tr( "%1 has changed the profile." ).arg( u.name() );

  dispatchSystemMessage( ID_DEFAULT_CHAT, u.id(), sHtmlMsg, DispatchToAllChatsWithUser );
  emit userChanged( u );
}

void Core::searchUsers( const QHostAddress& host_address )
{
  mp_broadcaster->sendDatagramToHost( host_address );
  QString sHtmlMsg = tr( "%1 Looking for the available users in the network address %2..." )
      .arg( Bee::iconToHtml( ":/images/search.png", "*B*" ), host_address.toString() );
  dispatchSystemMessage( ID_DEFAULT_CHAT, ID_LOCAL_USER, sHtmlMsg, DispatchToChat );
}

void Core::sendLocalUserStatus()
{
  QByteArray user_status_message = Protocol::instance().localUserStatusMessage();
  foreach( Connection *c, m_connections )
    c->sendData( user_status_message );
  sendLocalUserStatusToXmppServer();
}

bool Core::setUserColor( VNumber user_id, const QString& user_color )
{
  User u = UserManager::instance().userList().find( user_id );
  if( !u.isValid() )
    return false;
  u.setColor( user_color );
  UserManager::instance().setUser( u );
  userChanged( u );
  return true;
}

void Core::setLocalUserVCard( const VCard& vc )
{
  User u = Settings::instance().localUser();
  bool nick_name_changed = u.vCard().nickName() != vc.nickName();
  u.setVCard( vc );
  Settings::instance().setLocalUser( u );

  QByteArray vcard_message = Protocol::instance().localVCardMessage();
  QByteArray nick_message = Protocol::instance().localUserNameMessage();

  foreach( Connection *c, m_connections )
  {
    if( c->protoVersion() == 1 )
    {
      if( nick_name_changed )
        c->sendData( nick_message );
    }
    else
      c->sendData( vcard_message );
  }

  showUserVCardChanged( u );
}

