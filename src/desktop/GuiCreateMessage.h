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

#ifndef BEEBEEP_GUICREATEMESSAGE_H
#define BEEBEEP_GUISENDMESSAGE_H

#include "ui_GuiCreateMessage.h"


class GuiCreateMessage : public QDialog, private Ui::GuiCreateMessageWidget
{
  Q_OBJECT

public:
  GuiCreateMessage( QWidget* parent = 0 );

  inline QString message();
  inline const QList<VNumber>& toChatIdList() const;

protected slots:
  void sendMessage();

protected:
  void updateRecipients();

private:
  QList<VNumber> m_toChatIdList;


};


// Inline Functions
inline QString GuiCreateMessage::message() { return mp_teMessage->message(); }
inline const QList<VNumber>& GuiCreateMessage::toChatIdList() const { return m_toChatIdList; }

#endif // BEEBEEP_GUICREATEMESSAGE_H
