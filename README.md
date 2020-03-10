# ParthanonsQWebSocketIO
An implementation of QWebSocket that behaves similar to a SocketIO client without needing an additional library. It works well in QML.

It will retry to connect if a connection fails. It buffers outbound messages for 5 connection attempts if it fails to send. (can be changed with retryLimit variable)

It still emits the error signal from the QWebSocket, but will catch the error and try to reconnect afterwards.

Usage:

Add the class to your QML:

```
QGuiApplication app(argc, argv);
qmlRegisterType<ParthanonsQWebSocket>("ParthanonsQWebSocket",1,0,"ParthanonsQWebSocket");
const QUrl url(QStringLiteral("qrc:/main.qml"));
QQmlApplicationEngine engine;
engine.load(url);
return app.exec();
```

Use within QML main.qml:
```
ParthanonsQWebSocket {
    id: socket
    Component.onCompleted: {
        socket.on("RecieveTest", function(data) {
            console.log("GOT DATA FROM SERVER: " + data)
            let obj = JSON.parse(data);
            console.log(obj)
        })
        socket.connectTo("ws://demos.kaazing.com/echo")
    }
    onConnectionEstablished: {
        // Do connection established stuff
        statusIndicator.state = "Connected"
    }
    onDisconnect: {
        // Do Connection died stuff
        statusIndicator.state = "Disconnected"
    }
}

Item {
    MouseArea {
        onClicked: {
            data = {
                mydata: "data", 
                moredata: "moredata"
            }
            socket.send("ServerEndpoint", JSON.stringify(data), (result)=>{
                if (result == "Failed"){
                        console.log("Send timed out.")
                } else {
                        console.log("Message sent")
                }
            })
        }
    }
}
```

