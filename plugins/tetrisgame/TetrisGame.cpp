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

#include <QtPlugin>
#include <QtDebug>
#include "GuiTetris.h"
#include "TetrisGame.h"
#include "Version.h"


QString TetrisGame::name() const
{
  return tr( "Tetris" );
}

QString TetrisGame::version() const
{
  return "0.2.1";
}

QString TetrisGame::author() const
{
  return "Marco Mastroddi";
}

QString TetrisGame::help() const
{
  return tr( "You can play Tetris." );
}

QIcon TetrisGame::icon() const
{
  return QIcon( iconFileName() );
}

QString TetrisGame::iconFileName() const
{
  return QLatin1String( ":/plugins/tetris.png" );
}

int TetrisGame::priority() const
{
  return 1000;
}

QString TetrisGame::coreVersion() const
{
  return QString( BEEBEEP_VERSION );
}


QWidget* TetrisGame::mainWindow() const
{
  return mp_tetris;
}

TetrisGame::TetrisGame()
  : QObject()
{
  setEnabled( true );
  qDebug() << "Tetris game plugin loaded";
  mp_tetris = new GuiTetris();
}

Q_EXPORT_PLUGIN2( beetetris, TetrisGame )

TetrisGame::~TetrisGame()
{
  delete mp_tetris;
}

void TetrisGame::pause()
{
  mp_tetris->board()->pause();
}



