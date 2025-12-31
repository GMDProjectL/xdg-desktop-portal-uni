import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: root

    width: 500
    height: 400
    minimumWidth: 400
    minimumHeight: 300
    title: "Select Screen Source"

    visible: true

    signal sourceSelected(int index)
    signal cancelled()

    property alias model: listView.model

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: "Select a screen or window to share:"
            font.pixelSize: 14
        }

        ListView {
            id: listView
            model: sourceModel
            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true
            currentIndex: 0
            focus: true

            highlight: Rectangle {
                color: palette.highlight
                radius: 4
            }
            highlightMoveDuration: 100

            delegate: ItemDelegate {
                width: listView.width
                height: 48
                padding: 20

                Label {
                    text: model.displayName
                    font.pixelSize: 13
                    font.bold: true
                }

                onClicked: listView.currentIndex = index
                onDoubleClicked: {
                    root.sourceSelected(index)
                    root.close()
                }
            }

            ScrollBar.vertical: ScrollBar {}

            Keys.onReturnPressed: {
                root.sourceSelected(currentIndex)
                root.close()
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8

            Button {
                text: "Cancel"
                onClicked: {
                    root.cancelled()
                    root.close()
                }
            }

            Button {
                text: "Select"
                highlighted: true
                onClicked: {
                    root.sourceSelected(listView.currentIndex)
                    root.close()
                }
            }
        }
    }
}
