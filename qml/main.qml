import QtQuick 2.14
import QtQuick.Window 2.14
import Dashboard 1.0

Window {
    id: root
    visible: true
    width: 1280
    height: 720
    color: "#080b0d"
    title: "Qt 5.14 OpenGL Dashboard"

    property real speed: 0
    property real targetSpeed: 130
    property real battery: 77
    property real demoTime: 0
    property real distance: 0
    property real roadCurve: 0
    property real targetCurve: 0
    property real curveDirection: 1
    property real curveStrength: 0.72
    property real sceneTime: 0
    property real sceneDuration: 0
    property string sceneMode: "straightFast"
    property real carProgress: (distance * 0.008) % 1.0
    property real carEase: carProgress * carProgress * (3.0 - 2.0 * carProgress)
    property real carRoadOffset: roadCurve * Math.pow(carEase, 1.55) * roadView.width * 0.19
    property real carTurnAngle: roadCurve * (10 + carEase * 7)

    Component.onCompleted: beginScene("straightFast")

    function randomRange(minValue, maxValue) {
        return minValue + Math.random() * (maxValue - minValue)
    }

    function beginScene(mode) {
        sceneMode = mode
        sceneTime = 0

        if (mode === "straightFast") {
            sceneDuration = randomRange(2.8, 4.2)
            targetSpeed = randomRange(138, 168)
            targetCurve = 0
        } else if (mode === "straightBrake") {
            sceneDuration = randomRange(1.7, 2.4)
            targetSpeed = randomRange(62, 86)
            targetCurve = 0
        } else if (mode === "straightRecover") {
            sceneDuration = randomRange(2.0, 2.8)
            targetSpeed = randomRange(142, 176)
            targetCurve = 0
        } else if (mode === "curveIn") {
            sceneDuration = randomRange(2.2, 3.1)
            curveDirection = Math.random() > 0.5 ? 1 : -1
            curveStrength = randomRange(0.58, 0.86)
            targetSpeed = randomRange(54, 78)
            targetCurve = curveDirection * curveStrength
        } else if (mode === "curveHold") {
            sceneDuration = randomRange(1.5, 2.4)
            targetSpeed = randomRange(48, 72)
            targetCurve = curveDirection * curveStrength
        } else if (mode === "curveOut") {
            sceneDuration = randomRange(2.2, 3.0)
            targetSpeed = randomRange(132, 164)
            targetCurve = 0
        }
    }

    function advanceScene() {
        if (sceneMode === "straightFast") {
            beginScene(Math.random() < 0.30 ? "straightBrake" : "curveIn")
        } else if (sceneMode === "straightBrake") {
            beginScene("straightRecover")
        } else if (sceneMode === "straightRecover") {
            beginScene("curveIn")
        } else if (sceneMode === "curveIn") {
            beginScene("curveHold")
        } else if (sceneMode === "curveHold") {
            beginScene("curveOut")
        } else {
            beginScene("straightFast")
        }
    }

    function approach(currentValue, targetValue, rate, dt) {
        var factor = 1.0 - Math.exp(-rate * dt)
        return currentValue + (targetValue - currentValue) * factor
    }

    Timer {
        id: ticker
        interval: 16
        repeat: true
        running: true
        onTriggered: {
            var dt = interval / 1000.0
            root.demoTime += dt
            root.sceneTime += dt

            if (root.sceneTime >= root.sceneDuration) {
                root.advanceScene()
            }

            root.speed = root.approach(root.speed, root.targetSpeed, 1.35, dt)
            root.roadCurve = root.approach(root.roadCurve, root.targetCurve, 1.55, dt)
            root.distance += root.speed * dt
            gauges.requestPaint()
            frameDecor.requestPaint()
        }
    }

    Canvas {
        id: frameDecor
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.globalAlpha = 1.0
            ctx.lineCap = "butt"

            ctx.fillStyle = "#111518"
            ctx.fillRect(0, 0, width, height)

            var grad = ctx.createRadialGradient(width * 0.50, height * 0.50, 60, width * 0.50, height * 0.50, width * 0.68)
            grad.addColorStop(0.0, "#1b2426")
            grad.addColorStop(1.0, "#07090a")
            ctx.fillStyle = grad
            ctx.fillRect(0, 0, width, height)

            ctx.strokeStyle = "#11e5bd"
            ctx.lineWidth = 12
            ctx.globalAlpha = 0.92
            ctx.beginPath()
            ctx.moveTo(0, height * 0.56)
            ctx.quadraticCurveTo(width * 0.04, height * 0.92, width * 0.18, height * 0.93)
            ctx.lineTo(width * 0.82, height * 0.93)
            ctx.quadraticCurveTo(width * 0.96, height * 0.92, width, height * 0.56)
            ctx.stroke()
            ctx.globalAlpha = 1.0
        }
    }

    DashboardRoadView {
        id: roadView
        width: parent.width * 0.34
        height: parent.height * 0.43
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height * 0.12
        speed: root.speed
        distance: root.distance
        curve: root.roadCurve
    }

    VehicleIcon {
        id: nativeVehicleIcon
        width: roadView.width * 0.20
        height: width * 1.18
        x: roadView.x + roadView.width * 0.5 + root.carRoadOffset - width * 0.5
        y: roadView.y + roadView.height * (0.74 - root.carEase * 0.38) - height * 0.5
        z: roadView.z + 2
        scale: 1.08 - root.carEase * 0.34
        rotation: root.carTurnAngle
        transformOrigin: Item.Center
        antialiasing: true
        smooth: true
    }

    Canvas {
        id: gauges
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.globalAlpha = 1.0
            ctx.lineCap = "butt"

            drawBattery(ctx)
            drawSpeedGauge(ctx)
            drawRightInfo(ctx)
            drawLeftIcons(ctx)
            drawBottomInfo(ctx)
        }

        function drawArc(ctx, cx, cy, radius, startDeg, endDeg, width, color) {
            ctx.strokeStyle = color
            ctx.lineWidth = width
            ctx.lineCap = "round"
            ctx.beginPath()
            ctx.arc(cx, cy, radius, startDeg * Math.PI / 180, endDeg * Math.PI / 180, false)
            ctx.stroke()
        }

        function drawBattery(ctx) {
            var cx = width * 0.28
            var cy = height * 0.33
            var r = height * 0.16

            drawArc(ctx, cx, cy, r, 150, 390, 14, "rgba(0, 255, 213, 0.22)")
            drawArc(ctx, cx, cy, r, 150, 150 + 240 * root.battery / 100, 14, "#12f1cc")

            ctx.fillStyle = "#f7f7ed"
            ctx.textAlign = "center"
            ctx.font = "42px serif"
            ctx.fillText(Math.round(root.battery) + "%", cx, cy - 8)
            ctx.font = "22px monospace"
            ctx.fillText("Battery charge", cx, cy + 34)
        }

        function drawSpeedGauge(ctx) {
            var cx = width * 0.51
            var cy = height * 0.20
            var r = height * 0.18

            drawArc(ctx, cx, cy, r + 20, 125, 415, 46, "#ec1535")
            drawArc(ctx, cx, cy, r - 4, 130, 410, 3, "rgba(255,255,255,0.35)")

            ctx.save()
            ctx.translate(cx, cy)
            ctx.strokeStyle = "#f4f4ee"
            ctx.lineCap = "butt"
            for (var i = 0; i <= 24; ++i) {
                var deg = 130 + i * (280 / 24)
                var rad = deg * Math.PI / 180
                var inner = i % 2 === 0 ? r - 28 : r - 15
                var outer = r + 2
                ctx.lineWidth = i % 2 === 0 ? 3 : 2
                ctx.beginPath()
                ctx.moveTo(Math.cos(rad) * inner, Math.sin(rad) * inner)
                ctx.lineTo(Math.cos(rad) * outer, Math.sin(rad) * outer)
                ctx.stroke()
            }
            ctx.restore()
        }

        function drawRightInfo(ctx) {
            var x = width * 0.76
            var y = height * 0.17
            ctx.textAlign = "left"
            ctx.fillStyle = "#f2f2e8"
            ctx.font = "22px serif"
            ctx.fillText("188 KM", x + 54, y)
            ctx.font = "16px serif"
            ctx.fillText("Distance", x + 54, y + 24)

            ctx.strokeStyle = "#13f2cd"
            ctx.lineWidth = 5
            ctx.beginPath()
            ctx.moveTo(x, y + 12)
            ctx.lineTo(x + 12, y - 36)
            ctx.moveTo(x + 30, y + 12)
            ctx.lineTo(x + 18, y - 36)
            ctx.stroke()

            ctx.fillStyle = "#f2f2e8"
            ctx.font = "22px serif"
            ctx.fillText("34 mpg", x + 54, y + 82)
            ctx.font = "16px serif"
            ctx.fillText("Avg. Fuel Usage", x + 54, y + 106)

            ctx.font = "22px serif"
            ctx.fillText("78 mph", x + 54, y + 164)
            ctx.font = "16px serif"
            ctx.fillText("Avg. Speed", x + 54, y + 188)
        }

        function drawLeftIcons(ctx) {
            var x = width * 0.06
            var y = height * 0.12
            ctx.fillStyle = "#61ff43"
            ctx.font = "30px sans-serif"
            ctx.fillText("AUTO", x, y - 35)
            ctx.fillText("D", x, y + 28)
            ctx.fillStyle = "rgba(255,255,255,0.70)"
            ctx.fillText("O", x + 4, y + 88)
        }

        function drawBottomInfo(ctx) {
            ctx.textAlign = "left"
            ctx.fillStyle = "#f0f0e7"
            ctx.font = "22px serif"
            ctx.fillText("100.6  F", width * 0.16, height * 0.80)

            ctx.fillStyle = "#ed1130"
            ctx.fillRect(width * 0.25, height * 0.785, width * 0.085, 14)

            ctx.fillStyle = "#f0f0e7"
            ctx.fillText(Math.round(root.speed) + " MPH", width * 0.35, height * 0.80)

            ctx.textAlign = "center"
            ctx.fillStyle = "#35ff55"
            ctx.font = "22px serif"
            ctx.fillText("READY", width * 0.78, height * 0.78)
            ctx.fillStyle = "#d8d8d0"
            ctx.fillText("P  R  N  D", width * 0.84, height * 0.78)
        }
    }

    Text {
        id: speedText
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.145
        text: Math.round(root.speed)
        color: "#10f0cd"
        font.pixelSize: 62
        font.family: "serif"
        horizontalAlignment: Text.AlignHCenter
    }

    Text {
        anchors.horizontalCenter: speedText.horizontalCenter
        y: speedText.y + speedText.height - 2
        text: "MPH"
        color: "#10f0cd"
        font.pixelSize: 28
        font.family: "serif"
    }
}
