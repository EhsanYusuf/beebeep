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

#include "GuiSystemTray.h"
#include "Settings.h"


GuiSystemTray::GuiSystemTray( QObject *parent )
  : QSystemTrayIcon( parent ), m_iconStatus( -1 ), m_chatId( ID_DEFAULT_CHAT )
{
  setDefaultIcon();
}

void GuiSystemTray::showNewMessageArrived( VNumber chat_id, const QString& msg, bool lont_time )
{
  m_chatId = chat_id;
  setMessageIcon();
  showMessageInTray( msg, lont_time );
}

void GuiSystemTray::showUserStatusChanged( VNumber chat_id, const QString& msg )
{
  m_chatId = chat_id;
  showMessageInTray( msg, false );
}

void GuiSystemTray::setUnreadMessages( VNumber chat_id, int ur )
{
  m_chatId = chat_id;
  if( ur > 0 )
    setMessageIcon();
  else
    setDefaultIcon();
}

void GuiSystemTray::setDefaultIcon()
{
  if( m_iconStatus != GuiSystemTray::Default )
  {
    setIcon( QIcon( ":/images/beebeep.png" ) );
    m_iconStatus = GuiSystemTray::Default;
  }
}

void GuiSystemTray::setMessageIcon()
{
  if( m_iconStatus != GuiSystemTray::Message )
  {
    setIcon( QIcon( ":/images/beebeep-message.png" ) );
    m_iconStatus = GuiSystemTray::Message;
  }
}

void GuiSystemTray::showMessageInTray( const QString& msg, bool long_time )
{
  if( Settings::instance().showNotificationOnTray() && Settings::instance().trayMessageTimeout() > 0 )
  {
    int min_time = 1000;
    int show_time = qMax( min_time, Settings::instance().trayMessageTimeout() );
    if( long_time )
    {
      min_time = 3000;
      show_time = qMax( 5000, show_time * 2 );
    }

    showMessage( msg, Settings::instance().programName(), QSystemTrayIcon::Information, show_time );
  }
}

void GuiSystemTray::onTickEvent( int ticks )
{
  if( m_iconStatus == GuiSystemTray::Message )
  {
    if( ticks % 2 == 0 )
      setIcon( QIcon( ":/images/beebeep.png" ) );
    else
      setIcon( QIcon( ":/images/beebeep-message.png" ) );
  }
}