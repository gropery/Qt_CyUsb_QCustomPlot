#include "threadloopsync.h"
#include <QTime>
#include <QDebug>

ThreadLoopSync::ThreadLoopSync()
{

}

//停止线程
void ThreadLoopSync::stopThread()
{
    m_stop=true;
}

bool ThreadLoopSync::setBulkLoopEpt(CCyUSBEndPoint *eptin, CCyUSBEndPoint *eptout, long loopTransferSize, int msec)
{
    if((eptin == NULL)||(eptout == NULL)){
        return false;
    }
    else{
        m_BulkInEpt = eptin;
        m_BulkOutEpt = eptout;
        m_totalLoopTransferSize = loopTransferSize;
        m_timeoutPerTransferMilliSec = msec;

        return true;
    }
}

//线程任务
void ThreadLoopSync::run()
{
    UCHAR *bufferInput = new UCHAR[m_totalLoopTransferSize];
    UCHAR *bufferOutput  = new UCHAR[m_totalLoopTransferSize];
    memset(bufferInput,0x00,m_totalLoopTransferSize);
    memset(bufferOutput,0x00,m_totalLoopTransferSize);

    m_BulkInEpt->TimeOut = m_timeoutPerTransferMilliSec;
    m_BulkInEpt->SetXferSize(m_totalLoopTransferSize);
    m_BulkOutEpt->TimeOut = m_timeoutPerTransferMilliSec;
    m_BulkOutEpt->SetXferSize(m_totalLoopTransferSize);

    m_inBytes=0;
    m_inSeccesses=0;
    m_inFailures=0;
    m_outBytes=0;
    m_outSeccesses=0;
    m_outFailures=0;
    m_loopBytes=0;
    m_loopSeccesses=0;
    m_loopFailures=0;
    m_tPcLoop=0;
    m_tMaxPcLoop=0;
    m_tAvgPcLoop=0;

    long readLength=0;
    long writeLength=0;
    quint64 tSumPcLoop=0;
    quint64 cntPcLoop=0;
    UCHAR cnt=0;            //每次轮训发送数据值累加
    int STATE=0;            //状态机
    int readDummyTimes=0;   //启动线程初始，读出USB硬件端多余的IN数据计数
    int runTimes=0;         //启动线程初始，计数loop次数，除外前n次计数

    m_stop=false;
    //循环主体
    while(!m_stop)
    {
        switch (STATE) {
        case 0: //第一次运行读出USB硬件端多余数据
            readLength = m_totalLoopTransferSize;
            while(m_BulkInEpt->XferData(bufferInput, readLength) == TRUE)
            {
                readDummyTimes++;
                qDebug()<<"first run read dummy data"<<readDummyTimes;
            }
            STATE = 1;
            break;

        case 1: //启动BulkOut传输
            for (int i=0; i<writeLength; i++)
            {
                bufferOutput[i] = cnt;
            }

            writeLength = m_totalLoopTransferSize;
            if (m_BulkOutEpt->XferData(bufferOutput, writeLength) == TRUE )
            {
                m_outBytes += writeLength/1024; //kByte
                m_outSeccesses++;
                STATE = 2;
            }
            else
            {
                m_outFailures++;
                STATE = 0;
            }
            break;

        case 2:  //启动BulkIn传输
            readLength = m_totalLoopTransferSize;
            if (m_BulkInEpt->XferData(bufferInput, readLength) == TRUE )
            {
                m_inBytes += readLength/1024; //kByte
                m_inSeccesses++;
                STATE = 3;
            }
            else
            {
                m_inFailures++;
                STATE = 0;
            }
            break;

        case 3:
            if((writeLength==m_totalLoopTransferSize)   //OUT长度等于burst len
                &&(readLength==m_totalLoopTransferSize  //IN长度等于burst len
                &&(bufferInput[0]==bufferOutput[0])))   //loop数据校验正确
            {
                 //-----------------------------------------------------
                 //大小端测试
//                quint32 buf1=0;
//                QString strbuf1("");
//                buf1 = (bufferInput[7]<<8*3)
//                      +(bufferInput[6]<<8*2)
//                      +(bufferInput[5]<<8*1)
//                      +(bufferInput[4]);
//                strbuf1 = tr("%1").arg(buf1,8,16,QChar('0'));
//                qDebug()<<"strbuf1="<<strbuf1;

                //--------------------------------------------------------
                //tPcLoop时长计算
                m_tPcLoop = ((bufferInput[11]<<8*3)
                       +(bufferInput[10]<<8*2)
                       +(bufferInput[9]<<8*1)
                       +(bufferInput[8]))/100;      //us

                //至少跳过最早的2个tPcLoop值
                //因为在第一个为之前的计算值，第二个由于中间暂停所以数值很大不正确
                //第三个已经是正确的值了，这里选择3只是为了调试
                if(runTimes>=3)
                {
                    //查找最大时间
                    if(m_tMaxPcLoop < m_tPcLoop)
                        m_tMaxPcLoop = m_tPcLoop;

                    //计算平均时间
                    tSumPcLoop += m_tPcLoop;
                    cntPcLoop++;
                    m_tAvgPcLoop = tSumPcLoop/cntPcLoop;
                }
                else
                {
                    qDebug()<<"runTimes="<<runTimes<<", m_tPcLoop(us)="<<m_tPcLoop;
                    runTimes++;
                }

                m_loopBytes += m_totalLoopTransferSize/1024; //kByte
                m_loopSeccesses++;
                cnt++;      //累加loop值
                STATE = 1;
            }
            else
            {
                m_loopFailures++;
                STATE = 0;
            }
            break;

        default:
            STATE = 0;
            break;
        }

        //qDebug()<<m_inBytes;
        //msleep(500); //线程休眠500ms
    }

    delete [] bufferInput;
    delete [] bufferOutput;

//  在  m_stop==true时结束线程任务
    quit();//相当于  exit(0),退出线程的事件循环
}

