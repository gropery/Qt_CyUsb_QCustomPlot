#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"

#define     USB30MAJORVER                       0x0300
#define     PACKETS_PER_TRANSFER                32          //每次传输的数据包个数（每个包大小是MaxPkgSize=4096B）
#define     NUM_TRANSFER_PER_TRANSACTION        16          //传输事务发起的传输数量（目的是减少PC端程序频繁介入）
#define     TIMEOUT_PER_TRANSFER_MILLI_SEC      1500

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_CCyUSBDevice = new CCyUSBDevice(this, CYUSBDRV_GUID, true);

    SurveyExistingDevices();					//填充Connected Devices Combox
    EnumerateEndpointForTheSelectedDevice();	//填充IN/OUT Endpoints Combox

    // Label数据更新-定时器
    timerUpdateLabel = new QTimer;
    timerUpdateLabel->start(1000);
    connect(timerUpdateLabel, &QTimer::timeout, this, [=](){
        slot_timerUpdateLabel_timeout();
    });
}

MainWindow::~MainWindow()
{
    if (m_ThreadOutSync.isRunning())
    {
        m_ThreadOutSync.stopThread();
        m_ThreadOutSync.wait();
    }

    if (m_ThreadInSync.isRunning())
    {
        m_ThreadInSync.stopThread();
        m_ThreadInSync.wait();
    }

    if (m_ThreadOutAsync.isRunning())
    {
        m_ThreadOutAsync.stopThread();
        m_ThreadOutAsync.wait();
    }

    if (m_ThreadInAsync.isRunning())
    {
        m_ThreadInAsync.stopThread();
        m_ThreadInAsync.wait();
    }

    if (m_ThreadLoopSync.isRunning())
    {
        m_ThreadLoopSync.stopThread();
        m_ThreadLoopSync.wait();
    }

    if (m_ThreadPlotSync.isRunning())
    {
        m_ThreadPlotSync.stopThread();
        m_ThreadPlotSync.wait();
    }

    if (m_CCyUSBDevice)
    {
        if (m_CCyUSBDevice->IsOpen())
            m_CCyUSBDevice->Close();
        delete m_CCyUSBDevice;
    }

    delete ui;
}

//查找USBDevice存在的设备，并填充IDC_CBO_DEVICES，并保留之前选中的设备index操作
BOOL MainWindow::SurveyExistingDevices()
{
    CCyUSBDevice *USBDevice = new CCyUSBDevice(this, CYUSBDRV_GUID, true);

    //暂存当前设备名称 & 清除设备下拉列表框
    QString strDevice("");
    if(ui->comboBoxDevices->count()>0)
        strDevice = ui->comboBoxDevices->currentText();
    ui->comboBoxDevices->clear();

    if (USBDevice != NULL)
    {
        int nCboIndex = -1;
        int nDeviceCount = USBDevice->DeviceCount();
        //枚举并打开所有设备，读取器VID,PID,设备名
        for (int device = 0; device < nDeviceCount; device++ )
        {
            QString strDeviceData("");
            USBDevice->Open(device);    //打开USB设备
            strDeviceData = tr("(0x%1 - 0x%2) %3")
                            .arg(USBDevice->VendorID,4,16,QChar('0'))
                            .arg(USBDevice->ProductID,4,16,QChar('0'))
                            .arg(USBDevice->FriendlyName);
            ui->comboBoxDevices->addItem(strDeviceData);
            //暂存之前已选定的设备在下拉列表的index值
            if (nCboIndex == -1 && strDevice.isEmpty() == FALSE && strDevice == strDeviceData )
                nCboIndex = device;

            USBDevice->Close();
        }

        delete USBDevice;

        //重新选定之前选定的设备
        if (ui->comboBoxDevices->count()>0)
        {
            if (nCboIndex != -1 )
                ui->comboBoxDevices->setCurrentIndex(nCboIndex);
            else
                ui->comboBoxDevices->setCurrentIndex(0);
        }

        return TRUE;
    }
    else
        return FALSE;   //未找到USB Device
}

