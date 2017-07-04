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

#include "BeeUtils.h"
#include "GuiIconProvider.h"
#include "GuiShareBox.h"
#include "GuiShareBoxFileInfoList.h"
#include "IconManager.h"
#include "Settings.h"
#include "FileDialog.h"
#include "UserManager.h"


GuiShareBox::GuiShareBox( QWidget *parent )
  : QWidget( parent )
{
  setupUi( this );
  setObjectName( "GuiShareBox" );

  QGridLayout* grid_layout = new QGridLayout( this );
  grid_layout->setSpacing( 0 );
  grid_layout->setObjectName( QString::fromUtf8( "grid_layout" ) );
  grid_layout->setContentsMargins( 0, 0, 0, 0 );

  grid_layout->addWidget( mp_lTitle, 0, 0, 1, 1 );

  mp_splitter = new QSplitter( this );
  mp_splitter->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
  mp_splitter->setOrientation( Qt::Horizontal );
  mp_splitter->setChildrenCollapsible( false );
  mp_splitter->addWidget( mp_myFrame );
  mp_splitter->addWidget( mp_outFrame );
  grid_layout->addWidget( mp_splitter, 1, 0, 1, 1);

  mp_lTitle->setText( QString( "<b>%1</b>" ).arg( tr( "ShareBox" ) ) );

  mp_myBox->initTree();
  mp_outBox->initTree();

  m_myCurrentFolder = "";
  m_outCurrentFolder = "";
  m_userId = ID_INVALID;

  mp_pbMyUpdate->setToolTip( tr( "Update your ShareBox" ) );
  mp_pbMyCreateFolder->setToolTip( tr( "Create folder in your ShareBox" ) );
  mp_pbMyOpenFolder->setToolTip( tr( "Show current folder" ) );
  mp_pbOutUpdate->setToolTip( tr( "Update ShareBox" ) );
  mp_pbUpdate->setToolTip( tr( "Update ShareBox" ) );
  mp_pbOutCreateFolder->setToolTip( tr( "Create folder in ShareBox" ) );

  mp_pbSelectMyBox->setIcon( IconManager::instance().icon( "sharebox.png" ) );
  mp_pbMyUpdate->setIcon( IconManager::instance().icon( "update.png" ) );
  mp_pbOutUpdate->setIcon( IconManager::instance().icon( "update.png" ) );
  mp_pbUpdate->setIcon( IconManager::instance().icon( "update.png" ) );
  mp_pbMyCreateFolder->setIcon( IconManager::instance().icon( "folder-add.png" ) );
  mp_pbOutCreateFolder->setIcon( IconManager::instance().icon( "folder-add.png" ) );
  mp_pbMyOpenFolder->setIcon( IconManager::instance().icon( "folder.png" ) );

  connect( mp_comboUsers, SIGNAL( currentIndexChanged( int ) ), this, SLOT( onShareBoxSelected( int ) ) );
  connect( mp_pbSelectMyBox, SIGNAL( clicked() ), this, SLOT( selectMyShareBoxFolder() ) );
  connect( mp_cbEnableMyBox, SIGNAL( toggled( bool ) ), this, SLOT( onEnableMyShareBoxClicked() ) );
  connect( mp_pbMyUpdate, SIGNAL( clicked() ), this, SLOT( updateMyBox() ) );
  connect( mp_pbOutUpdate, SIGNAL( clicked() ), this, SLOT( updateOutBox() ) );
  connect( mp_pbUpdate, SIGNAL( clicked() ), this, SLOT( updateOutBoxToBaseFolder() ) );
  connect( mp_myBox, SIGNAL( itemDoubleClicked( QTreeWidgetItem*, int ) ), this, SLOT( onMyItemDoubleClicked( QTreeWidgetItem*, int ) ) );
  connect( mp_outBox, SIGNAL( itemDoubleClicked( QTreeWidgetItem*, int ) ), this, SLOT( onOutItemDoubleClicked( QTreeWidgetItem*, int ) ) );
  connect( mp_myBox, SIGNAL( dropEventRequest( const QString& ) ), this, SLOT( dropInMyBox( const QString& ) ) );
  connect( mp_outBox, SIGNAL( dropEventRequest( const QString& ) ), this, SLOT( dropInOutBox( const QString& ) ) );
  connect( mp_pbMyCreateFolder, SIGNAL( clicked() ), this, SLOT( createFolderInMyBox() ) );
  connect( mp_pbOutCreateFolder, SIGNAL( clicked() ), this, SLOT( createFolderInOutBox() ) );
  connect( mp_pbMyOpenFolder, SIGNAL( clicked() ), this, SLOT( openMyBox() ) );
}

