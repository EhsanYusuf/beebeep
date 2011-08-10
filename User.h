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

#ifndef BEEBEEP_USER_H
#define BEEBEEP_USER_H

#include "Config.h"


class User
{
public:
  enum Status { Offline, Online, Busy, Away, NumStatus };
  User();
  User( const User& );
  explicit User( VNumber );

  User& operator=( const User& );
  inline bool operator==( const User& ) const;

  inline bool isValid() const;
  inline VNumber id() const;
  inline void setName( const QString& );
  inline const QString& name() const;
  inline void setNickname( const QString& );
  inline const QString& nickname() const;
  inline void setHostAddress( const QHostAddress& );
  inline const QHostAddress& hostAddress() const;
  inline void setStatus( int );
  inline int status() const;
  inline void setStatusDescription( const QString& );
  inline const QString& statusDescription() const;

private:
  VNumber m_id;
  QString m_name;
  QString m_nickname;
  QHostAddress m_hostAddress;
  int m_status;
  QString m_statusDescription;

};


// Inline Functions

inline bool User::operator==( const User& u ) const { return m_id == u.m_id; }
inline bool User::isValid() const { return m_id > 0 && !m_nickname.isEmpty(); }
inline VNumber User::id() const { return m_id; }
inline void User::setName( const QString& new_value ) { m_name = new_value; }
inline const QString& User::name() const { return m_name; }
inline void User::setNickname( const QString& new_value ) { m_nickname = new_value; }
inline const QString& User::nickname() const { return m_nickname; }
inline void User::setHostAddress( const QHostAddress& new_value ) { m_hostAddress = new_value; }
inline const QHostAddress& User::hostAddress() const { return m_hostAddress; }
inline void User::setStatus( int new_value ) { m_status = new_value; }
inline int User::status() const { return m_status; }
inline void User::setStatusDescription( const QString& new_value ) { m_statusDescription = new_value; }
inline const QString& User::statusDescription() const { return m_statusDescription; }

#endif // BEEBEEP_USER_H
