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

#include "ChatManager.h"
#include "GuiUserList.h"
#include "GuiConfig.h"
#include "Settings.h"
#include "UserManager.h"


GuiUserList::GuiUserList( QWidget* parent )
  : QWidget( parent )
{
  setupUi( this );
  m_showOnlyChatMembers = false;

#ifdef Q_OS_MAC
  gridLayout->setHorizontalSpacing( -1 );
  gridLayout->setVerticalSpacing( -1 );
#endif

  setObjectName( "GuiUserList" );
  mp_menu = 0;

  mp_twUsers->setContextMenuPolicy( Qt::CustomContextMenu );
  mp_twUsers->setRootIsDecorated( false );
  mp_twUsers->setSortingEnabled( true );
  mp_twUsers->setColumnCount( 1 );

  m_coreIsConnected = false;
  m_filter = "";
  m_blockShowChatRequest = false;

#if QT_VERSION >= 0x040700
  mp_leFilter->setPlaceholderText( tr( "Search" ) );
#endif

  mp_twUsers->setHeaderHidden( true );
  resetList();

  connect( mp_twUsers, SIGNAL( customContextMenuRequested( const QPoint& ) ), this, SLOT( showUserMenu( const QPoint& ) ) );
  connect( mp_twUsers, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this, SLOT( userItemClicked( QTreeWidgetItem*, int ) ), Qt::QueuedConnection );
  connect( mp_leFilter, SIGNAL( textChanged( const QString& ) ), this, SLOT( filterText( const QString& ) ) );
  connect( mp_pbClearFilter, SIGNAL( clicked() ), this, SLOT( clearFilter() ) );
  connect( mp_pbSettings, SIGNAL( clicked() ), this, SLOT( showMenuSettings() ) );
}

void GuiUserList::clear()
{
  resetList();
}

void GuiUserList::sortUsers()
{
  mp_twUsers->sortItems( 0, Settings::instance().sortUsersAscending() ? Qt::AscendingOrder : Qt::DescendingOrder );
}

QSize GuiUserList::sizeHint() const
{
  return QSize( BEE_DOCK_WIDGET_SIZE_HINT_WIDTH, BEE_DOCK_WIDGET_SIZE_HINT_HEIGHT );
}

void GuiUserList::resetList()
{
  if( mp_twUsers->topLevelItemCount() > 0 )
    mp_twUsers->clear();
  if( Settings::instance().showUserPhoto() )
    mp_twUsers->setIconSize( Settings::instance().avatarIconSize() );
  else
    mp_twUsers->setIconSize( QSize( 16, 16 ) );
}

void GuiUserList::updateUsers( bool is_connected )
{
  m_coreIsConnected = is_connected;
  resetList();
  setUser( Settings::instance().localUser(), false );
  setDefaultChatConnected( 0, is_connected );
  foreach( User u, UserManager::instance().userList().toList() )
  {
    if( m_filter.isEmpty() || u.vCard().nickName().contains( m_filter, Qt::CaseInsensitive ) ||
        u.vCard().fullName().contains( m_filter, Qt::CaseInsensitive ) ||
        u.vCard().email().contains( m_filter, Qt::CaseInsensitive ) ||
        u.vCard().phoneNumber().contains( m_filter, Qt::CaseInsensitive ) )
      setUser( u, false );
  }
  sortUsers();
}

GuiUserItem* GuiUserList::itemFromUserId( VNumber user_id )
{
  GuiUserItem* item;
  QTreeWidgetItemIterator it( mp_twUsers );
  while( *it )
  {
    item = (GuiUserItem*)(*it);
    if( item->userId() == user_id )
      return item;
    ++it;
  }
  return 0;
}

GuiUserItem* GuiUserList::itemFromChatId( VNumber chat_id )
{
  GuiUserItem* item;
  QTreeWidgetItemIterator it( mp_twUsers );
  while( *it )
  {
    item = (GuiUserItem*)(*it);
    if( item->chatId() == chat_id )
      return item;
    ++it;
  }
  return 0;
}

