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

#ifndef BEEBEEP_EMOTICON_H
#define BEEBEEP_EMOTICON_H

#include "Config.h"


class Emoticon
{
public:
  enum Group { Unknown, Recent, Smiley, Objects, Nature, Places, Symbols, NumGroups };

  Emoticon();
  Emoticon( const QString&, const QString&, int emoticon_group = Emoticon::Unknown );
  Emoticon( const Emoticon& );

  Emoticon& operator=( const Emoticon& );
  inline bool operator==( const Emoticon& ) const;

  inline bool isValid() const;

  inline const QString& textToMatch() const;
  inline const QString& name() const;
  inline void setGroup( int );
  inline int group() const;

  inline QString toHtml( const QString& theme = "" ) const;
  inline QPixmap pixmap( const QString& theme = "" ) const;

private:
  QString m_textToMatch;
  QString m_name;
  int m_group;

};


// Inline Functions

inline bool Emoticon::operator==( const Emoticon& e ) const { return m_textToMatch == e.m_textToMatch; }
inline bool Emoticon::isValid() const { return m_textToMatch.size() > 0 && m_name.size() > 0; }
inline const QString& Emoticon::textToMatch() const { return m_textToMatch; }
inline const QString& Emoticon::name() const { return m_name; }
inline void Emoticon::setGroup( int new_value ) { m_group = new_value; }
inline int Emoticon::group() const { return m_group; }
inline QString Emoticon::toHtml( const QString& theme ) const { return QString( "<img src=':/%1/%2.png'>").arg( theme.isEmpty() ? "emoticons" : theme ).arg( m_name ); }
inline QPixmap Emoticon::pixmap( const QString& theme ) const { return QPixmap( QString( ":/%1/%2.png").arg( theme.isEmpty() ? "emoticons" : theme ).arg( m_name ) ); }


#endif // BEEBEEP_EMOTICON_H
