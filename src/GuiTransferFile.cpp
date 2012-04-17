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
#include "GuiTransferFile.h"
#include "FileInfo.h"
#include "Settings.h"
#include "User.h"


GuiTransferFile::GuiTransferFile( QWidget *parent )
 : QTreeWidget( parent )
{
  setObjectName( "GuiTransferFile" );
  QStringList labels;
  labels << "" << tr( "File" ) << tr( "User" ) << tr( "Status" ) << "";
  setHeaderLabels( labels );
  setRootIsDecorated( false );
  setSortingEnabled( false );
  setColumnHidden( ColumnSort, true );
  sortItems( ColumnSort, Qt::DescendingOrder );

  QHeaderView* hv = header();
  hv->setResizeMode( ColumnCancel, QHeaderView::Fixed );
  setColumnWidth( ColumnCancel, 24 );
  hv->setResizeMode( ColumnFile, QHeaderView::ResizeToContents );
  hv->setResizeMode( ColumnUser, QHeaderView::ResizeToContents );
  hv->setResizeMode( ColumnProgress, QHeaderView::Stretch );
  hv->hide();

  connect( this, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this, SLOT( checkItemClicked( QTreeWidgetItem*, int ) ) );
}

void GuiTransferFile::setProgress( VNumber peer_id, const User& u, const FileInfo& fi, FileSizeType bytes )
{
  qDebug() << "GuiTransferFile setProgress::" << bytes << "of" << fi.size() << "bytes";

  QHeaderView* hv = header();
    if( hv->isHidden() )
      hv->show();

  QTreeWidgetItem* item = findItem( peer_id );
  if( !item )
  {
    item = new QTreeWidgetItem( this );
    item->setFirstColumnSpanned( false );
    item->setIcon( ColumnFile, fi.isDownload() ? QIcon( ":/images/download.png" ) : QIcon( ":/images/upload.png" ) );
    item->setText( ColumnFile, fi.name() );
    item->setData( ColumnFile, PeerId, peer_id );
    item->setData( ColumnFile, FileId, fi.id() );
    item->setData( ColumnFile, FilePath, fi.path() );
    item->setData( ColumnFile, TransferInProgress, true );
    item->setData( ColumnFile, TransferCompleted, false );
    item->setText( ColumnUser, u.name() );
    item->setIcon( ColumnCancel, QIcon( ":/images/disconnect.png") );
    item->setText( ColumnSort, QString( "0%1").arg( peer_id ) );
    sortItems( ColumnSort, Qt::DescendingOrder );
  }

  if( item->data( ColumnFile, TransferInProgress ).toBool() )
  {
    qDebug() << "FileTransfer in progress";
    item->setData( ColumnFile, TransferCompleted, (bool)(bytes==fi.size()) );
    item->setData( ColumnFile, TransferInProgress, (bool)(bytes<fi.size()) );
    showProgress( item, fi, bytes );
  }

  showIcon( item );
}

void GuiTransferFile::showIcon( QTreeWidgetItem* item )
{
  if( !item )
    return;

  QIcon icon;
  QString status_tip;
  if( item->data( ColumnFile, TransferCompleted ).toBool() )
  {
    icon = QIcon( ":/images/green-ball.png" );
    status_tip = tr( "Completed" );
  }
  else if( item->data( ColumnFile, TransferInProgress ).toBool() )
  {
    icon = QIcon( ":/images/disconnect.png" );
    status_tip = tr( "Cancel Transfer" );
  }
  else
  {
    icon = QIcon( ":/images/red-ball.png" );
    status_tip = tr( "Not Completed" );
  }

  item->setToolTip( ColumnCancel, status_tip );
  item->setIcon( ColumnCancel, icon );
}

QTreeWidgetItem* GuiTransferFile::findItem( VNumber peer_id )
{
  QTreeWidgetItemIterator it( this );
  while( *it )
  {
    if( Bee::qVariantToVNumber( (*it)->data( ColumnFile, PeerId ) ) == peer_id )
      return *it;
    ++it;
  }
  return 0;
}

void GuiTransferFile::showProgress( QTreeWidgetItem* item, const FileInfo& fi, FileSizeType bytes )
{
  if( fi.size() == 0 )
  {
    qWarning() << "GuiTransferFile::showProgress try to show progress divided by 0:" << fi.path();
    return;
  }

  if( item->data( ColumnFile, TransferCompleted ).toBool() )
  {
    item->setText( ColumnProgress, tr( "Transfer completed" ) );
    return;
  }

  QString file_transfer_progress = QString( "%1 %2 of %3 (%4%)" ).arg( fi.isDownload() ? tr( "Downloading") : tr( "Uploading"),
                                      Bee::bytesToString( bytes ), Bee::bytesToString( fi.size() ),
                                      QString::number( static_cast<FileSizeType>( (bytes * 100) / fi.size())) );
  item->setText( ColumnProgress, file_transfer_progress );

  file_transfer_progress.prepend( QString( "[%1] " ).arg( fi.name() ) );
  if( !isVisible() )
    emit stringToShow( file_transfer_progress, 1000 );
  qDebug() << file_transfer_progress;
}

void GuiTransferFile::setMessage( VNumber peer_id, const User& u, const FileInfo& fi, const QString& msg )
{
  qApp->processEvents();
  qDebug() << "GuiTransferFile setMessage:" << msg;
  QTreeWidgetItem* item = findItem( peer_id );
  if( !item )
    setProgress( peer_id, u, fi, 0 );
  item = findItem( peer_id );
  if( !item )
  {
    qWarning() << "Unable to find file transfer item with id" << peer_id;
    return;
  }

  item->setData( ColumnFile, TransferInProgress, false );
  item->setText( ColumnProgress, msg );
  showIcon( item );
}

void GuiTransferFile::checkItemClicked( QTreeWidgetItem* item, int col )
{
  if( !item )
  {
    qWarning() << "GuiTransferFile::checkItemClicked has the item invalid";
    return;
  }

  if( col == ColumnCancel && item->data( ColumnFile, TransferInProgress ).toBool() )
  {
    if( QMessageBox::question( this, Settings::instance().programName(), tr( "Do you really want to cancel the transfer of %1?" ).arg( item->text( ColumnFile ) ),
                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) == QMessageBox::Yes )
    {
      item->setIcon( ColumnCancel, QIcon( ":/images/red-ball.png") );
      VNumber peer_id = Bee::qVariantToVNumber( item->data( ColumnFile, PeerId ) );
      item->setData( ColumnFile, TransferInProgress, false );
      emit transferCancelled( peer_id );
    }
  }
}
