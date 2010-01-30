/************************************************************************
 * KDevelop4 Custom Buildsystem Support                                 *
 *                                                                      *
 * Copyright 2010 Andreas Pakulat <apaku@gmx.de>                        *
 *                                                                      *
 * This program is free software; you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation; either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful, but  *
 * WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU     *
 * General Public License for more details.                             *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, see <http://www.gnu.org/licenses/>. *
 ************************************************************************/

#ifndef CUSTOMBUILDSYSTEMCONFIGWIDGET_H
#define CUSTOMBUILDSYSTEMCONFIGWIDGET_H

#include <QWidget>
#include "custombuildsystemconfig.h"

class KConfig;

namespace Ui
{
class CustomBuildSystemConfigWidget;
}

class CustomBuildSystemConfigWidget : public QWidget
{
Q_OBJECT
public:
    CustomBuildSystemConfigWidget( QWidget* parent = 0 );
    void loadFrom( KConfig* );
    void saveTo( KConfig* );
    void loadDefaults();
signals:
    void changed();
private slots:
    void changeCurrentConfig(int);
private:
    Ui::CustomBuildSystemConfigWidget* ui;
    QList<CustomBuildSystemConfig> configs;
};

#endif