//根据Connected Devices选择的设备，枚举其接口，从其接口类函数枚举Bulk,ISOC,INTR端点至IN Out Combox
BOOL MainWindow::EnumerateEndpointForTheSelectedDevice()
{
    int nDeviceIndex = ui->comboBoxDevices->currentIndex();

    if (nDeviceIndex == -1 || m_CCyUSBDevice == NULL )
        return FALSE;

    // 打开选定的设备
    m_CCyUSBDevice->Open(nDeviceIndex);
    // 获取设备中总接口数量，其等于主接口(AltSetting == 0)加上副接口数量
    int nIntfcCount = m_CCyUSBDevice->AltIntfcCount()+1;
    ui->comboBoxInEndPoints->clear();
    ui->comboBoxOutEndPoints->clear();

    // 枚举打开设备中的每个接口
    for (int intfc = 0; intfc < nIntfcCount; intfc++ )
    {
        if(m_CCyUSBDevice->SetAltIntfc(intfc) == true)
        {
            int eptCnt = m_CCyUSBDevice->EndPointCount();

            // 枚举接口中的每个端口，但跳过端点0控制端口
            for (int endPoint = 1; endPoint < eptCnt; endPoint++)
            {
                CCyUSBEndPoint *ept = m_CCyUSBDevice->EndPoints[endPoint];

                // Attributes表示端点的类型,这里只处理块端点
                // 0 控制端点
                // 1 同步端点
                // 2 块端点
                // 3 中断端点
                // 对应USB的四种传输方式
                if (ept->Attributes == 2)
                {
                    QString strData("BULK-");

                    strData += (ept->bIn ? "IN, " : "OUT, ");
                    strData += tr("MaxPktSize-%1 Bytes, ").arg(ept->MaxPktSize);
                    if(m_CCyUSBDevice->BcdUSB == USB30MAJORVER)
                    {
                        strData += tr("USB3MaxBurst-%1, ").arg(ept->ssmaxburst);
                    }
                    strData += tr("AltIntfc-%1, ").arg(intfc);
                    strData += tr("EpAddr-0x%1, ").arg(ept->Address,2,16,QChar('0'));

                    if (ept->bIn)
                    {
                       ui->comboBoxInEndPoints->addItem(strData);
                       m_BulkInEpt = ept;
                       ui->lineEditInSyncOnceBytes->setText(tr("").number(ept->MaxPktSize));
                    }
                    else
                    {
                       ui->comboBoxOutEndPoints->addItem(strData);
                       m_BulkOutEpt = ept;
                       ui->lineEditOutSyncOnceBytes->setText(tr("").number(ept->MaxPktSize));
                    }
                }
            }
         }
    }

    if(ui->comboBoxOutEndPoints->count()>0)
        ui->comboBoxOutEndPoints->setCurrentIndex(0);

    if(ui->comboBoxInEndPoints->count()>0)
        ui->comboBoxInEndPoints->setCurrentIndex(0);

    return TRUE;
}

void MainWindow::on_comboBoxDevices_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    EnumerateEndpointForTheSelectedDevice();
}

//同步连续输出
void MainWindow::on_pushButtonOutSyncContinue_clicked()
{
    if(ui->pushButtonOutSyncContinue->text()=="Start")
    {
        if(m_ThreadOutSync.setBulkOutEpt(m_BulkOutEpt,
                                         PACKETS_PER_TRANSFER*m_BulkOutEpt->MaxPktSize,
                                         TIMEOUT_PER_TRANSFER_MILLI_SEC)==true)
        {
           //启动线程
           m_ThreadOutSync.start();
           ui->pushButtonOutSyncContinue->setText("Stop");
        }
    }
    else{
        ui->pushButtonOutSyncContinue->setText("Start");
        //停止线程
        m_ThreadOutSync.stopThread();
        m_ThreadOutSync.wait();
    }
}

