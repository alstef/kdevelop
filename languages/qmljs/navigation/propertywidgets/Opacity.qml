/*************************************************************************************
 *  Copyright (C) 2013 by Sven Brauch <svenbrauch@gmail.com>                         *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/
import QtQuick 2.2
import QtQuick.Controls 1.2 as QtControls

PropertyWidget {
    id: root
    width: 120
    height: 120
    value: (slider.value / 100).toFixed(2)
    onInitialValueChanged: slider.value = parseFloat(root.initialValue) * 100.0

    Text {
        z: 20
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: i18n("Opacity of an item")
        color: "white"
        opacity: 0.8
    }
    QtControls.Slider {
        id: slider
        anchors.top: parent.top
        anchors.left: parent.left
        maximumValue: 100
        width: 100
    }
    Rectangle {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -8
        anchors.horizontalCenterOffset: -8
        color: "#1f61a8"
        width: 45
        height: 45
        opacity: parent.value
        z: 1
    }
    Rectangle {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 8
        anchors.horizontalCenterOffset: 8
        color: "#a8002f"
        width: 45
        height: 45
        opacity: 1
        z: 0
    }
}
