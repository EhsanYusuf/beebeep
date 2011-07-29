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
// $Id: BeeBeep.cpp 24 2011-07-29 08:26:19Z mastroddi $
//
//////////////////////////////////////////////////////////////////////

#include "FileTransferServer.h"


FileTransferServer::FileTransferServer( QObject *parent )
  : QTcpServer( parent )
{
}

void FileTransferServer::incomingConnection( int socketDescriptor )
{

}