//同步单次输出
void MainWindow::on_pushButtonOutSyncOnce_clicked()
{
    long outTransferSize = ui->lineEditOutSyncOnceBytes->text().toLong();

    if(outTransferSize < m_BulkOutEpt->MaxPktSize)
    {
        outTransferSize = m_BulkOutEpt->MaxPktSize;
        ui->lineEditOutSyncOnceBytes->setText(tr("").number(outTransferSize));
    }

    if(m_ThreadOutSync.setBulkOutEpt(m_BulkOutEpt,
                                     //这里单次读写长度和BurstLen一致
                                     outTransferSize,
                                     TIMEOUT_PER_TRANSFER_MILLI_SEC)==true)
    {
        QString strSendData = ui->plainTextEditBufferOut->toPlainText();
        QByteArray ba = QByteArray::fromHex(strSendData.toLocal8Bit());    // GB2312编码输出
        int baLen = ba.length();

        UCHAR *bufferOutput = new UCHAR[outTransferSize];
        for(int i=0; i<outTransferSize; i++)
        {
            if(i < baLen)                   //包起始
                bufferOutput[i] = ba.at(i);
            else if(i > outTransferSize-2)  //包尾
                bufferOutput[i] = 0xA5;
            else
                bufferOutput[i] = 0x12;     //包填充
        }

        if(m_ThreadOutSync.outSyncOnce(bufferOutput)==true)
        {
            qDebug()<<"out ok";
        }

        delete [] bufferOutput;
    }
}

//同步连续输入
void MainWindow::on_pushButtonInSyncContinue_clicked()
{
    if(ui->pushButtonInSyncContinue->text()=="Start")
    {
        if(m_ThreadInSync.setBulkInEpt(m_BulkInEpt,
                                         PACKETS_PER_TRANSFER*m_BulkInEpt->MaxPktSize,
                                         TIMEOUT_PER_TRANSFER_MILLI_SEC)==true)
        {
           //启动线程
           m_ThreadInSync.start();
           ui->pushButtonInSyncContinue->setText("Stop");
        }
    }
    else{
        ui->pushButtonInSyncContinue->setText("Start");
        //停止线程
        m_ThreadInSync.stopThread();
        m_ThreadInSync.wait();
    }
}

//同步单次输入
void MainWindow::on_pushButtonInSyncOnce_clicked()
{
    long inTransferSize = ui->lineEditInSyncOnceBytes->text().toLong();

    if(inTransferSize < m_BulkInEpt->MaxPktSize)
    {
        inTransferSize = m_BulkInEpt->MaxPktSize;
        ui->lineEditInSyncOnceBytes->setText(tr("").number(inTransferSize));
    }

    if(m_ThreadInSync.setBulkInEpt(m_BulkInEpt,
                                   //这里单次读写长度和BurstLen一致
                                   inTransferSize,
                                   TIMEOUT_PER_TRANSFER_MILLI_SEC)==true)
    {
        UCHAR *bufferInput = new UCHAR[inTransferSize];

        char *bufferInput2 = new char[inTransferSize];
        if(m_ThreadInSync.inSyncOnce(bufferInput)==true)
        {
            //uchar * 类型转换成 char *
            for(int i=0; i<inTransferSize; i++)
            {
                bufferInput2[i] = (char)bufferInput[i];
            }

            //char * 类型转换成 qstring
            QByteArray baRecvData(bufferInput2,inTransferSize);
            QByteArray ba = baRecvData.toHex(' ').toUpper();
            QString str = QString(ba).append(' ');

            //qstring 每16个bytes作为一行打印
            QString str2("");
            for(int j=0; j<inTransferSize*3; j=j+3)
            {
                if(j%(16*3)==0)
                {
                    str2 += tr("0x%1(%2): ").arg(int(j/3),4,16,QChar('0'))
                                            .arg(int(j/3),4,10,QChar('0'));
                    str2 += str.mid(j,16*3).append('\n');
                }
            }

            // 在当前位置插入文本
            ui->plainTextEditBufferIn->appendPlainText(str2);
            // 移动光标到文本结尾
            ui->plainTextEditBufferIn->moveCursor(QTextCursor::End);

        }
        delete [] bufferInput;
        delete [] bufferInput2;
    }
}

