#ifndef THREADOUTASYNC_H
#define THREADOUTASYNC_H

#include <QThread>

//USB必需添加的头文件，需要调用系统的库函数
#include <windows.h>
#include "CyUsb/inc/CyAPI.h"
//USB必需添加的库，否则不能运行
#pragma comment(lib,"SetupAPI.lib")
#pragma comment(lib,"User32.lib")
#pragma comment(lib,"legacy_stdio_definitions.lib")

class ThreadOutAsync : public QThread
{
    Q_OBJECT
public:
    ThreadOutAsync();

    void stopThread(); //结束线程
    bool setBulkOutEpt(CCyUSBEndPoint *ept, long inTransferSize, int msec, int queueSize);

    long m_outBytes=0;
    long m_outSeccesses=0;
    long m_outFailures=0;

protected:
    void run() Q_DECL_OVERRIDE;

private:
    bool m_stop=false; //停止线程
    long m_totalOutTransferSize;
    ULONG m_timeoutPerTransferMilliSec;
    int m_queueSize;

    CCyUSBEndPoint  *m_BulkOutEpt;

    void abortXferLoop(int pending, PUCHAR *buffers, PUCHAR *contexts, OVERLAPPED *outOvLap);

};

#endif // THREADOUTASYNC_H
