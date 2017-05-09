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

#include "GuiChatList.h"
#include "GuiConfig.h"
#include "ChatManager.h"
#include "Settings.h"
#include "UserManager.h"


GuiChatList::GuiChatList( QWidget* parent )
  : QWidget( parent )
{
  setObjectName( "GuiChatList" );
  setupUi( this );

  mp_twChatList->setColumnCount( 1 );
  mp_twChatList->header()->hide();
  mp_twChatList->setRootIsDecorated( false );
  mp_twChatList->setSortingEnabled( true );
  mp_twChatList->setIconSize( Settings::instance().avatarIconSize() );
  mp_twChatList->setContextMenuPolicy( Qt::CustomContextMenu );
  mp_twChatList->setMouseTracking( true );
  mp_twChatList->setHeaderHidden( true );
  QString w_stylesheet = "background: white url(:/images/chat-list.png);"
                        "background-repeat: no-repeat;"
                        "background-position: bottom center;";
  mp_twChatList->setStyleSheet( w_stylesheet );

  m_chatSelected = ID_INVALID;
  m_blockShowChatRequest = false;
  m_filter = "";

#if QT_VERSION >= 0x040700
  mp_leFilter->setPlaceholderText( tr( "Search chat" ) );
#endif

  mp_menuContext = new QMenu( this );

  QAction* act = mp_menuContext->addAction( QIcon( ":/images/chat.png" ), tr( "Show" ), this, SLOT( openChatSelected() ) );
  mp_menuContext->setDefaultAction( act );
  mp_menuContext->addSeparator();
  mp_actClear = mp_menuContext->addAction( QIcon( ":/images/clear.png" ), tr( "Clear" ), this, SLOT( clearChatSelected() ) );
  mp_actClear->setToolTip( tr( "Clear all chat messages" ) );
  mp_menuContext->addSeparator();
  mp_actDelete = mp_menuContext->addAction( QIcon( ":/images/delete.png" ), tr( "Delete" ), this, SLOT( removeChatSelected() ) );

  connect( mp_twChatList, SIGNAL( customContextMenuRequested( const QPoint& ) ), this, SLOT( showChatMenu( const QPoint& ) ) );
  connect( mp_twChatList, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this, SLOT( chatClicked( QTreeWidgetItem*, int ) ), Qt::QueuedConnection );
  connect( mp_leFilter, SIGNAL( textChanged( const QString& ) ), this, SLOT( filterText( const QString& ) ) );
  connect( mp_pbClearFilter, SIGNAL( clicked() ), this, SLOT( clearFilter() ) );
}

void GuiChatList::reloadChatList()
{
  mp_twChatList->clearSelection();
  mp_twChatList->clear();
  mp_twChatList->setIconSize( Settings::instance().avatarIconSize() );
  foreach( Chat c, ChatManager::instance().constChatList() )
    updateChat( c );
}

GuiChatItem* GuiChatList::itemFromChatId( VNumber chat_id )
{
  GuiChatItem* item;
  QTreeWidgetItemIterator it( mp_twChatList );
  while( *it )
  {
    item = (GuiChatItem*)(*it);
    if( item->chatId() == chat_id )
      return item;
    ++it;
  }
  return 0;
}

void GuiChatList::updateChat( const Chat& c )
{
  if( !c.isValid() )
    return;

  if( !m_filter.isEmpty() )
  {
    if( c.isDefault() )
    {
      if( !GuiChatItem::defaultChatName().contains( m_filter, Qt::CaseInsensitive ) )
        return;
    }
    else
    {
      if( !c.name().contains( m_filter, Qt::CaseInsensitive ) )
        return;
    }
  }

  GuiChatItem* item = itemFromChatId( c.id() );
  if( !item )
  {
    item = new GuiChatItem( mp_twChatList );
    item->setChatId( c.id() );
  }

  item->updateItem( c );
}

void GuiChatList::updateUser( const User& u )
{
  QList<Chat> chat_list = ChatManager::instance().chatsWithUser( u.id() );
  foreach( Chat c, chat_list )
    updateChat( c );
}

void GuiChatList::chatClicked( QTreeWidgetItem* item, int )
{
  if( !item )
    return;

  if( m_blockShowChatRequest )
  {
    m_blockShowChatRequest = false;
    return;
  }

  mp_twChatList->clearSelection();
  GuiChatItem* user_item = (GuiChatItem*)item;
  emit chatSelected( user_item->chatId() );
}

void GuiChatList::showChatMenu( const QPoint& p )
{
  QTreeWidgetItem* item = mp_twChatList->itemAt( p );
  if( !item )
  {
    QMenu* menu_create_chat = new QMenu( this );
    bool create_chat_is_enabled = true;
    if( UserManager::instance().userList().toList().size() < 2 )
    {
      menu_create_chat->addAction( QIcon( ":/images/info.png" ), tr( "Please wait for two or more users" ) );
      create_chat_is_enabled = false;
      menu_create_chat->addSeparator();
    }

    QAction* act = menu_create_chat->addAction( QIcon( ":/images/chat-create.png" ), tr( "Create chat" ), this, SIGNAL( createNewChatRequest() ) );
    act->setEnabled( create_chat_is_enabled );
    menu_create_chat->exec( QCursor::pos() );
    return;
  }

  GuiChatItem* chat_item = (GuiChatItem*)item;
  m_chatSelected = chat_item->chatId();

  Chat c = ChatManager::instance().chat( m_chatSelected );
  if( !c.isValid() )
    return;

  m_blockShowChatRequest = true;

  mp_actClear->setDisabled( ChatManager::instance().isChatEmpty( c, true ) );
  mp_actDelete->setDisabled( c.isDefault() || c.isPrivate() );

  mp_menuContext->exec( QCursor::pos() );

  mp_twChatList->clearSelection();
}

void GuiChatList::openChatSelected()
{
  emit chatSelected( m_chatSelected );
}

void GuiChatList::clearChatSelected()
{
  emit chatToClear( m_chatSelected );
}

void GuiChatList::removeChatSelected()
{
  emit chatToRemove( m_chatSelected );
}

void GuiChatList::onTickEvent( int ticks )
{
  GuiChatItem* item;
  QTreeWidgetItemIterator it( mp_twChatList );
  while( *it )
  {
    item = (GuiChatItem*)(*it);
    item->onTickEvent( ticks );
    ++it;
  }
}

void GuiChatList::filterText( const QString& txt )
{
  QString new_filter = txt.trimmed().toLower();
  if( m_filter == new_filter )
    return;

  m_filter = new_filter;
  reloadChatList();
}

void GuiChatList::clearFilter()
{
  mp_leFilter->setText( "" );
}