//异步连续输出
void MainWindow::on_pushButtonOutAsyncContinue_clicked()
{
    if(ui->pushButtonOutAsyncContinue->text()=="Start")
    {
        if(m_ThreadOutAsync.setBulkOutEpt(m_BulkOutEpt,
                                         PACKETS_PER_TRANSFER*m_BulkOutEpt->MaxPktSize,
                                         TIMEOUT_PER_TRANSFER_MILLI_SEC,
                                         NUM_TRANSFER_PER_TRANSACTION)==true)
        {
           //启动线程
           m_ThreadOutAsync.start();
           ui->pushButtonOutAsyncContinue->setText("Stop");
        }
    }
    else{
        ui->pushButtonOutAsyncContinue->setText("Start");
        //停止线程
        m_ThreadOutAsync.stopThread();
        m_ThreadOutAsync.wait();
    }
}

//异步连续输入
void MainWindow::on_pushButtonInAsyncContinue_clicked()
{
    if(ui->pushButtonInAsyncContinue->text()=="Start")
    {
        if(m_ThreadInAsync.setBulkInEpt(m_BulkInEpt,
                                         PACKETS_PER_TRANSFER*m_BulkInEpt->MaxPktSize,
                                         TIMEOUT_PER_TRANSFER_MILLI_SEC,
                                         NUM_TRANSFER_PER_TRANSACTION)==true)
        {
           //启动线程
           m_ThreadInAsync.start();
           ui->pushButtonInAsyncContinue->setText("Stop");
        }
    }
    else{
        ui->pushButtonInAsyncContinue->setText("Start");
        //停止线程
        m_ThreadInAsync.stopThread();
        m_ThreadInAsync.wait();
    }
}

//同步连续LOOP
void MainWindow::on_pushButtonLoopSyncContinue_clicked()
{
    if(ui->pushButtonLoopSyncContinue->text()=="Start")
    {
        if(m_ThreadLoopSync.setBulkLoopEpt(m_BulkInEpt,
                                           m_BulkOutEpt,
                                           //这里单次读写长度和BurstLen一致
                                           m_BulkOutEpt->MaxPktSize,
                                           TIMEOUT_PER_TRANSFER_MILLI_SEC)==true)
        {
           //启动线程
           m_ThreadLoopSync.start();
           ui->pushButtonLoopSyncContinue->setText("Stop");
        }
    }
    else{
        ui->pushButtonLoopSyncContinue->setText("Start");
        //停止线程
        m_ThreadLoopSync.stopThread();
        m_ThreadLoopSync.wait();
    }
}

//同步连续LOOP将数据Plot
void MainWindow::on_checkBox_stateChanged(int arg1)
{
    int TimerInterval = ui->lineEditWaveGeneInterval->text().toInt();

    if(arg1 == Qt::Checked){
        if(m_ThreadPlotSync.setBulkEpt(m_BulkInEpt,
                                     m_BulkOutEpt,
                                     //这里单次读写长度和BurstLen一致
                                     m_BulkOutEpt->MaxPktSize,
                                     TIMEOUT_PER_TRANSFER_MILLI_SEC)==true)
        {
            //启动USB IN线程
            m_ThreadPlotSync.start();

            //启动USB OUT定时输出功能
            ui->lineEditWaveGeneInterval->setEnabled(false);
            m_ThreadPlotSync.startTimer(TimerInterval);
        }
    }
    else if(arg1 == Qt::Unchecked){
        //停止USB OUT定时输出功能
        m_ThreadPlotSync.stopTimer();
        ui->lineEditWaveGeneInterval->setEnabled(true);

        //停止USB IN线程
        m_ThreadPlotSync.stopThread();
        m_ThreadPlotSync.wait();
    }
}

