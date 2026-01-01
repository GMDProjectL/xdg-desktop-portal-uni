import QtQuick
import QtQuick.Controls.Universal 2.12
import QtQuick.Layouts
import org.kde.layershell 1.0 as LayerShell


ApplicationWindow {
    id: root

    width: 500
    height: Screen.desktopAvailableHeight
    minimumWidth: 400

    title: "Select Screen Source"

    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"

    visible: true

    signal sourceSelected(int index)
    signal cancelled()

    Universal.theme: Universal.Dark
    Universal.accent: Universal.Violet

    property alias model: listView.model

    LayerShell.Window.anchors: LayerShell.Window.AnchorRight
    LayerShell.Window.layer: LayerShell.Window.LayerOverlay
    LayerShell.Window.exclusionZone: -1
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityOnDemand

    Component.onCompleted: openAnimation()

    function openAnimation() {
        backgroundAnim.start()
        contentOpenAnim.start()
    }

    function closeAnimation() {
        closeAnim.start()
    }

    Rectangle {
        id: backgroundRect
        width: root.width
        height: root.height
        x: root.width
        color: Universal.background

        ParallelAnimation {
            id: backgroundAnim
            running: false

            NumberAnimation {
                target: backgroundRect
                property: "x"
                to: 0
                duration: 200
                easing.type: Easing.OutSine
            }

            NumberAnimation {
                target: backgroundRect
                property: "opacity"
                from: 0
                to: 1
                duration: 100
                easing.type: Easing.OutSine
            }
        }

    }

    Item {
        id: contentWrapper
        width: root.width
        height: root.height
        x: root.width
        opacity: 0

        ParallelAnimation {
            id: contentOpenAnim

            NumberAnimation {
                target: contentWrapper
                property: "x"
                to: 0
                duration: 300
                easing.type: Easing.OutCubic
            }

            NumberAnimation {
                target: contentWrapper
                property: "opacity"
                to: 1
                duration: 300
            }
        }

        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 24

            Label {
                text: "Share"
                font.pointSize: 48
                font.weight: 300
                opacity: 0.8
                leftPadding: 20
            }

            Label {
                text: "a screen or a window"
                font.pointSize: 35
                font.weight: 200
                opacity: 0.45
                leftPadding: 30
            }

            ListView {
                id: listView
                Layout.fillWidth: true
                Layout.fillHeight: true

                Layout.topMargin: 15

                clip: true
                currentIndex: 0
                focus: true

                model: sourceModel

                highlight: Rectangle {
                    color: "transparent"
                }

                delegate: ItemDelegate {
                    id: sourceDelegate
                    width: listView.width
                    height: 56
                    padding: 20

                    property bool activeAndHighlighted: listView.currentIndex == index
                    property bool isPressed: false

                    scale: isPressed ? 0.985 : 1

                    Behavior on scale {
                        NumberAnimation {
                            duration: 80
                            easing.type: Easing.OutBack
                        }
                    }

                    MouseArea {
                        id: sourceDelegateArea
                        anchors.fill: parent

                        onClicked: listView.currentIndex = index
                        onDoubleClicked: {
                            root.sourceSelected(index)
                            root.closeAnimation()
                        }

                        onPressedChanged: {
                            sourceDelegate.isPressed = sourceDelegateArea.containsPress
                        }
                        
                    }

                    background: Rectangle {
                        color: sourceDelegate.isPressed ? Universal.baseMediumColor : (!sourceDelegate.activeAndHighlighted 
                            ? (sourceDelegate.hovered 
                                ? Universal.baseLowColor 
                                : Universal.background
                            )
                            : Universal.accent
                        )
                        border.color: sourceDelegate.activeAndHighlighted ? Qt.lighter(Universal.accent) : "transparent"
                        border.width: 1
                    }

                    Label {
                        text: model.displayName
                        font.pointSize: 13
                        leftPadding: 24
                        rightPadding: 24
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                ScrollBar.vertical: ScrollBar {}

                Keys.onReturnPressed: {
                    root.sourceSelected(currentIndex)
                    root.closeAnimation()
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 8

                Button {
                    text: "Cancel"
                    font.pointSize: 13
                    padding: 12
                    onClicked: {
                        root.cancelled()
                        root.closeAnimation()
                    }
                }

                Button {
                    text: "Share to " + selectorApi.getAppDisplayName(requestAppId)
                    font.pointSize: 13
                    padding: 12
                    leftPadding: 20
                    rightPadding: 20
                    highlighted: true
                    onClicked: {
                        root.sourceSelected(listView.currentIndex)
                        root.closeAnimation()
                    }
                }
            }
        }
    }

    ParallelAnimation {
        id: closeAnim

        NumberAnimation {
            target: backgroundRect
            property: "x"
            to: root.width
            duration: 250
            easing.type: Easing.InCubic
        }

        NumberAnimation {
            target: contentWrapper
            property: "x"
            to: root.width
            duration: 250
            easing.type: Easing.InCubic
        }

        NumberAnimation {
            target: contentWrapper
            property: "x"
            to: contentWrapper.width
            duration: 250
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            target: contentWrapper
            property: "opacity"
            to: 0
            duration: 250
        }

        onFinished: root.close()
    }
}
