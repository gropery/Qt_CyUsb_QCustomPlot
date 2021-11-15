#ifndef THREADINASYNC_H
#define THREADINASYNC_H

#include <QThread>

//USB必需添加的头文件，需要调用系统的库函数
#include <windows.h>
#include "CyUsb/inc/CyAPI.h"
//USB必需添加的库，否则不能运行
#pragma comment(lib,"SetupAPI.lib")
#pragma comment(lib,"User32.lib")
#pragma comment(lib,"legacy_stdio_definitions.lib")

class ThreadInAsync : public QThread
{
    Q_OBJECT
public:
    ThreadInAsync();
    void stopThread(); //结束线程
    bool setBulkInEpt(CCyUSBEndPoint *ept, long inTransferSize, int msec, int queueSize);

    long m_inBytes=0;
    long m_inSeccesses=0;
    long m_inFailures=0;

protected:
    void run() Q_DECL_OVERRIDE;

private:
    bool m_stop=false; //停止线程
    long m_totalInTransferSize;
    ULONG m_timeoutPerTransferMilliSec;
    int m_queueSize;

    CCyUSBEndPoint  *m_BulkInEpt;

    void abortXferLoop(int pending, PUCHAR *buffers, PUCHAR *contexts, OVERLAPPED *inOvLap);

};

#endif // THREADINASYNC_H