void GuiShareBox::updateShareBoxes()
{
  mp_cbEnableMyBox->setChecked( Settings::instance().useShareBox() );
  if( mp_comboUsers->count() > 0 )
    mp_comboUsers->clear();
  foreach( User u, UserManager::instance().userList().toList() )
  {
    if( u.isStatusConnected() )
      mp_comboUsers->addItem( u.name(), u.id() );
  }

  mp_pbUpdate->setEnabled( mp_comboUsers->count() > 0 );
  updateMyBox();
  updateOutBox();
}

void GuiShareBox::onEnableMyShareBoxClicked()
{
  Settings::instance().setUseShareBox( mp_cbEnableMyBox->isChecked() );
  if( mp_cbEnableMyBox->isChecked() )
  {
    bool select_box = Settings::instance().shareBoxPath().isEmpty();
    if( !select_box )
    {
      QDir box_folder( Settings::instance().shareBoxPath() );
      if( box_folder.exists() )
        select_box = !Bee::folderIsWriteable( Settings::instance().shareBoxPath() );
    }

    if( select_box )
    {
      QMessageBox::information( this, Settings::instance().programName(), tr( "ShareBox path does not exist. Please select a valid folder.") );
      selectMyShareBoxFolder();
      return;
    }
  }

  Settings::instance().save();
  updateMyBox();
}

void GuiShareBox::enableMyUpdateButton()
{
  if( !mp_pbMyUpdate->isEnabled() )
    mp_pbMyUpdate->setEnabled( mp_cbEnableMyBox->isChecked() );
}

void GuiShareBox::enableOutUpdateButton()
{
  if( !mp_pbOutUpdate->isEnabled() )
    mp_pbOutUpdate->setEnabled( mp_cbEnableMyBox->isChecked() );
}

void GuiShareBox::enableUpdateButton()
{
  if( !mp_pbUpdate->isEnabled() )
    mp_pbUpdate->setEnabled( mp_comboUsers->count() > 0 );
}

void GuiShareBox::updateMyBox()
{
  mp_pbMyUpdate->setEnabled( false );
  if( mp_cbEnableMyBox->isChecked() )
  {
    mp_myBox->setEnabled( true );
    QString my_path = QString( "%1%2%3" ).arg( Settings::instance().shareBoxPath() ).arg( QDir::separator() ).arg( m_myCurrentFolder );
    mp_lMyBox->setToolTip( my_path );
    emit shareBoxRequest( ID_LOCAL_USER, m_myCurrentFolder, false );
    QTimer::singleShot( 10000, this, SLOT( enableMyUpdateButton() ) );
    mp_pbMyCreateFolder->setEnabled( true );
    mp_pbMyOpenFolder->setEnabled( true );
  }
  else
  {
    mp_myBox->clearTree();
    m_myCurrentFolder = "";
    mp_lMyBox->setToolTip( "" );
    mp_lMyBox->setText( tr( "Your ShareBox is disabled" ) );
    mp_myBox->setDisabled( true );
    mp_pbMyCreateFolder->setEnabled( false );
    mp_pbMyOpenFolder->setEnabled( !Settings::instance().shareBoxPath().isEmpty() );
  }
}

void GuiShareBox::updateOutBox()
{
  mp_pbOutUpdate->setEnabled( false );
#if QT_VERSION >= 0x050000
  VNumber user_id = mp_comboUsers->count() > 0 ? Bee::qVariantToVNumber( mp_comboUsers->currentData() ) : ID_INVALID;
#else
  VNumber user_id = mp_comboUsers->count() > 0 ? Bee::qVariantToVNumber( mp_comboUsers->itemData( mp_comboUsers->currentIndex() ) ) : ID_INVALID;
#endif
  if( user_id > ID_INVALID )
  {
    mp_outBox->setEnabled( false );
    if( user_id != m_userId )
      emit shareBoxRequest( user_id, "", false );
    else
      emit shareBoxRequest( user_id, m_outCurrentFolder, false );
    mp_pbOutCreateFolder->setEnabled( false );
    QTimer::singleShot( 10000, this, SLOT( enableOutUpdateButton() ) );
  }
  else
  {
    mp_outBox->setEnabled( false );
    mp_outBox->clearTree();
    m_outCurrentFolder = "";
    mp_lOutBox->setText( tr( "ShareBox is not available" ) );
    mp_pbOutCreateFolder->setEnabled( false );
  }
}