void GuiUserList::setUnreadMessages( VNumber private_chat_id, int n )
{
  GuiUserItem* item = itemFromChatId( private_chat_id );
  if( !item )
    return;

  item->setUnreadMessages( n );
  item->updateUser();
  sortUsers();
}

void GuiUserList::setMessages( VNumber private_chat_id, int n )
{
  GuiUserItem* item = itemFromChatId( private_chat_id );
  if( !item )
    return;

  item->setMessages( n );
  item->updateUser();
  sortUsers();
}

void GuiUserList::setUser( const User& u, bool sort_and_check_opened )
{
  GuiUserItem* item = itemFromUserId( u.id() );
  bool item_is_created = false;
  if( !item )
  {
    if( !u.isStatusConnected() && Settings::instance().showOnlyOnlineUsers() )
      return;

    item = new GuiUserItem( mp_twUsers );
    item->setUserId( u.id() );
    item_is_created = true;
  }
  else
  {
    if( !u.isStatusConnected() && Settings::instance().showOnlyOnlineUsers() )
    {
      removeUser( u );
      return;
    }
  }

  Chat c = ChatManager::instance().privateChatForUser( u.id() );
  if( c.isValid() )
  {
    item->setChatId( c.id() );
    item->setUnreadMessages( c.unreadMessages() );
  }

  if( u.isLocal() )
    setDefaultChatConnected( item, m_coreIsConnected );

  item->updateUser( u );

  if( sort_and_check_opened )
    sortUsers();
}

void GuiUserList::removeUser( const User& u )
{
  GuiUserItem* item = itemFromUserId( u.id() );
  if( item )
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "Delete user item from GuiUserList";
#endif
    QTreeWidgetItem* root_item = mp_twUsers->invisibleRootItem();
    if( root_item )
    {
      root_item->removeChild( (QTreeWidgetItem*)item );
      delete item;
    }
  }
}

void GuiUserList::showUserMenu( const QPoint& p )
{
  QTreeWidgetItem* item = mp_twUsers->itemAt( p );
  if( !item )
    return;

  GuiUserItem* user_item = (GuiUserItem*)item;
  if( user_item->chatId() == ID_DEFAULT_CHAT )
  {
    emit chatSelected( ID_DEFAULT_CHAT );
  }
  else
  {
    if( !Settings::instance().showVCardOnRightClick() )
      return;

    m_blockShowChatRequest = true;
    emit showVCardRequest( user_item->userId(), true );
  }
}

void GuiUserList::userItemClicked( QTreeWidgetItem* item, int )
{
  if( !item )
    return;

  if( m_blockShowChatRequest )
  {
    m_blockShowChatRequest = false;
    return;
  }

  GuiUserItem* user_item = (GuiUserItem*)item;
  if( user_item->chatId() != ID_INVALID )
    emit chatSelected( user_item->chatId() );
  else
    emit userSelected( user_item->userId() );
}

void GuiUserList::setDefaultChatConnected( GuiUserItem* item, bool yes )
{
  if( !item )
    item = itemFromChatId( ID_DEFAULT_CHAT );
  if( !item )
    return;
  QIcon def_icon = QIcon( (yes ? ":/images/default-chat-online" : ":/images/default-chat-offline" ) );
  item->setIcon( 0, def_icon );
  item->setDefaultIcon( def_icon );
}

void GuiUserList::filterText( const QString& txt )
{
  QString new_filter = txt.trimmed().toLower();
  if( m_filter == new_filter )
    return;

  m_filter = new_filter;
  updateUsers( m_coreIsConnected );
}

void GuiUserList::clearFilter()
{
  mp_leFilter->setText( "" );
}

void GuiUserList::showMenuSettings()
{
  if( mp_menu )
    mp_menu->exec( QCursor::pos() );
}

void GuiUserList::onTickEvent( int ticks )
{
  GuiUserItem* item;
  QTreeWidgetItemIterator it( mp_twUsers );
  while( *it )
  {
    item = (GuiUserItem*)(*it);
    item->onTickEvent( ticks );
    ++it;
  }
}
