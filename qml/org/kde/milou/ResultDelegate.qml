/*
 * This file is part of the KDE Milou Project
 * SPDX-FileCopyrightText: 2013-2014 Vishesh Handa <me@vhanda.in>
 * SPDX-FileCopyrightText: 2015-2016 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 */

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents3
import org.kde.milou as Milou
import org.kde.ksvg 1.0 as KSvg

PlasmaComponents3.ItemDelegate {
    id: resultDelegate

    // Altezza della card: maggiore per i risultati MPRIS.
    implicitHeight: resultDelegate.isMpris
    ? Kirigami.Units.gridUnit * 11
    : labelWrapper.implicitHeight + Kirigami.Units.mediumSpacing * 2

    down: tapHandler.pressed
    hoverEnabled: true

    readonly property int indexModifier: reversed ? 0 : 1
    property bool reversed: false

    readonly property bool isCurrent: ListView.isCurrentItem

    readonly property bool sectionHasChanged:
    (reversed && ListView.section != ListView.nextSection)
    || (!reversed && ListView.section != ListView.previousSection)

    property int activeAction: -1

    property string typeText: sectionHasChanged ? ListView.section : ""

    // Identifica i risultati provenienti dal runner MPRIS.
    readonly property bool isMpris: model.category === "MPRIS"

    property int categoryWidth: Kirigami.Units.gridUnit * 10

    required property int index
    required property var model
    required property var actions

    property alias displayText: displayLabel.text

    displayText: model.display

    // Posizione corrente del brano, in microsecondi.
    property real mprisPosition: 0

    // Durata totale del brano, in microsecondi.
    property real mprisDuration: 0

    // Stato corrente del player
    property string mprisStatus: "Stopped"

    // Dati utilizzati per la barra di avanzamento.
    property var mprisProgressData: []

    // ID del risultato MPRIS, aggiornato quando KRunner sostituisce il match.
    property string mprisMatchId: isMpris
    ? String(model.id || "")
    : ""

    // Formatta i microsecondi nel formato mm:ss.
    function formatTime(microseconds) {
        if (!microseconds || microseconds < 0)
            return "00:00"

            const totalSeconds = Math.floor(microseconds / 1000000)
            const minutes = Math.floor(totalSeconds / 60)
            const seconds = totalSeconds % 60

            return (minutes < 10 ? "0" : "") + minutes + ":"
            + (seconds < 10 ? "0" : "") + seconds
    }

    // Legge posizione, durata e stato dall'ID generato dal runner C++.
    function updateProgressData() {
        if (!isMpris) {
            mprisPosition = 0
            mprisDuration = 0
            mprisStatus = "Stopped"
            return
        }

        const parts = mprisMatchId.split("|")

        if (parts.length >= 4) {
            const position = Number(parts[1])
            const duration = Number(parts[2])

            if (!isNaN(position))
                mprisPosition = position

                if (!isNaN(duration))
                    mprisDuration = duration

                    mprisStatus = parts[3]
        } else {
            mprisPosition = 0
            mprisDuration = 0
            mprisStatus = "Stopped"
        }
    }

    // Fa avanzare localmente la barra mentre il brano è in riproduzione.
    Timer {
        id: progressTimer

        interval: 250
        repeat: true
        running: resultDelegate.isMpris

        onTriggered: {
            // Se il player è in riproduzione, avanza di 250 ms.
            if (mprisStatus === "Playing" && mprisDuration > 0) {
                mprisPosition += 250000

                // Non supera mai la durata totale del brano.
                if (mprisPosition > mprisDuration)
                    mprisPosition = mprisDuration
            }
        }
    }

    onMprisProgressDataChanged: updateProgressData()
    onMprisMatchIdChanged: updateProgressData()

    Component.onCompleted: updateProgressData()

    Accessible.role: Accessible.ListItem

    Accessible.name: displayLabel.text

    Accessible.description: {
        var section = ListView.section

        if (!section) {
            return ""
        }

        var subtext = subtextLabel.text

        if (subtext.length > 0) {
            return i18nd(
                "milou",
                "%1, in category %2",
                subtext,
                section
            )
        } else {
            return i18nd(
                "milou",
                "in category %1",
                section
            )
        }
    }

    onIsCurrentChanged: {
        if (!isCurrent) {
            activeAction = -1
        }
    }

    function activateNextAction() {
        if (activeAction === actionsRepeater.count - 1) {
            return false
        }

        ++activeAction
        return true
    }

    function activatePreviousAction() {
        if (activeAction < 0) {
            return false
        }

        --activeAction
        return true
    }

    function activateLastAction() {
        activeAction = actionsRepeater.count - 1
    }

    onClicked: {
        resultDelegate.ListView.view.currentIndex = index
        resultDelegate.ListView.view.runCurrentIndex()
    }

    onHoveredChanged: {
        if (hovered) {
            resultDelegate.ListView.view.currentIndex = index
        }
    }

    // Gestione del trascinamento dei risultati normali.
    DragHandler {
        id: dragHandler

        parent: labelWrapper
        target: labelLayout

        grabPermissions: PointerHandler.TakeOverForbidden

        onActiveChanged: if (active) {
            typePixmap.grabToImage((result) => {
                const mimeData =
                resultDelegate.ListView.view.model.getMimeData(
                    resultDelegate.ListView.view.model.index(
                        resultDelegate.index,
                        0
                    )
                )

                if (!mimeData) {
                    return
                }

                dragHelper.Drag.imageSource = result.url
                dragHelper.Drag.mimeData =
                Milou.MouseHelper.generateMimeDataMap(mimeData)

                dragHelper.Drag.active = dragHandler.active
            })
        } else {
            dragHelper.Drag.active = false
        }
    }

    contentItem: Item {
        id: labelWrapper

        // L'altezza segue il layout MPRIS oppure quello normale.
        implicitHeight: resultDelegate.isMpris
        ? mprisLayout.implicitHeight
        : labelLayout.implicitHeight

        // QTBUG-63395: DragHandler blocks ItemDelegate's clicked signal.
        TapHandler {
            id: tapHandler

            onTapped: resultDelegate.clicked()
        }

        // Separatore usato dal layout originale di Milou.
        KSvg.SvgItem {
            width: parent.width
            height: 1

            anchors.bottom: parent.top
            anchors.bottomMargin: Kirigami.Units.smallSpacing

            imagePath: "widgets/line"
            elementId: "horizontal-line"

            visible: !resultDelegate.isMpris
            && resultDelegate.sectionHasChanged
            && !resultDelegate.isCurrent
            && resultDelegate.index !== 0
            && resultDelegate.ListView.view.currentIndex
            !== (resultDelegate.index - indexModifier)

            opacity: 0.5
        }

        // Categoria del risultato normale.
        PlasmaComponents3.Label {
            id: typeText

            anchors {
                left: parent.left
                verticalCenter: labelWrapper.verticalCenter
            }

            width: resultDelegate.categoryWidth
            - Kirigami.Units.largeSpacing

            opacity: 0.75

            color: resultDelegate.down
            ? Kirigami.Theme.highlightedTextColor
            : Kirigami.Theme.textColor

            elide: Text.ElideRight

            text: resultDelegate.typeText
            textFormat: Text.PlainText

            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter

            visible: !resultDelegate.isMpris
        }

        // =========================================================
        // Layout originale di Milou.
        // Viene utilizzato per tutti i risultati NON MPRIS.
        // =========================================================
        RowLayout {
            id: labelLayout

            anchors {
                left: parent.left
                leftMargin: resultDelegate.categoryWidth

                right: actionsRow.visible
                ? actionsRow.left
                : parent.right

                rightMargin: actionsRow.visible
                ? Kirigami.Units.smallSpacing
                : 0

                verticalCenter: parent.verticalCenter
            }

            visible: !resultDelegate.isMpris

            Kirigami.Icon {
                id: typePixmap

                Layout.preferredWidth:
                Kirigami.Units.iconSizes.smallMedium

                Layout.preferredHeight:
                Kirigami.Units.iconSizes.smallMedium

                Layout.rightMargin: Kirigami.Units.smallSpacing

                Layout.alignment: Qt.AlignVCenter

                source: model.decoration

                animated: false
                selected: resultDelegate.down
            }

            PlasmaComponents3.Label {
                id: displayLabel

                elide: Text.ElideMiddle

                wrapMode: model.multiLine
                ? Text.WordWrap
                : Text.NoWrap

                maximumLineCount: model.multiLine
                ? Infinity
                : 1

                verticalAlignment: Text.AlignVCenter

                textFormat: model.multiLine
                ? Text.StyledText
                : Text.PlainText

                color: resultDelegate.down
                ? Kirigami.Theme.highlightedTextColor
                : Kirigami.Theme.textColor

                Layout.rightMargin: Kirigami.Units.smallSpacing

                Layout.maximumWidth:
                labelLayout.width
                - typePixmap.implicitWidth

                PlasmaComponents3.ToolTip {
                    text: displayLabel.text

                    visible:
                    displayLabelHoverHandler.hovered
                    && displayLabel.truncated

                    timeout: -1
                }

                HoverHandler {
                    id: displayLabelHoverHandler
                }
            }

            PlasmaComponents3.Label {
                id: subtextLabel

                Layout.fillWidth: true

                opacity: width > 0 ? 0.75 : 0

                color: resultDelegate.down
                ? Kirigami.Theme.highlightedTextColor
                : Kirigami.Theme.textColor

                text:
                model.isDuplicate === true
                || model.isDuplicate > 1
                || resultDelegate.isCurrent
                ? String(model.subtext || "")
                : ""

                elide: Text.ElideMiddle

                wrapMode: Text.NoWrap

                maximumLineCount: 1

                verticalAlignment: Text.AlignVCenter

                textFormat: Text.PlainText

                PlasmaComponents3.ToolTip {
                    text: subtextLabel.text

                    visible:
                    subtextLabelHoverHandler.hovered
                    && subtextLabel.truncated

                    timeout: -1
                }

                HoverHandler {
                    id: subtextLabelHoverHandler
                }
            }
        }

        // =========================================================
        // NUOVO LAYOUT MPRIS.
        //
        // Tutti gli elementi sono nello stesso ColumnLayout:
        //
        //   logo
        //   titolo
        //   artista
        //   barra
        //   tempi
        //   pulsanti
        //
        // In questo modo i pulsanti non possono sovrapporsi alla barra.
        // =========================================================
        ColumnLayout {
            id: mprisLayout

            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: parent.bottom

                leftMargin: Kirigami.Units.largeSpacing
                rightMargin: Kirigami.Units.largeSpacing

                topMargin: Kirigami.Units.smallSpacing
                bottomMargin: Kirigami.Units.smallSpacing
            }

            spacing: Kirigami.Units.smallSpacing

            visible: resultDelegate.isMpris

            // Logo del player.
            Kirigami.Icon {
                id: mprisIcon

                Layout.alignment: Qt.AlignHCenter

                Layout.preferredWidth:
                Kirigami.Units.iconSizes.large

                Layout.preferredHeight:
                Kirigami.Units.iconSizes.large

                source: model.decoration

                animated: false
                selected: resultDelegate.down
            }

            // Titolo della canzone.
            PlasmaComponents3.Label {
                id: mprisTitle

                Layout.fillWidth: true

                text: String(model.display || "")

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                elide: Text.ElideRight

                maximumLineCount: 1

                textFormat: Text.PlainText

                color: resultDelegate.down
                ? Kirigami.Theme.highlightedTextColor
                : Kirigami.Theme.textColor

                font.bold: true
            }

            // Artista.
            PlasmaComponents3.Label {
                id: mprisArtist

                Layout.fillWidth: true

                text: String(model.subtext || "")

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                elide: Text.ElideRight

                maximumLineCount: 1

                textFormat: Text.PlainText

                opacity: 0.75

                color: resultDelegate.down
                ? Kirigami.Theme.highlightedTextColor
                : Kirigami.Theme.textColor
            }

            // Spazio minimo tra artista e barra.
            Item {
                Layout.preferredHeight: Kirigami.Units.smallSpacing / 2
            }

            // Barra di avanzamento.
            PlasmaComponents3.ProgressBar {
                id: progressBar

                Layout.fillWidth: true

                from: 0
                to: mprisDuration > 0
                ? mprisDuration
                : 1

                value: Math.min(
                    mprisPosition,
                    mprisDuration > 0
                    ? mprisDuration
                    : 1
                )
            }

            // Tempo corrente e durata totale.
            RowLayout {
                id: progressTimes

                Layout.fillWidth: true

                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents3.Label {
                    id: currentTimeLabel

                    Layout.fillWidth: true

                    text: formatTime(mprisPosition)

                    horizontalAlignment: Text.AlignLeft

                    opacity: 0.75
                }

                PlasmaComponents3.Label {
                    id: durationLabel

                    Layout.fillWidth: true

                    text: formatTime(mprisDuration)

                    horizontalAlignment: Text.AlignRight

                    opacity: 0.75
                }
            }

            // Spazio ridotto prima dei pulsanti.
            Item {
                Layout.preferredHeight: Kirigami.Units.smallSpacing / 2
            }

            // Pulsanti MPRIS.
            Row {
                id: mprisActionsRow

                Layout.alignment: Qt.AlignHCenter

                spacing: Kirigami.Units.largeSpacing

                visible:
                resultDelegate.isCurrent
                && actionsRepeater.count > 0

                Repeater {
                    id: mprisActionsRepeater

                    model: resultDelegate.actions

                    PlasmaComponents3.ToolButton {
                        width: height

                        height: Kirigami.Units.iconSizes.medium

                        visible: modelData.visible || true
                        enabled: modelData.enabled || true

                        Accessible.role: Accessible.Button
                        Accessible.name: modelData.text

                        checkable: checked

                        checked:
                        resultDelegate.activeAction === index

                        focus:
                        resultDelegate.activeAction === index

                        Kirigami.Icon {
                            anchors.centerIn: parent

                            implicitWidth:
                            Kirigami.Units.iconSizes.smallMedium

                            implicitHeight:
                            Kirigami.Units.iconSizes.smallMedium

                            // ToolButton non gestisce direttamente QIcon.
                            source: modelData.iconSource || ""

                            active:
                            parent.hovered
                            || parent.checked
                        }

                        PlasmaComponents3.ToolTip {
                            text: modelData.text || ""

                            visible: parent.hovered
                        }

                        onClicked:
                        resultDelegate.ListView.view.runAction(index)
                    }
                }
            }
        }

        // =========================================================
        // Azioni originali di Milou.
        // Rimangono invariate per i risultati normali.
        // =========================================================
        Row {
            id: actionsRow

            anchors.right: parent.right
            anchors.verticalCenter: labelWrapper.verticalCenter

            visible:
            !resultDelegate.isMpris
            && resultDelegate.isCurrent
            && actionsRepeater.count > 0

            Repeater {
                id: actionsRepeater

                model: resultDelegate.actions

                PlasmaComponents3.ToolButton {
                    width: height
                    height: Kirigami.Units.iconSizes.medium

                    visible: modelData.visible || true
                    enabled: modelData.enabled || true

                    Accessible.role: Accessible.Button
                    Accessible.name: modelData.text

                    checkable: checked

                    checked:
                    resultDelegate.activeAction === index

                    focus:
                    resultDelegate.activeAction === index

                    Kirigami.Icon {
                        anchors.centerIn: parent

                        implicitWidth:
                        Kirigami.Units.iconSizes.smallMedium

                        implicitHeight:
                        Kirigami.Units.iconSizes.smallMedium

                        // ToolButton cannot cope with QIcon.
                        source: modelData.iconSource || ""

                        active:
                        parent.hovered
                        || parent.checked
                    }

                    PlasmaComponents3.ToolTip {
                        text: {
                            var text = modelData.text || ""

                            if (index === 0) {
                                text = i18ndc(
                                    "milou",
                                    "placeholder is action e.g. run in terminal, in parenthesis is shortcut",
                                    "%1 (Shift+Return)",
                                              text
                                )
                            }

                            return text
                        }
                    }

                    onClicked:
                    resultDelegate.ListView.view.runAction(index)
                }
            }
        }
    }

    // Dati utilizzati per il drag & drop dei risultati.
    Item {
        id: dragHelper

        Drag.dragType: Drag.Automatic

        Drag.supportedActions:
        Qt.CopyAction | Qt.LinkAction

        Drag.onDragFinished: {
            Drag.mimeData = {}
            Drag.imageSource = ""
        }
    }
}
