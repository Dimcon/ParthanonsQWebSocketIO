#ifndef CUSTOMWEBSOCKET_H
#define CUSTOMWEBSOCKET_H

#include <QObject>
#include <QQuickItem>
#include <QWidget>
#include <QtWebSockets/QWebSocket>
#include <QtGui>

// Created by Daimon Sewell - Daimonsewell@gmail.com

class CustomWebSocket : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString URL READ getUrl WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString Connected READ isConnected NOTIFY connectionChanged)
public:
    explicit CustomWebSocket(QObject *parent = nullptr): QObject(parent) {
        qDebug() << "WebSocket client";
        connect(&socket, &QWebSocket::connected, this, &CustomWebSocket::handleConnect);
        connect(&socket, &QWebSocket::disconnected, this, &CustomWebSocket::handleDisconnect);
        connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            [=](QAbstractSocket::SocketError error){
            qDebug() << "Connection died because: " << error;
            this->retryConnect(error);
        });
        connect(&socket, &QWebSocket::textMessageReceived, this, &CustomWebSocket::handleMessageReceived);
        connect(timer, &QTimer::timeout, this, &CustomWebSocket::updateSendQueue);
        timer->start(retryDelay);
    }

signals:
    void connectionEstablished();
    void disconnect();
    void error();
    void messageReceived(QString event, QString message);
    void urlChanged();
    void connectionChanged();

public slots:
    //C++
//    Q_INVOKABLE void getData(QJSValue value) {
//        if (value.isCallable()) {
//            QJSValueList args;
//            args << QJSValue("Hello, world!");
//            value.call(args);
//        }
//    }

    //QML
//    action.getData(function (data){console.log(data)});
    bool isConnected() {
        return bisConnected;
    }
    void setUrl(QString url) {
        this->DisconnectFromServer();
        this->connectTo(url);
    }
    void connectTo(QString url) {
        this->URL = url;
        socket.open(QUrl(this->URL));
    }
    void retryConnect(QAbstractSocket::SocketError error) {
        qDebug() << error;
        handleDisconnect();
    }
    void DisconnectFromServer() {
        socket.close();
    }
    void handleDisconnect() {
        bisConnected = false;
        emit disconnect();
        emit connectionChanged();
        socket.close();
        socket.open(QUrl(this->URL));
    }
    void handleConnect() {
        bisConnected = true;
        emit connectionEstablished();
        emit connectionChanged();
    }
    void handleMessageReceived(QString message) {
        qDebug() << "CPP: [<-]" << message;
        QString event = message.mid(0,message.indexOf(";"));
        QString msg = message.right(message.length() - message.indexOf(";") - 1);
        if (messageHandlers.contains(event)) {
            QList<QJSValue> functions = ((QList<QJSValue>)messageHandlers.value(event));
            for (QJSValue function: functions) {
                QJSValueList args;
                args << QJSValue(msg);
                function.call(args);
            }
        }
        emit messageReceived(event, msg);
    }
    void on(QString callString, QJSValue function) {
        if (!messageHandlers.contains(callString)) {
            QList<QJSValue> tmp = {};
            messageHandlers.insert(callString,tmp);
        }
        QList<QJSValue> handlers = messageHandlers.value(callString);
        handlers.push_back(function);
        messageHandlers.insert(callString, handlers);
    }

    void send(QString endpoint, QString data, QJSValue onComplete) {
        // TODO: This doesn't take the endpoint into consideration. Needs to be adjusted.
        qDebug() << "CPP: [->]" << endpoint;
        if (bisConnected) {
            qint64 bytesSent = socket.sendTextMessage(endpoint + ";" + data);
        } else {
            events += 1;
            QString key = QString::number(events);
            outBoundEvent.insert(key, endpoint);
            outBoundData.insert(key, data);
            outBoundOnCompleteFunctions.insert(key, onComplete);
            outBoundRetryCounters.insert(key, 0);
        }
        // TODO: Needs tweaking

            // UTF8 is the wrong format. This doesn't check how many bytes were sent properly
//        qint64 bytes = data.toUtf8().size();
//        if (bytesSent != bytes) didFail = true;

//        if (onComplete.isCallable()) {
//            QJSValueList args;
//            args << QJSValue((didFail)? "Failed":"Success");
//            isFailing = !didFail;
//            onComplete.call(args);
//        }
    }

    void updateSendQueue() {
        if(outBoundData.size() > 0)
            qDebug() << "C++: Retrying [->] queue.length =" << outBoundData.size();
        QMapIterator<QString, QString> i(outBoundEvent);
        while (i.hasNext()) {
            i.next();
            QString key = i.key();
            QString endpoint = outBoundEvent.value(key);
            QString data = outBoundData.value(key);
            QJSValue onComplete = outBoundOnCompleteFunctions.value(key);
            qDebug() << "C++: [->]: " << endpoint << "";
            int retryCount = outBoundRetryCounters.value(key);
            outBoundRetryCounters.insert( key, retryCount + 1);
            bool didFail = false;
            if (bisConnected) {
                qint64 bytesSent = socket.sendTextMessage(endpoint + ";" + data);
            } else {
                didFail = true;
            }
//          if (bytesSent != data.toUtf8().size()) didFail = true;
//          qDebug() << "Cpp -> Success?: " << !didFail;
            if ((!didFail) || retryCount >= retryLimit) {
                if (retryCount >= retryLimit){
                    qDebug() << "C++: [->  Timed out]";
                }
                if (onComplete.isCallable()) {
                    QJSValueList args;
                    args << QJSValue((didFail)? "Failed":"Success");
                    isFailing = !didFail;
                    onComplete.call(args);
                }
                outBoundEvent.remove(key);
                outBoundData.remove(key);
                outBoundRetryCounters.remove(key);
                outBoundOnCompleteFunctions.remove(key);
                break;
            }
        }
    }

private:
    QTimer *timer = new QTimer();
    const int retryLimit = 5;
    const int retryDelay = 3000;
    QWebSocket socket;
    bool isFailing = false;
    volatile bool bisConnected = false;
    QMap<QString, QList<QJSValue>> messageHandlers;
    long events = 0;
    QMap<QString, QString> outBoundEvent;
    QMap<QString, QString> outBoundData;
    QMap<QString, QJSValue> outBoundOnCompleteFunctions;
    QMap<QString, int> outBoundRetryCounters;
    QString URL;
    QString getUrl() {
        return URL;
    }



};

#endif // CUSTOMWEBSOCKET_H
