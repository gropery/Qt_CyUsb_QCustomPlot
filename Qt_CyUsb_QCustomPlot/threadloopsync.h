#ifndef THREADLOOPSYNC_H
#define THREADLOOPSYNC_H

#include <QThread>

//USB必需添加的头文件，需要调用系统的库函数
#include <windows.h>
#include "CyUsb/inc/CyAPI.h"
//USB必需添加的库，否则不能运行
#pragma comment(lib,"SetupAPI.lib")
#pragma comment(lib,"User32.lib")
#pragma comment(lib,"legacy_stdio_definitions.lib")

class ThreadLoopSync : public QThread
{
    Q_OBJECT
public:
    ThreadLoopSync();
    void stopThread(); //结束线程
    bool setBulkLoopEpt(CCyUSBEndPoint *eptin, CCyUSBEndPoint *eptout, long loopTransferSize, int msec);

    //loop过程中in阶段
    long m_inBytes=0;
    long m_inSeccesses=0;
    long m_inFailures=0;

    //loop过程中out阶段
    long m_outBytes=0;
    long m_outSeccesses=0;
    long m_outFailures=0;

    //loop总过程
    long m_loopBytes=0;
    long m_loopSeccesses=0;
    long m_loopFailures=0;

    //PC接收到IN数据后，再发送OUT数据至FPGA的时间，此时间由FPGA计算
    quint64 m_tPcLoop=0;
    quint64 m_tMaxPcLoop=0;
    quint64 m_tAvgPcLoop=0;

protected:
    void run() Q_DECL_OVERRIDE;

private:
    bool m_stop=false;                      //停止线程
    long m_totalLoopTransferSize;           //单次传输数据量，为了更方便测试，这里等于1 burstlen
    ULONG m_timeoutPerTransferMilliSec;

    CCyUSBEndPoint  *m_BulkInEpt;
    CCyUSBEndPoint  *m_BulkOutEpt;
};

#endif // THREADLOOPSYNC_H