void GuiShareBox::updateBox( const User& u, const QString& folder_path, const QList<FileInfo>& file_info_list )
{
  if( u.isLocal() )
  {
    updateMyBox( folder_path, file_info_list );
    enableMyUpdateButton();
  }
  else
  {
    updateOutBox( u, folder_path, file_info_list );
    enableOutUpdateButton();
  }
}

void GuiShareBox::updateMyBox( const QString& folder_path, const QList<FileInfo>& file_info_list )
{
  m_myCurrentFolder = folder_path;
  mp_myBox->setFileInfoList( file_info_list );
  if( !folder_path.isEmpty() )
    mp_myBox->addDotDotFolder();

  QString s_title = QString( "<b>./%1</b>" ).arg( m_myCurrentFolder );
  mp_lMyBox->setText( Bee::convertToNativeFolderSeparator( s_title ) );
}

void GuiShareBox::updateOutBox( const User& u, const QString& folder_path, const QList<FileInfo>& file_info_list )
{
  m_userId = u.id();
  m_outCurrentFolder = folder_path;
  mp_outBox->setEnabled( true );
  mp_outBox->setFileInfoList( file_info_list );
  if( !folder_path.isEmpty() )
    mp_outBox->addDotDotFolder();

  QString s_title = QString( "<b>./%1</b>" ).arg( m_outCurrentFolder );
  mp_lOutBox->setText( Bee::convertToNativeFolderSeparator( s_title ) );

  mp_comboUsers->blockSignals( true );
  if( mp_comboUsers->findData( u.id() ) == -1 )
    mp_comboUsers->addItem( u.name(), u.id() );

  mp_comboUsers->setEnabled( mp_comboUsers->count() > 0 );
  mp_comboUsers->blockSignals( false );
  mp_pbOutCreateFolder->setEnabled( true );
}

void GuiShareBox::onMyItemDoubleClicked( QTreeWidgetItem* item, int )
{
  if( !item )
    return;

  GuiShareBoxFileInfoItem* file_info_item = (GuiShareBoxFileInfoItem*)item;

  if( file_info_item->isFolder() )
  {
    QString new_folder;
    if( file_info_item->fileInfo().id() == ID_DOTDOT_FOLDER )
      new_folder = Bee::folderCdUp( m_myCurrentFolder );
    else if( m_myCurrentFolder.isEmpty() )
      new_folder = QString( "%1" ).arg( file_info_item->fileInfo().name() );
    else
      new_folder = QString( "%1%2%3" ).arg( m_myCurrentFolder ).arg( QDir::separator() ).arg( file_info_item->fileInfo().name() );
    emit shareBoxRequest( ID_LOCAL_USER, new_folder, false );
  }
  else
  {
    QUrl file_url = QUrl::fromLocalFile( file_info_item->fileInfo().path() );
    emit openUrlRequest( file_url );
  }
}

void GuiShareBox::onOutItemDoubleClicked( QTreeWidgetItem* item, int )
{
  if( !item )
    return;

  GuiShareBoxFileInfoItem* file_info_item = (GuiShareBoxFileInfoItem*)item;

  if( file_info_item->isFolder() )
  {
    QString new_folder;
    if( file_info_item->fileInfo().id() == ID_DOTDOT_FOLDER )
      new_folder = Bee::folderCdUp( m_outCurrentFolder );
    else if( m_outCurrentFolder.isEmpty() )
      new_folder = QString( "%1" ).arg( file_info_item->fileInfo().name() );
    else
      new_folder = QString( "%1%2%3" ).arg( m_outCurrentFolder ).arg( QDir::separator() ).arg( file_info_item->fileInfo().name() );
    emit shareBoxRequest( m_userId, new_folder, false );
  }
}

