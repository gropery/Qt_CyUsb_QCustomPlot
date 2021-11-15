#include "threadinsync.h"
#include <QTime>
#include <QDebug>

ThreadInSync::ThreadInSync()
{

}

//停止线程
void ThreadInSync::stopThread()
{
    m_stop=true;
}

bool ThreadInSync::setBulkInEpt(CCyUSBEndPoint *ept, long inTransferSize, int msec)
{
    if(ept == NULL){
        return false;
    }
    else{
        m_BulkInEpt = ept;
        m_totalInTransferSize = inTransferSize;
        m_timeoutPerTransferMilliSec = msec;

        return true;
    }
}

//线程任务
void ThreadInSync::run()
{
    UCHAR *bufferInput = new UCHAR[m_totalInTransferSize];

    m_BulkInEpt->TimeOut = m_timeoutPerTransferMilliSec;
    m_BulkInEpt->SetXferSize(m_totalInTransferSize);

    long readLength=0;
    m_inBytes=0;
    m_inSeccesses=0;
    m_inFailures=0;

    m_stop=false;
    //循环主体
    while(!m_stop)
    {
        //每次重新赋值，因为xferdata可能会修改readLength
        readLength = m_totalInTransferSize;
        //启动BulkIn传输
        if (m_BulkInEpt->XferData(bufferInput, readLength) == TRUE )
        {
            m_inBytes += readLength/1024; //kByte
            m_inSeccesses++;
        }
        else
        {
            m_inFailures++;
        }

        //qDebug()<<m_inBytes;
        //msleep(500); //线程休眠500ms
    }

    delete [] bufferInput;

//  在  m_stop==true时结束线程任务
    quit();//相当于  exit(0),退出线程的事件循环
}

bool ThreadInSync::inSyncOnce(UCHAR *bufferInput)
{
    m_BulkInEpt->TimeOut = m_timeoutPerTransferMilliSec;
    m_BulkInEpt->SetXferSize(m_totalInTransferSize);

    long readLength = m_totalInTransferSize;

    //启动BulkIn传输
    if (m_BulkInEpt->XferData(bufferInput, readLength) == TRUE )
    {
        m_inBytes += readLength/1024; //kByte
        m_inSeccesses++;
        return true;
    }
    else
    {
        m_inFailures++;
        return false;
    }
}
