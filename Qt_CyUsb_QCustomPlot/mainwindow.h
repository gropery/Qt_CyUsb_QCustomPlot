#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QThread>
#include "threadoutsync.h"
#include "threadinsync.h"
#include "threadoutasync.h"
#include "threadinasync.h"
#include "threadloopsync.h"
#include "threadplotsync.h"

//USB必需添加的头文件，需要调用系统的库函数
#include <windows.h>
#include "CyUsb/inc/CyAPI.h"
//USB必需添加的库，否则不能运行
#pragma comment(lib,"SetupAPI.lib")
#pragma comment(lib,"User32.lib")
#pragma comment(lib,"legacy_stdio_definitions.lib")

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_comboBoxDevices_currentIndexChanged(int index);

    void on_pushButtonOutSyncContinue_clicked();    //同步连续输出
    void on_pushButtonOutSyncOnce_clicked();        //同步单次输出
    void on_pushButtonInSyncContinue_clicked();     //同步连续输入
    void on_pushButtonInSyncOnce_clicked();         //同步单次输入
    void on_pushButtonOutAsyncContinue_clicked();   //异步连续输出
    void on_pushButtonInAsyncContinue_clicked();    //异步连续输入
    void on_pushButtonLoopSyncContinue_clicked();   //同步连续LOOP
    void on_checkBox_stateChanged(int arg1);        //同步连续LOOP将数据Plot
    void on_pushButtonShowWave_clicked();           //显示波形

    void slot_timerUpdateLabel_timeout();           //统计数据更新定时器槽函数

    void on_pushButtonInTxtClear_clicked();
    void on_pushButtonOutTxtClear_clicked();

private:
    Ui::MainWindow *ui;

    CCyUSBDevice	*m_CCyUSBDevice;
    CCyUSBEndPoint  *m_BulkInEpt;
    CCyUSBEndPoint  *m_BulkOutEpt;

    ThreadOutSync m_ThreadOutSync;
    ThreadInSync  m_ThreadInSync;
    ThreadOutAsync m_ThreadOutAsync;
    ThreadInAsync  m_ThreadInAsync;
    ThreadLoopSync  m_ThreadLoopSync;
    ThreadPlotSync  m_ThreadPlotSync;

    QTimer *timerUpdateLabel;

    long m_curOutBytes=0,m_lastOutByte=0,m_outRate=0;
    long m_curInBytes=0,m_lastInByte=0,m_inRate=0;
    long m_curInAsyncBytes=0,m_lastInAsyncByte=0,m_inAsyncRate=0;
    long m_curOutAsyncBytes=0,m_lastOutAsyncByte=0,m_outAsyncRate=0;

    long m_curLoopOutBytes=0,m_lastLoopOutByte=0,m_loopOutRate=0;
    long m_curLoopInBytes=0,m_lastLoopInByte=0,m_loopInRate=0;
    long m_curLoopBytes=0,m_lastLoopByte=0,m_loopRate=0;

    long m_curPlotOutBytes=0,m_lastPlotOutByte=0,m_plotOutRate=0;
    long m_curPlotInBytes=0,m_lastPlotInByte=0,m_plotInRate=0;
    long m_curPlotBytes=0,m_lastPlotByte=0,m_plotRate=0;

    BOOL SurveyExistingDevices();
    BOOL EnumerateEndpointForTheSelectedDevice();

};
#endif // MAINWINDOW_H
