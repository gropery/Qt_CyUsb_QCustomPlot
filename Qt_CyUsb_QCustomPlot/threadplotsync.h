#ifndef THREADPLOTSYNC_H
#define THREADPLOTSYNC_H

#include <QThread>
#include <QTimer>
#include "plot.h"

//USB必需添加的头文件，需要调用系统的库函数
#include <windows.h>
#include "CyUsb/inc/CyAPI.h"
//USB必需添加的库，否则不能运行
#pragma comment(lib,"SetupAPI.lib")
#pragma comment(lib,"User32.lib")
#pragma comment(lib,"legacy_stdio_definitions.lib")

#define FIND_HEADER1      1
#define FIND_HEADER2      2
#define CHECK_FUNWORD     3
#define CHECK_DATALEN     4
#define TAKE_DATA         5
#define CHECK_CRC         6

class ThreadPlotSync : public QThread
{
    Q_OBJECT
public:
    ThreadPlotSync();
    ~ThreadPlotSync();
    void stopThread(); //结束线程
    bool setBulkEpt(CCyUSBEndPoint *eptin, CCyUSBEndPoint *eptout, long transferSize, int msec);

    //loop过程中in阶段
    long m_inBytes=0;
    long m_inSeccesses=0;
    long m_inFailures=0;

    //loop过程中out阶段
    long m_outBytes=0;
    long m_outSeccesses=0;
    long m_outFailures=0;

    //loop总过程
    long m_plotSeccesses=0;
    long m_plotFailures=0;

    void startTimer(int msec);
    void stopTimer();
    void showWave();

protected:
    void run() Q_DECL_OVERRIDE;

private:
    bool m_stop=false;                  //停止线程
    long m_totalTransferSize;           //单次传输数据量，为了更方便测试，这里等于1 burstlen
    ULONG m_timeoutPerTransferMilliSec;

    CCyUSBEndPoint  *m_BulkInEpt;
    CCyUSBEndPoint  *m_BulkOutEpt;

    // 仿真波形生成-定时器
    QTimer *timerWaveGene;

    //-----------------------
    QByteArray baRecvDataBuf;   //接收数据流暂存
    int STATE=FIND_HEADER1;     //接受数据处理状态机
    char funWord=0;             //协议-功能字
    char dataLen=0;             //协议-有效数据长度
    char dataLenCnt=0;          //协议-有效数据长度计数
    char crc=0;                 //协议-校验

    //-----------------------
    // 波形绘图窗口
    Plot *plot = NULL;// 必须初始化为空，否则后面NEW判断的时候会异常结束

    //协议处理函数
    void processRecvProtocol(QByteArray *baRecvData);

private slots:
    void slot_timerWaveGene_timeout();  //仿真波形定时器
};

#endif // THREADPLOTSYNC_H
