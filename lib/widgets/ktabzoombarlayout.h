/***************************************************************************
 *   Copyright (C) 2001-2003                                               *
 *   The KDevelop Team                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KTABZOOMBAR_LAYOUT_H__
#define __KTABZOOMBAR_LAYOUT_H__

#include <qlayout.h>
#include <qptrlist.h>

#include "ktabzoomposition.h"


class KTabZoomBarLayout : public QLayout
{
public:
	
  KTabZoomBarLayout(QWidget *parent, KTabZoomPosition::Position pos);
  KTabZoomBarLayout(QLayout *parent, KTabZoomPosition::Position pos);
  KTabZoomBarLayout(KTabZoomPosition::Position pos);
  ~KTabZoomBarLayout();

  void addItem(QLayoutItem *item);
  QSize sizeHint() const;
  QSize minimumSize() const;
  QLayoutIterator iterator();
  void setGeometry(const QRect &rect);
  bool hasHeightForWidth () const;
  int heightForWidth(int) const;

private:

  int completeSize() const;

  KTabZoomPosition::Position m_pos;
  QPtrList<QLayoutItem> m_list;
  int m_fontHeight;

};


#endif

