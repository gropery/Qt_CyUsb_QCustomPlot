#include "threadoutasync.h"
#include <QTime>
#include <QDebug>

#define     MAX_QUEUE_SZ   64

ThreadOutAsync::ThreadOutAsync()
{

}

//停止线程
void ThreadOutAsync::stopThread()
{
    m_stop=true;
}

bool ThreadOutAsync::setBulkOutEpt(CCyUSBEndPoint *ept, long outTransferSize, int msec, int queueSize)
{
    if(ept == NULL){
        return false;
    }
    else{
        m_BulkOutEpt = ept;
        m_totalOutTransferSize = outTransferSize;
        m_timeoutPerTransferMilliSec = msec;
        m_queueSize = queueSize;

        return true;
    }
}

//线程任务
void ThreadOutAsync::run()
{
    long writeLength=m_totalOutTransferSize;
    m_outBytes=0;
    m_outSeccesses=0;
    m_outFailures=0;
    int i = 0;
    UCHAR m=0;

    //申请接收缓存
    PUCHAR *bufferOutput = new PUCHAR[m_queueSize];
    PUCHAR *contexts   = new PUCHAR[m_queueSize];
    OVERLAPPED outOvLap[MAX_QUEUE_SZ];

    for (i=0; i< m_queueSize; i++)
    {
        bufferOutput[i]    = new UCHAR[m_totalOutTransferSize];
        outOvLap[i].hEvent = CreateEvent(NULL, false, false, NULL);

        memset(bufferOutput[i],0xEF,m_totalOutTransferSize);
    }

    m_BulkOutEpt->SetXferSize(m_totalOutTransferSize);

    //发起第一次OUT接收请求
    for (i=0; i< m_queueSize; i++)
    {
        contexts[i] = m_BulkOutEpt->BeginDataXfer(bufferOutput[i], writeLength, &outOvLap[i]);
        if (m_BulkOutEpt->NtStatus || m_BulkOutEpt->UsbdStatus) // BeginDataXfer failed
        {
            qDebug()<<"Xfer request rejected. NTSTATUS = "<<m_BulkOutEpt->NtStatus;
            abortXferLoop(i+1, bufferOutput, contexts, outOvLap);
            return;
        }
    }

    i=0;
    m_stop=false;
    //循环主体
    while(!m_stop)
    {
        //每次重新赋值，因为xferdata可能会修改writeLength
        writeLength = m_totalOutTransferSize;

        //等待传输
        if (!m_BulkOutEpt->WaitForXfer(&outOvLap[i], m_timeoutPerTransferMilliSec))
        {
            m_BulkOutEpt->Abort();
            if (m_BulkOutEpt->LastError == ERROR_IO_PENDING)
                WaitForSingleObject(outOvLap[i].hEvent, m_timeoutPerTransferMilliSec);
        }

        //传输完成
        if (m_BulkOutEpt->FinishDataXfer(bufferOutput[i], writeLength, &outOvLap[i], contexts[i]))
        {
            m_outBytes += writeLength/1024;  //kByte
            m_outSeccesses++;

            //qDebug()<<m_outBytes;
        }
        else
        {
            m_outFailures++;
        }

        //输出数据累加赋值
        m++;
        for (int k=0; k<writeLength; k++)
        {
            bufferOutput[i][k] = m;
        }

        //重新发起OUT请求
        contexts[i] = m_BulkOutEpt->BeginDataXfer(bufferOutput[i], writeLength, &outOvLap[i]);
        if (m_BulkOutEpt->NtStatus || m_BulkOutEpt->UsbdStatus) // BeginDataXfer failed
        {
            qDebug()<<"Xfer request rejected. NtStatus = "<<m_BulkOutEpt->NtStatus;
            qDebug()<<"Xfer request rejected. UsbdStatus = "<<m_BulkOutEpt->UsbdStatus;
            abortXferLoop(m_queueSize, bufferOutput, contexts, outOvLap);
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
    abortXferLoop(m_queueSize, bufferOutput, contexts, outOvLap);

//  在  m_stop==true时结束线程任务
    quit();//相当于  exit(0),退出线程的事件循环
}

void ThreadOutAsync::abortXferLoop(int pending, PUCHAR *buffers, PUCHAR *contexts, OVERLAPPED *outOvLap)
{
    //EndPt->Abort(); - This is disabled to make sure that while application is doing IO and user unplug the device, this function hang the app.
    long len = m_totalOutTransferSize;
    m_BulkOutEpt->Abort();

    for (int j=0; j< m_queueSize; j++)
    {
        if (j<pending)
        {
            m_BulkOutEpt->WaitForXfer(&outOvLap[j], m_timeoutPerTransferMilliSec);
            /*{
                EndPt->Abort();
                if (EndPt->LastError == ERROR_IO_PENDING)
                    WaitForSingleObject(outOvLap[j].hEvent,2000);
            }*/
            m_BulkOutEpt->FinishDataXfer(buffers[j], len, &outOvLap[j], contexts[j]);
        }

        CloseHandle(outOvLap[j].hEvent);
        delete [] buffers[j];
    }

    delete [] buffers;
    delete [] contexts;

    //emit stop signal

}
