import QtQuick
import QtQuick.Controls.Universal 2.12
import QtQuick.Layouts
import UniQmlTk

BaseSidebarShellOverlay {
    id: root

    width: 500
    minimumWidth: 400

    titleText: "Share"
    subtitleText: "a screen or a window"

    signal sourceSelected(int index)
    signal cancelled()

    property var model

    mainContent: ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 25

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
                    text: (model.type == 0 ? "[Monitor] " : "") + model.displayName
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
            Layout.topMargin: 16
            spacing: 8

            UniButton {
                text: "Cancel"
                onClicked: {
                    root.cancelled()
                    root.closeAnimation()
                }
            }

            UniButton {
                text: "Share to " + selectorApi.getAppDisplayName(requestAppId)
                highlighted: true
                onClicked: {
                    root.sourceSelected(listView.currentIndex)
                    root.closeAnimation()
                }
            }
        }
    }
}