void GuiShareBox::selectMyShareBoxFolder()
{
  QString folder_path = FileDialog::getExistingDirectory( this,
                                                          tr( "%1 - Select the ShareBox folder" )
                                                          .arg( Settings::instance().programName() ),
                                                                Settings::instance().shareBoxPath() );
  if( folder_path.isEmpty() )
    return;

  Settings::instance().setShareBoxPath( Bee::convertToNativeFolderSeparator( folder_path ) );
  Settings::instance().save();
  m_myCurrentFolder = "";
  updateMyBox();
}

void GuiShareBox::onShareBoxSelected( int )
{
#if QT_VERSION >= 0x050000
  VNumber current_user_id = Bee::qVariantToVNumber( mp_comboUsers->currentData() );
#else
  VNumber current_user_id = Bee::qVariantToVNumber( mp_comboUsers->itemData( mp_comboUsers->currentIndex() ) );
#endif
  if( current_user_id > 0 && current_user_id != m_userId )
  {
    m_outCurrentFolder = "";
    m_userId = current_user_id;
    mp_outBox->clearTree();
    mp_outBox->setEnabled( true );
#ifdef BEEBEEP_DEBUG
    qDebug() << "ShareBox requests list for user" << current_user_id;
#endif
    emit shareBoxRequest( current_user_id, m_outCurrentFolder, false );
  }
}

void GuiShareBox::onShareFolderUnavailable( const User& u, const QString& folder_path )
{
#ifdef BEEBEEP_DEBUG
  qDebug() << qPrintable( u.path() ) << "has not shared box folder" << qPrintable( folder_path );
#endif
  if( u.isLocal() )
  {
    QMessageBox::information( this, Settings::instance().programName(), tr( "%1: access denied." ).arg( folder_path ) );
    return;
  }

#if QT_VERSION >= 0x050000
  if( u.id() == Bee::qVariantToVNumber( mp_comboUsers->currentData() ) )
#else
  if( u.id() == Bee::qVariantToVNumber( mp_comboUsers->itemData( mp_comboUsers->currentIndex() ) ) )
#endif
  {
    mp_lOutBox->setText( tr( "%1 <b>%2</b>" ).arg( folder_path.isEmpty() ? tr( "ShareBox" ) : folder_path ).arg( tr( "is unavailable" ) ) );
    if( folder_path == m_outCurrentFolder )
    {
      mp_outBox->clearTree();
      mp_outBox->setEnabled( false );
    }
  }
}

void GuiShareBox::dropInMyBox( const QString& share_path )
{
  QStringList sl_paths = share_path.split( "\n" );
  QList<FileInfo> selected_list = mp_outBox->selectedFileInfoList();
  if( sl_paths.size() != selected_list.size() )
    qWarning() << "ShareBox (mybox) has found drop list size" << sl_paths.size() << "not equal to selected list size" << selected_list.size();

  foreach( FileInfo file_info, selected_list )
  {
#ifdef BEEBEEP_DEBUG
    QString from_path = file_info.shareFolder().isEmpty() ? file_info.name() : QString( "%1%2%3" ).arg( file_info.shareFolder() ).arg( QDir::separator() ).arg( file_info.name() );
    qDebug() << "Drop in MY sharebox the file" << file_info.name() << "->" << from_path;
#endif
    QString to_path;
    if( m_myCurrentFolder.isEmpty() )
      to_path = Bee::convertToNativeFolderSeparator( QString( "%1/%2" ).arg( Settings::instance().shareBoxPath(), file_info.name() ) );
    else
      to_path = Bee::convertToNativeFolderSeparator( QString( "%1/%2/%3" ).arg( Settings::instance().shareBoxPath(), m_myCurrentFolder, file_info.name() ) );

    emit shareBoxDownloadRequest( m_userId, file_info, to_path );
  }
}

void GuiShareBox::dropInOutBox( const QString& share_path )
{
  QStringList sl_paths = share_path.split( "\n" );
  QList<FileInfo> selected_list = mp_myBox->selectedFileInfoList();
  if( sl_paths.size() != selected_list.size() )
    qWarning() << "ShareBox (outbox) has found drop list size" << sl_paths.size() << "not equal to selected list size" << selected_list.size();

  foreach( FileInfo file_info, selected_list )
  {
    QString to_path = Bee::convertToNativeFolderSeparator( m_outCurrentFolder );
#ifdef BEEBEEP_DEBUG
    qDebug() << "Drop in OUT sharebox the file" << file_info.name() << "->" << to_path;
#endif
    emit shareBoxUploadRequest( m_userId, file_info, to_path );
  }
}

