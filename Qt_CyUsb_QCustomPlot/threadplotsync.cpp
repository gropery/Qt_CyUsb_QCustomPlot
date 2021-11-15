#include "threadplotsync.h"
#include <QTime>
#include <QDebug>

#define PLOTLEN 7

ThreadPlotSync::ThreadPlotSync()
{
    // 模拟波形串口输出定时器
    timerWaveGene = new QTimer;
    timerWaveGene->setInterval(20);
    connect(timerWaveGene, &QTimer::timeout, this, [=](){
        slot_timerWaveGene_timeout();
    });

    // 新建波形显示界面
    plot = new Plot;
    plot->show();
}

ThreadPlotSync::~ThreadPlotSync()
{
    delete plot;
}

//停止线程
void ThreadPlotSync::stopThread()
{
    m_stop=true;
}

bool ThreadPlotSync::setBulkEpt(CCyUSBEndPoint *eptin, CCyUSBEndPoint *eptout, long transferSize, int msec)
{
    if((eptin == NULL)||(eptout == NULL)){
        return false;
    }
    else{
        m_BulkInEpt = eptin;
        m_BulkOutEpt = eptout;
        m_totalTransferSize = transferSize;
        m_timeoutPerTransferMilliSec = msec;

        return true;
    }
}

//线程任务
void ThreadPlotSync::run()
{
    UCHAR *bufferInput = new UCHAR[m_totalTransferSize];
    char *bufferInput2 = new char[m_totalTransferSize];

    m_BulkInEpt->TimeOut = m_timeoutPerTransferMilliSec;
    m_BulkInEpt->SetXferSize(m_totalTransferSize);

    long readLength=0;
    m_inBytes=0;
    m_inSeccesses=0;
    m_inFailures=0;
    m_plotSeccesses=0;
    m_plotFailures=0;

    m_stop=false;
    //循环主体
    while(!m_stop)
    {
        //每次重新赋值，因为xferdata可能会修改readLength
        readLength = m_totalTransferSize;
        //启动BulkIn传输
        if (m_BulkInEpt->XferData(bufferInput, readLength) == TRUE )
        {
            m_inBytes += readLength/1024; //kByte
            m_inSeccesses++;

            //uchar * 类型转换成 char *
            for(int i=0; i<readLength; i++)
            {
                bufferInput2[i] = (char)bufferInput[i];
            }
            //char * 类型转换成 qstring
            QByteArray baRecvData(bufferInput2,PLOTLEN);
            //调用私有成员函数解析数据协议
            processRecvProtocol(&baRecvData);
        }
        else
        {
            m_inFailures++;
        }

        //qDebug()<<m_inBytes;
        //msleep(500); //线程休眠500ms
    }

    delete [] bufferInput;
    delete [] bufferInput2;
//  在  m_stop==true时结束线程任务
    quit();//相当于  exit(0),退出线程的事件循环
}

//对接收数据流进行协议解析
void ThreadPlotSync::processRecvProtocol(QByteArray *baRecvData)
{
    int num = baRecvData->size();

    if(num) {
        for(int i=0; i<num; i++){
            switch(STATE) {
            //查找第一个帧头
            case FIND_HEADER1:
                if(baRecvData->at(i) == 0x3A){
                    STATE = FIND_HEADER2;
                }
                break;
            //查找第二个帧头
            case FIND_HEADER2:
                if(baRecvData->at(i) == 0x3B){
                    //找到第二个帧头
                    STATE = CHECK_FUNWORD;
                }
                else if(baRecvData->at(i) == 0x3A){
                    //避免漏掉3A 3A 3B xx情况
                    STATE = FIND_HEADER2;
                }
                else
                    STATE = FIND_HEADER1;
                break;
            //检查功能字是否合规
            case CHECK_FUNWORD:
                if(baRecvData->at(i) == 0x01){
                    funWord = baRecvData->at(i);
                    STATE = CHECK_DATALEN;
                }
                else{
                    funWord = 0;
                    STATE = FIND_HEADER1;
                }
                break;
            //判断数据长度是否合规
            case CHECK_DATALEN:
                if(baRecvData->at(i) <= 32*2){
                    //限制最大接收长度64字节，即32个ch
                    dataLen = baRecvData->at(i);
                    STATE = TAKE_DATA;
                }
                else{
                    dataLen = 0;
                    STATE = FIND_HEADER1;
                }
                break;
            //接收所有数据
            case TAKE_DATA:
                if(dataLenCnt < dataLen){
                    baRecvDataBuf.append(baRecvData->at(i));
                    dataLenCnt++;
                    //满足条件后立刻转入下个状态
                    if(dataLenCnt >= dataLen){
                        dataLenCnt = 0;
                        STATE = CHECK_CRC;
                    }
                }
                break;
            //CRC校验
            case CHECK_CRC:
                crc = 0x3A + 0X3B + funWord + dataLen;
                for(int c=0; c<dataLen; c++){
                    crc += baRecvDataBuf[c];
                }

                //校验通过，数据有效
                if(crc == baRecvData->at(i)){
                    plot->onNewDataArrived(baRecvDataBuf);
                    m_plotSeccesses++; //有效帧数量计数
                }
                else{
                    m_plotFailures++; //误码帧数量计数
                }

                //清除暂存数据
                baRecvDataBuf.clear();
                funWord=0;
                dataLen=0;
                dataLenCnt=0;
                crc=0;
                STATE = FIND_HEADER1;
                break;

            default:
                STATE = FIND_HEADER1;
                break;
            }
        }
    }
}


void ThreadPlotSync::slot_timerWaveGene_timeout()
{
    // 准备正弦数据并按协议打包
    static double x;
    char y1,y2;
    char crc;

    x += 0.01;
    y1 = sin(x)*50;
    y2 = cos(x)*100;
    crc = 0x3A+0x3B+0x01+0x02+y1+y2;

    QByteArray ba;
    ba.append(0x3A).append(0x3B).append(0x01).append(0x02).append(y1).append(y2).append(crc);
    int baLen = ba.length();

    //将打包好的数据放入4096B pkg中
    UCHAR *bufferOutput = new UCHAR[m_totalTransferSize];
    memset(bufferOutput, 0x00, m_totalTransferSize);

    for(int i=0; i<m_totalTransferSize; i++)
    {
        if(i < baLen)                   //包起始
            bufferOutput[i] = ba.at(i);
        else if(i > m_totalTransferSize-2)  //包尾
            bufferOutput[i] = 0xA5;
        else
            bufferOutput[i] = 0x12;     //包填充
    }

    //USB OUT数据
    m_BulkOutEpt->TimeOut = m_timeoutPerTransferMilliSec;
    m_BulkOutEpt->SetXferSize(m_totalTransferSize);
    long writeLength = m_totalTransferSize;

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
    delete [] bufferOutput;
}



void ThreadPlotSync::startTimer(int msec)
{
    timerWaveGene->start(msec);
}

void ThreadPlotSync::stopTimer()
{
    timerWaveGene->stop();
}

void ThreadPlotSync::showWave()
{
    if(plot==NULL)
        return;

    plot->show();
}