//显示波形
void MainWindow::on_pushButtonShowWave_clicked()
{
    m_ThreadPlotSync.showWave();
}

//定时器槽函数timeout，1s 数据更新
void MainWindow::slot_timerUpdateLabel_timeout(void)
{
    //从类中获取数据
    m_curOutBytes = m_ThreadOutSync.m_outBytes;
    m_curInBytes = m_ThreadInSync.m_inBytes;
    m_curOutAsyncBytes = m_ThreadOutAsync.m_outBytes;
    m_curInAsyncBytes = m_ThreadInAsync.m_inBytes;

    m_curLoopOutBytes = m_ThreadLoopSync.m_outBytes;
    m_curLoopInBytes = m_ThreadLoopSync.m_inBytes;
    m_curLoopBytes = m_ThreadLoopSync.m_loopBytes;

    m_curPlotOutBytes = m_ThreadPlotSync.m_outBytes;
    m_curPlotInBytes = m_ThreadPlotSync.m_inBytes;
    m_curPlotBytes = m_ThreadPlotSync.m_plotSeccesses;

    //当前总计数-上次总结存暂存
    m_outRate = m_curOutBytes - m_lastOutByte;
    m_inRate = m_curInBytes - m_lastInByte;
    m_outAsyncRate = m_curOutAsyncBytes - m_lastOutAsyncByte;
    m_inAsyncRate = m_curInAsyncBytes - m_lastInAsyncByte;

    m_loopOutRate = m_curLoopOutBytes - m_lastLoopOutByte;
    m_loopInRate = m_curLoopInBytes - m_lastLoopInByte;
    m_loopRate = m_curLoopBytes - m_lastLoopByte;

    m_plotOutRate = m_curPlotOutBytes - m_lastPlotOutByte;
    m_plotInRate = m_curPlotInBytes - m_lastPlotInByte;
    m_plotRate = m_curPlotBytes - m_lastPlotByte;

    //暂存当前总计数
    m_lastOutByte = m_curOutBytes;
    m_lastInByte = m_curInBytes;
    m_lastOutAsyncByte = m_curOutAsyncBytes;
    m_lastInAsyncByte = m_curInAsyncBytes;

    m_lastLoopOutByte = m_curLoopOutBytes;
    m_lastLoopInByte = m_curLoopInBytes;
    m_lastLoopByte = m_curLoopBytes;

    m_lastPlotOutByte = m_curPlotOutBytes;
    m_lastPlotInByte = m_curPlotInBytes;
    m_lastPlotByte = m_curPlotBytes;

    //设置label
    ui->lineEditOutRate->setText(tr("%1").arg(m_outRate));
    ui->lineEditOutBytes->setText(tr("%1").arg(m_curOutBytes));
    ui->lineEditOutSuccesses->setText(tr("%1").arg(m_ThreadOutSync.m_outSeccesses));
    ui->lineEditOutFailures->setText(tr("%1").arg(m_ThreadOutSync.m_outFailures));

    ui->lineEditInRate->setText(tr("%1").arg(m_inRate));
    ui->lineEditInBytes->setText(tr("%1").arg(m_curInBytes));
    ui->lineEditInSuccesses->setText(tr("%1").arg(m_ThreadInSync.m_inSeccesses));
    ui->lineEditInFailures->setText(tr("%1").arg(m_ThreadInSync.m_inFailures));

    ui->lineEditInAsyncRate->setText(tr("%1").arg(m_inAsyncRate));
    ui->lineEditInAsyncBytes->setText(tr("%1").arg(m_curInAsyncBytes));
    ui->lineEditInAsyncSuccesses->setText(tr("%1").arg(m_ThreadInAsync.m_inSeccesses));
    ui->lineEditInAsyncFailures->setText(tr("%1").arg(m_ThreadInAsync.m_inFailures));

    ui->lineEditOutAsyncRate->setText(tr("%1").arg(m_outAsyncRate));
    ui->lineEditOutAsyncBytes->setText(tr("%1").arg(m_curOutAsyncBytes));
    ui->lineEditOutAsyncSuccesses->setText(tr("%1").arg(m_ThreadOutAsync.m_outSeccesses));
    ui->lineEditOutAsyncFailures->setText(tr("%1").arg(m_ThreadOutAsync.m_outFailures));

    ui->lineEditLoopOutRate->setText(tr("%1").arg(m_loopOutRate));
    ui->lineEditLoopOutBytes->setText(tr("%1").arg(m_curLoopOutBytes));
    ui->lineEditLoopOutSuccesses->setText(tr("%1").arg(m_ThreadLoopSync.m_outSeccesses));
    ui->lineEditLoopOutFailures->setText(tr("%1").arg(m_ThreadLoopSync.m_outFailures));

    ui->lineEditLoopInRate->setText(tr("%1").arg(m_loopInRate));
    ui->lineEditLoopInBytes->setText(tr("%1").arg(m_curLoopInBytes));
    ui->lineEditLoopInSuccesses->setText(tr("%1").arg(m_ThreadLoopSync.m_inSeccesses));
    ui->lineEditLoopInFailures->setText(tr("%1").arg(m_ThreadLoopSync.m_inFailures));

    ui->lineEditLoopRate->setText(tr("%1").arg(m_loopRate));
    ui->lineEditLoopBytes->setText(tr("%1").arg(m_curLoopBytes));
    ui->lineEditLoopSuccesses->setText(tr("%1").arg(m_ThreadLoopSync.m_loopSeccesses));
    ui->lineEditLoopFailures->setText(tr("%1").arg(m_ThreadLoopSync.m_loopFailures));

    ui->lineEditPcLoop->setText(tr("%1").arg(m_ThreadLoopSync.m_tPcLoop));
    ui->lineEditMaxPcLoop->setText(tr("%1").arg(m_ThreadLoopSync.m_tMaxPcLoop));
    ui->lineEditAvgPcLoop->setText(tr("%1").arg(m_ThreadLoopSync.m_tAvgPcLoop));

    ui->lineEditPlotOutRate->setText(tr("%1").arg(m_plotOutRate));
    ui->lineEditPlotOutBytes->setText(tr("%1").arg(m_curPlotOutBytes));
    ui->lineEditPlotOutSuccesses->setText(tr("%1").arg(m_ThreadPlotSync.m_outSeccesses));
    ui->lineEditPlotOutFailures->setText(tr("%1").arg(m_ThreadPlotSync.m_outFailures));

    ui->lineEditPlotInRate->setText(tr("%1").arg(m_plotInRate));
    ui->lineEditPlotInBytes->setText(tr("%1").arg(m_curPlotInBytes));
    ui->lineEditPlotInSuccesses->setText(tr("%1").arg(m_ThreadPlotSync.m_inSeccesses));
    ui->lineEditPlotInFailures->setText(tr("%1").arg(m_ThreadPlotSync.m_inFailures));

    ui->lineEditPlotRate->setText(tr("%1").arg(m_plotRate));
    ui->lineEditPlotSuccesses->setText(tr("%1").arg(m_ThreadPlotSync.m_plotSeccesses));
    ui->lineEditPlotFailures->setText(tr("%1").arg(m_ThreadPlotSync.m_plotFailures));
}

void MainWindow::on_pushButtonInTxtClear_clicked()
{
    ui->plainTextEditBufferIn->clear();
}

void MainWindow::on_pushButtonOutTxtClear_clicked()
{
    ui->plainTextEditBufferOut->clear();
}