void GuiShareBox::updateUser( const User& u )
{
  if( u.isLocal() )
    return;

  int user_index = mp_comboUsers->findData( u.id() );
  if( user_index >= 0 )
  {
    if( !u.isStatusConnected() )
    {
      if( mp_comboUsers->currentIndex() == user_index )
      {
        mp_outBox->clearTree();
        m_outCurrentFolder = "";
      }
      mp_comboUsers->removeItem( user_index );
    }
    else
      mp_comboUsers->setItemText( user_index, u.name() );
  }
  else
  {
    if( u.isStatusConnected() )
      mp_comboUsers->addItem( u.name(), u.id() );
  }

  mp_comboUsers->setEnabled( mp_comboUsers->count() > 0 );
  mp_pbUpdate->setEnabled( mp_comboUsers->isEnabled() );
}

void GuiShareBox::onFileUploadCompleted( VNumber user_id, const FileInfo& fi )
{
#ifdef BEEBEEP_DEBUG
  qDebug() << "ShareBox (upload completed) update list of the folder" << qPrintable( fi.shareFolder() ) << "and user" << user_id;
#else
  Q_UNUSED( fi );
#endif
  if( m_userId == user_id && fi.shareFolder() == m_outCurrentFolder )
    updateOutBox();
}

void GuiShareBox::onFileDownloadCompleted( VNumber, const FileInfo& fi )
{
#ifdef BEEBEEP_DEBUG
  qDebug() << "ShareBox (download completed) update list of the folder" << qPrintable( fi.shareFolder() );
#else
  Q_UNUSED( fi );
#endif
  updateMyBox();
}

bool GuiShareBox::isValidNewFolderName( QTreeWidget* tw, const QString& name_to_check )
{
  QTreeWidgetItemIterator it( tw );
  while( *it )
  {
    if( (*it)->text( GuiShareBoxFileInfoItem::ColumnFile ).toLower() == name_to_check.toLower() )
      return false;
    ++it;
  }
  return true;
}

void GuiShareBox::createFolderInBox( VNumber user_id )
{
  if( user_id == ID_INVALID )
    return;

  QTreeWidget* tw = user_id == ID_LOCAL_USER ? mp_myBox : mp_outBox;
  if( !tw->isEnabled() )
    return;

  QString current_folder = user_id == ID_LOCAL_USER ? m_myCurrentFolder : m_outCurrentFolder;

  QString folder_to_create = QInputDialog::getText( tw, Settings::instance().programName(), tr( "Please insert the new folder name" ) );
  if( folder_to_create.isEmpty() )
    return;

  if( !isValidNewFolderName( tw, folder_to_create ) )
  {
    QMessageBox::information( tw, Settings::instance().programName(), tr( "%1 already exists." ).arg( folder_to_create ) );
    return;
  }

  QString new_folder_path = current_folder.isEmpty() ? folder_to_create : QString( "%1%2%3" ).arg( current_folder ).arg( QDir::separator() ).arg( folder_to_create );
  emit shareBoxRequest( user_id, new_folder_path, true );
}

void GuiShareBox::createFolderInMyBox()
{
  createFolderInBox( ID_LOCAL_USER );
}

void GuiShareBox::createFolderInOutBox()
{
  createFolderInBox( m_userId );
}

void GuiShareBox::openMyBox()
{
  QString folder_path = m_myCurrentFolder.isEmpty() ? Settings::instance().shareBoxPath() : QString( "%1%2%3" ).arg( Settings::instance().shareBoxPath() ).arg( QDir::separator() ).arg( m_myCurrentFolder );
  if( folder_path.isEmpty() )
  {
    mp_pbMyOpenFolder->setEnabled( false );
    return;
  }
  emit openUrlRequest( QUrl::fromLocalFile( folder_path ) );
}

void GuiShareBox::updateOutBoxToBaseFolder()
{
  mp_pbUpdate->setEnabled( false );
  if( mp_comboUsers->count() > 0 )
  {
    m_userId = ID_INVALID;
    QTimer::singleShot( 10000, this, SLOT( enableUpdateButton() ) );
    onShareBoxSelected( mp_comboUsers->currentIndex() );
  }
}
