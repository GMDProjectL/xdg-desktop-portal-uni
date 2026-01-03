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
    signal cancelled

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

            Component.onCompleted: {
                listView.forceActiveFocus()
            }

            keyNavigationEnabled: true
            keyNavigationWraps: true

            model: sourceModel

            highlight: Rectangle {
                color: "transparent"
            }

            delegate: PressableListDelegate {
                id: sourceDelegate
                width: listView.width
                height: 56
                padding: 20

                activeAndHighlighted: listView.currentIndex == index

                onClicked: listView.currentIndex = index
                onDoubleClicked: {
                    root.sourceSelected(index)
                    root.closeAnimation()
                }

                contentItem: UniLabel {
                    text: (model.type == 0 ? "[Monitor] " : "") + model.displayName
                    font.pointSize: 13
                    leftPadding: 20
                    rightPadding: 24
                    animDelay: 100 + (index * 70)
                    animDuration: 200
                    verticalAlignment: Text.AlignVCenter
                }
            }

            ScrollBar.vertical: ScrollBar {}

            Keys.onReturnPressed: {
                root.sourceSelected(currentIndex)
                root.closeAnimation()
            }

            Keys.onEscapePressed: {
                root.cancelled()
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
