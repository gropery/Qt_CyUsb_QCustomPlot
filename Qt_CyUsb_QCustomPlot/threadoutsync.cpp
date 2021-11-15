#include "threadoutsync.h"
#include <QTime>
#include <QDebug>

ThreadOutSync::ThreadOutSync()
{

}

//停止线程
void ThreadOutSync::stopThread()
{
    m_stop=true;
}

bool ThreadOutSync::setBulkOutEpt(CCyUSBEndPoint *ept, long outTransferSize, int msec)
{
    if(ept == NULL){
        return false;
    }
    else{
        m_BulkOutEpt = ept;
        m_totalOutTransferSize = outTransferSize;
        m_timeoutPerTransferMilliSec = msec;

        return true;
    }
}

//线程任务
void ThreadOutSync::run()
{
    m_BulkOutEpt->TimeOut = m_timeoutPerTransferMilliSec;
    m_BulkOutEpt->SetXferSize(m_totalOutTransferSize);

    // 申请缓存器，用来存储OutData
    UCHAR *bufferOutput  = new UCHAR[m_totalOutTransferSize];
    long writeLength=0;
    UCHAR j=0;
    m_outBytes=0;
    m_outSeccesses=0;
    m_outFailures=0;

    m_stop=false;
    //循环主体
    while(!m_stop)
    {
        for (int i=0; i<writeLength; i++)
        {
            bufferOutput[i] = j;
        }
        j++;

        //每次重新赋值，因为xferdata可能会修改writeLength
        writeLength = m_totalOutTransferSize;

        //启动BulkOut传输
        if (m_BulkOutEpt->XferData(bufferOutput, writeLength) == TRUE )
        {
            m_outBytes += writeLength/1024; //kByte
            m_outSeccesses++;
        }
        else
        {
            m_outFailures++;
        }
        //qDebug()<<m_outBytes;
        //msleep(500); //线程休眠500ms
    }

    delete [] bufferOutput;

//  在  m_stop==true时结束线程任务
    quit();//相当于  exit(0),退出线程的事件循环
}

bool ThreadOutSync::outSyncOnce(UCHAR *bufferOutput)
{
    m_BulkOutEpt->TimeOut = m_timeoutPerTransferMilliSec;
    m_BulkOutEpt->SetXferSize(m_totalOutTransferSize);

    long writeLength = m_totalOutTransferSize;

    //启动BulkOut传输
    if (m_BulkOutEpt->XferData(bufferOutput, writeLength) == TRUE )
    {
        m_outBytes += writeLength/1024; //kByte
        m_outSeccesses++;
        return true;
    }
    else
    {
        m_outFailures++;
        return false;
    }
}
