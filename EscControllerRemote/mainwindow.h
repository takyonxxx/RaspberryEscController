#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QString>
#include <QDebug>
#include <QtWidgets>
#include <qregularexpression.h>
#include "message.h"
#include "bluetoothclient.h"


#if defined (Q_OS_ANDROID)
const QVector<QString> permissions({"android.permission.BLUETOOTH",
                                    "android.permission.BLUETOOTH_ADMIN"});
#endif

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private slots:
    void DataHandler(QByteArray data);
    void sendCommand(uint8_t command, uint8_t value);    
    void sendString(uint8_t command, QByteArray value);
    void requestData(uint8_t command);
    void changedState(BluetoothClient::bluetoothleState state);
    void statusChanged(const QString &status);

    void on_ConnectClicked();
    void on_Exit();

    void on_scrollPwmR_sliderMoved(int position);

    void on_scrollPwmL_sliderMoved(int position);

signals:
    void connectToDevice(int i);

private:
    void initButtons();   
    void createMessage(uint8_t msgId, uint8_t rw, QByteArray payload, QByteArray *result);
    void parseMessage(QByteArray *data, uint8_t &command, QByteArray &value,  uint8_t &rw);

    int remoteConstant;

    Ui::MainWindow *ui;
    QList<QString> m_qlFoundDevices;
    BluetoothClient *m_bleConnection{};
    Message message;

};

#endif // MAINWINDOW_H
