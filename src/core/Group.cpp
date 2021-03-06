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

#include "Group.h"


Group::Group()
  : m_id( ID_INVALID ), m_name( "" ), m_usersId(),
    m_privateId( "" ), m_lastModified(), m_chatType( DefaultChat )
{
}

Group::Group( const Group& g )
{
  (void)operator=( g );
}

Group& Group::operator=( const Group& g )
{
  if( this != &g )
  {
    m_id = g.m_id;
    m_name = g.m_name;
    m_usersId = g.m_usersId;
    m_privateId = g.m_privateId;
    m_lastModified = g.m_lastModified;
    m_chatType = g.m_chatType;
  }
  return *this;
}

bool Group::addUser( VNumber user_id )
{
  if( hasUser( user_id ) )
    return false;
  m_usersId.append( user_id );
  return true;
}

void Group::setUsers( const QList<VNumber>& group_members )
{
  m_usersId.clear();
  addUser( ID_LOCAL_USER );
  foreach( VNumber member_id, group_members )
    addUser( member_id );
}

bool Group::hasUsers( const QList<VNumber>& user_id_list ) const
{
  foreach( VNumber user_id, user_id_list )
  {
    if( !hasUser( user_id ) )
      return false;
  }
  return true;
}
