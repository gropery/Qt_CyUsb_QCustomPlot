#include "threadinasync.h"
#include <QTime>
#include <QDebug>

#define     MAX_QUEUE_SZ   64

ThreadInAsync::ThreadInAsync()
{

}

//停止线程
void ThreadInAsync::stopThread()
{
    m_stop=true;
}

bool ThreadInAsync::setBulkInEpt(CCyUSBEndPoint *ept, long inTransferSize, int msec, int queueSize)
{
    if(ept == NULL){
        return false;
    }
    else{
        m_BulkInEpt = ept;
        m_totalInTransferSize = inTransferSize;
        m_timeoutPerTransferMilliSec = msec;
        m_queueSize = queueSize;

        return true;
    }
}

//线程任务
void ThreadInAsync::run()
{
    long readLength=m_totalInTransferSize;
    m_inBytes=0;
    m_inSeccesses=0;
    m_inFailures=0;
    int i = 0;

    //申请接收缓存
    PUCHAR *bufferInput = new PUCHAR[m_queueSize];
    PUCHAR *contexts   = new PUCHAR[m_queueSize];
    OVERLAPPED inOvLap[MAX_QUEUE_SZ];

    for (i=0; i< m_queueSize; i++)
    {
        bufferInput[i]    = new UCHAR[m_totalInTransferSize];
        inOvLap[i].hEvent = CreateEvent(NULL, false, false, NULL);

        memset(bufferInput[i],0xEF,m_totalInTransferSize);
    }

    m_BulkInEpt->SetXferSize(m_totalInTransferSize);

    //发起第一次IN接收请求
    for (i=0; i< m_queueSize; i++)
    {
        contexts[i] = m_BulkInEpt->BeginDataXfer(bufferInput[i], readLength, &inOvLap[i]);
        if (m_BulkInEpt->NtStatus || m_BulkInEpt->UsbdStatus) // BeginDataXfer failed
        {
            qDebug()<<"Xfer request rejected. NTSTATUS = "<<m_BulkInEpt->NtStatus;
            abortXferLoop(i+1, bufferInput, contexts, inOvLap);
            return;
        }
    }

    i=0;
    m_stop=false;
    //循环主体
    while(!m_stop)
    {
        //每次重新赋值，因为xferdata可能会修改readLength
        readLength = m_totalInTransferSize;

        //等待传输
        if (!m_BulkInEpt->WaitForXfer(&inOvLap[i], m_timeoutPerTransferMilliSec))
        {
            m_BulkInEpt->Abort();
            if (m_BulkInEpt->LastError == ERROR_IO_PENDING)
                WaitForSingleObject(inOvLap[i].hEvent, m_timeoutPerTransferMilliSec);
        }

        //传输完成
        if (m_BulkInEpt->FinishDataXfer(bufferInput[i], readLength, &inOvLap[i], contexts[i]))
        {
            m_inBytes += readLength/1024;  //kByte
            m_inSeccesses++;

            //qDebug()<<m_inBytes;
        }
        else
        {
            m_inFailures++;
        }

        // 重新发起IN接收请求
        contexts[i] = m_BulkInEpt->BeginDataXfer(bufferInput[i], readLength, &inOvLap[i]);
        if (m_BulkInEpt->NtStatus || m_BulkInEpt->UsbdStatus) // BeginDataXfer failed
        {
            qDebug()<<"Xfer request rejected. NtStatus = "<<m_BulkInEpt->NtStatus;
            qDebug()<<"Xfer request rejected. UsbdStatus = "<<m_BulkInEpt->UsbdStatus;
            abortXferLoop(m_queueSize, bufferInput, contexts, inOvLap);
            return;
        }

        i++;

        if (i == m_queueSize) //Only update the display once each time through the Queue
        {
            i=0;
        }
        //msleep(500); //线程休眠500ms
    } // End of the infinite loop

    // Memory clean-up
    abortXferLoop(m_queueSize, bufferInput, contexts, inOvLap);

//  在  m_stop==true时结束线程任务
    quit();//相当于  exit(0),退出线程的事件循环
}

void ThreadInAsync::abortXferLoop(int pending, PUCHAR *buffers, PUCHAR *contexts, OVERLAPPED *inOvLap)
{
    //EndPt->Abort(); - This is disabled to make sure that while application is doing IO and user unplug the device, this function hang the app.
    long len = m_totalInTransferSize;
    m_BulkInEpt->Abort();

    for (int j=0; j< m_queueSize; j++)
    {
        if (j<pending)
        {
            m_BulkInEpt->WaitForXfer(&inOvLap[j], m_timeoutPerTransferMilliSec);
            /*{
                EndPt->Abort();
                if (EndPt->LastError == ERROR_IO_PENDING)
                    WaitForSingleObject(inOvLap[j].hEvent,2000);
            }*/
            m_BulkInEpt->FinishDataXfer(buffers[j], len, &inOvLap[j], contexts[j]);
        }

        CloseHandle(inOvLap[j].hEvent);
        delete [] buffers[j];
    }

    delete [] buffers;
    delete [] contexts;

    //emit stop signal

}
