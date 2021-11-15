#include "plot.h"
#include "ui_plot.h"

Plot::Plot(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Plot),
    //������ɫ��ʼ�� 14��
    line_colors{
      /* For channel data (gruvbox palette) */
      /* Light */
      QColor ("#fb4934"),
      QColor ("#b8bb26"),
      QColor ("#fabd2f"),
      QColor ("#83a598"),
      QColor ("#d3869b"),
      QColor ("#8ec07c"),
      QColor ("#fe8019"),
      /* Light */
      QColor ("#cc241d"),
      QColor ("#98971a"),
      QColor ("#d79921"),
      QColor ("#458588"),
      QColor ("#b16286"),
      QColor ("#689d6a"),
      QColor ("#d65d0e"),
    },
    //����GUI��ɫ��ʼ�� 4��
    gui_colors {
      QColor (48,  47,  47,  255), /**<  0: qdark ui dark/background color */
      QColor (80,  80,  80,  255), /**<  1: qdark ui medium/grid color */
      QColor (170, 170, 170, 255), /**<  2: qdark ui light/text color */
      QColor (48,  47,  47,  200)  /**<  3: qdark ui dark/background color w/transparency */
    },
    //������ʼ��
    isPlotting(false),
    isTrackAixs(true),
    isYAutoScale(false),
    dataPointNumber (0),
    channelNumber(0)
{
    ui->setupUi(this);
    setWindowTitle("Plot");

    //��ʼ��Plot�������
    setupPlot();

    //���Ӳۺ���-ʵ��״̬����ʾ����xy����
    connect (ui->plot, SIGNAL (mouseMove (QMouseEvent*)), this, SLOT (slot_plot_mouseMove(QMouseEvent*)));
    //���Ӳۺ���-ʵ��plot�����߿�ѡ��
    connect (ui->plot, SIGNAL(selectionChangedByUser()), this, SLOT(slot_plot_selectionChangedByUser()));
    //���Ӳۺ���-ʵ��˫��legend���޸�����
    connect (ui->plot, SIGNAL(legendDoubleClick (QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)), this, SLOT(slot_plot_legendDoubleClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)));
    //���Ӳۺ���-ʵ�ֶ�ʱreplot������������ͼ��ˢ�·ֿ�
    connect (&timerUpdatePlot, SIGNAL (timeout()), this, SLOT (slot_timerUpdatePlotr_timeout()));
    //���Ӳۺ���-ʵ��������仯ֵͬ����spinBox��scrollBar
    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(slot_QCPXAxis_rangeChanged(QCPRange)));
    connect(ui->plot->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(slot_QCPYAxis_rangeChanged(QCPRange)));
}

Plot::~Plot()
{
    delete ui;
}

//------------------------------------------------------------------
//��ʼ��Plot�������
void Plot::setupPlot()
{
    //���ͼ����������Ŀ
    ui->plot->clearItems();

    //���ñ�����ɫ
    ui->plot->setBackground (gui_colors[0]);

    //���ڸ�����ģʽ��ȥ������ݣ� (see QCustomPlot real time example)
    ui->plot->setNotAntialiasedElements (QCP::aeAll);
    QFont font;
    font.setStyleStrategy (QFont::NoAntialias);
    ui->plot->legend->setFont (font);

    //See QCustomPlot examples / styled demo
    /* X Axis: Style */
    ui->plot->xAxis->grid()->setPen (QPen(gui_colors[2], 1, Qt::DotLine));
    ui->plot->xAxis->grid()->setSubGridPen (QPen(gui_colors[1], 1, Qt::DotLine));
    ui->plot->xAxis->grid()->setSubGridVisible (true);
    ui->plot->xAxis->setBasePen (QPen (gui_colors[2]));
    ui->plot->xAxis->setTickPen (QPen (gui_colors[2]));
    ui->plot->xAxis->setSubTickPen (QPen (gui_colors[2]));
    ui->plot->xAxis->setUpperEnding (QCPLineEnding::esSpikeArrow);
    ui->plot->xAxis->setTickLabelColor (gui_colors[2]);
    ui->plot->xAxis->setTickLabelFont (font);
    // ��Χ
    ui->plot->xAxis->setRange (dataPointNumber - ui->spinBoxXPoints->value(), dataPointNumber);
    ui->spinBoxXCurPos->setEnabled(false);
    // ��������������
    ui->plot->xAxis->setLabel("X");

    /* Y Axis */
    ui->plot->yAxis->grid()->setPen (QPen(gui_colors[2], 1, Qt::DotLine));
    ui->plot->yAxis->grid()->setSubGridPen (QPen(gui_colors[1], 1, Qt::DotLine));
    ui->plot->yAxis->grid()->setSubGridVisible (true);
    ui->plot->yAxis->setBasePen (QPen (gui_colors[2]));
    ui->plot->yAxis->setTickPen (QPen (gui_colors[2]));
    ui->plot->yAxis->setSubTickPen (QPen (gui_colors[2]));
    ui->plot->yAxis->setUpperEnding (QCPLineEnding::esSpikeArrow);
    ui->plot->yAxis->setTickLabelColor (gui_colors[2]);
    ui->plot->yAxis->setTickLabelFont (font);
    // ��Χ
    ui->plot->yAxis->setRange (ui->spinBoxYMin->value(), ui->spinBoxYMax->value());
    // ��������������
    ui->plot->yAxis->setLabel("Y");

    // ͼ����
    ui->plot->setInteraction (QCP::iRangeDrag, true);           //�ɵ�����ק
    ui->plot->setInteraction (QCP::iRangeZoom, true);           //�ɹ�������
    ui->plot->setInteraction (QCP::iSelectPlottables, true);    //ͼ�����ݿ�ѡ��
    ui->plot->setInteraction (QCP::iSelectLegend, true);        //ͼ����ѡ
    ui->plot->axisRect()->setRangeDrag (Qt::Horizontal|Qt::Vertical);  //ˮƽ��ק
    ui->plot->axisRect()->setRangeZoom (Qt::Vertical);        //ˮƽ����

    // ͼ������
    QFont legendFont;
    legendFont.setPointSize (9);
    ui->plot->legend->setVisible (true);
    ui->plot->legend->setFont (legendFont);
    ui->plot->legend->setBrush (gui_colors[3]);
    ui->plot->legend->setBorderPen (gui_colors[2]);
    // ͼ��λ�ã����Ͻ�
    ui->plot->axisRect()->insetLayout()->setInsetAlignment (0, Qt::AlignTop|Qt::AlignRight);
}

//------------------------------------------------------------------
// plot������д��������ˢ�·ֿ�
// ���ݿ��8bit
void Plot::onNewDataArrived(QByteArray baRecvData)
{
    static int num = 0;
    static int channelIndex = 0;

    if (isPlotting){
        num = baRecvData.size();

        for (int i = 0; i < num; i++){
            if(ui->plot->plottableCount() <= channelIndex){
                /* Add new channel data */
                ui->plot->addGraph();
                ui->plot->graph()->setPen (line_colors[channelNumber % CUSTOM_LINE_COLORS]);
                ui->plot->graph()->setName (QString("Channel %1").arg(channelNumber));
                if(ui->plot->legend->item(channelNumber)){
                    ui->plot->legend->item (channelNumber)->setTextColor (line_colors[channelNumber % CUSTOM_LINE_COLORS]);
                }
                ui->listWidgetChannels->addItem(ui->plot->graph()->name());
                ui->listWidgetChannels->item(channelIndex)->setForeground(QBrush(line_colors[channelNumber % CUSTOM_LINE_COLORS]));
                channelNumber++;
            }

            /* Add data to Graph */
            ui->plot->graph(channelIndex)->addData (dataPointNumber, baRecvData[channelIndex]);
            /* Increment data number and channel */
            channelIndex++;
        }

        dataPointNumber++;
        channelIndex = 0;
      }
}

//------------------------------------------------------------------
//״̬����ʾ����xy����
void Plot::slot_plot_mouseMove(QMouseEvent *event)
{
    int xPos = int(ui->plot->xAxis->pixelToCoord(event->x()));
    int yPos = int(ui->plot->yAxis->pixelToCoord(event->y()));
    QString coordinates = tr("X: %1 Y: %2").arg(xPos).arg(yPos);
    ui->statusBar->showMessage(coordinates);
}

//------------------------------------------------------------------
//plot�����ߺ�ͼ����ѡ��
void Plot::slot_plot_selectionChangedByUser (void)
{
    /* synchronize selection of graphs with selection of corresponding legend items */
     for (int i = 0; i < ui->plot->graphCount(); i++)
       {
         QCPGraph *graph = ui->plot->graph(i);
         QCPPlottableLegendItem *item = ui->plot->legend->itemWithPlottable (graph);
         if (item->selected())
           {
             item->setSelected (true);
   //          graph->set (true);
           }
         else
           {
             item->setSelected (false);
     //        graph->setSelected (false);
           }
       }
}

//------------------------------------------------------------------
// ˫��legend����Ŀ������������������
void Plot::slot_plot_legendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event)
{
    Q_UNUSED (legend)
    Q_UNUSED(event)
    /* Only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0) */
    if (item)
      {
        QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
        bool ok;
        QString newName = QInputDialog::getText (this, "Change channel name", "New name:", QLineEdit::Normal, plItem->plottable()->name(), &ok, Qt::Popup);
        if (ok)
          {
            plItem->plottable()->setName(newName);
            for(int i=0; i<ui->plot->graphCount(); i++)
            {
                ui->listWidgetChannels->item(i)->setText(ui->plot->graph(i)->name());
            }
            ui->plot->replot(QCustomPlot::rpQueuedReplot);
          }
      }
}

//------------------------------------------------------------------
// ��ʱreplotͼ����������д��ˢ��ͼ��ֿ�
void Plot::slot_timerUpdatePlotr_timeout()
{
    if(isTrackAixs){
        ui->plot->xAxis->setRange (dataPointNumber - ui->spinBoxXPoints->value(), dataPointNumber);
    }

    if(isYAutoScale)
        on_pushButtonYAutoScale_clicked();

    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// ͬ��X��ķ�Χ�仯��spinBox��scrollBar
void Plot::slot_QCPXAxis_rangeChanged(QCPRange xNewRange)
{
    ui->spinBoxXCurPos->setValue(xNewRange.lower);
    ui->spinBoxXPoints->setValue(xNewRange.upper - xNewRange.lower);

    ui->horizontalScrollBar->setRange(0, dataPointNumber);
    ui->horizontalScrollBar->setPageStep(xNewRange.upper - xNewRange.lower);
    ui->horizontalScrollBar->setValue(xNewRange.lower);
}

//------------------------------------------------------------------
// ͬ��Y��ķ�Χ�仯��spinBox
void Plot::slot_QCPYAxis_rangeChanged(QCPRange yNewRange)
{
    ui->spinBoxYMax->setValue(yNewRange.upper);
    ui->spinBoxYMin->setValue(yNewRange.lower);
}

//------------------------------------------------------------------
// �Ƿ����X�������ʾ
void Plot::on_checkBoxXTrackAixs_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked){
        isTrackAixs = true;
        ui->spinBoxXCurPos->setEnabled(false);
    }
    else{
        isTrackAixs = false;
        ui->spinBoxXCurPos->setEnabled(true);
    }
}

//------------------------------------------------------------------
// �Ƿ�����ӦY����ֵ
void Plot::on_checkBoxYAutoScale_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked){
        isYAutoScale = true;
        ui->spinBoxYMin->setEnabled(false);
        ui->spinBoxYMax->setEnabled(false);
    }
    else{
        isYAutoScale = false;
        ui->spinBoxYMin->setEnabled(true);
        ui->spinBoxYMax->setEnabled(true);
    }
}

//------------------------------------------------------------------
// ����ӦY����ֵ1��
void Plot::on_pushButtonYAutoScale_clicked()
{
    ui->plot->yAxis->rescale(true);
    int newYMax = int(ui->plot->yAxis->range().upper*1.1);
    int newYMin = int(ui->plot->yAxis->range().lower*1.1);
    ui->plot->yAxis->setRange(newYMin,newYMax);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// spinBoxXCurPosֵ�仯ͬ����x����Сֵ
void Plot::on_spinBoxXCurPos_valueChanged(int arg1)
{
    double newXMin = arg1;
    double newXNumber = ui->plot->xAxis->range().upper - ui->plot->xAxis->range().lower;
    ui->plot->xAxis->setRange (newXMin, newXMin+newXNumber);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// spinBoxXPointsֵ�仯ͬ����x�ᴰ����ʾ����
void Plot::on_spinBoxXPoints_valueChanged(int arg1)
{
    double newXMin = ui->plot->xAxis->range().lower;
    double newXNumber = arg1;
    ui->plot->xAxis->setRange (newXMin, newXMin+newXNumber);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// spinBoxYMinֵ�仯ͬ����Y����Сֵ
void Plot::on_spinBoxYMin_valueChanged(int arg1)
{
    ui->plot->yAxis->setRangeLower (arg1);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// spinBoxYMaxֵ�仯ͬ����Y�����ֵ
void Plot::on_spinBoxYMax_valueChanged(int arg1)
{
    ui->plot->yAxis->setRangeUpper (arg1);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// spinBoxXTicksֵ�仯ͬ����X��tick
void Plot::on_spinBoxXTicks_valueChanged(int arg1)
{
    ui->plot->xAxis->ticker()->setTickCount(arg1);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// spinBoxYTicksֵ�仯ͬ����Y��tick
void Plot::on_spinBoxYTicks_valueChanged(int arg1)
{
    ui->plot->yAxis->ticker()->setTickCount(arg1);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// �������š������ק��XY��ֱ��趨
void Plot::on_radioButtonRangeZoomX_toggled(bool checked)
{
    if(checked)
        ui->plot->axisRect()->setRangeZoom (Qt::Horizontal);
}

void Plot::on_radioButtonRangeZoomY_toggled(bool checked)
{
    if(checked)
        ui->plot->axisRect()->setRangeZoom (Qt::Vertical);
}

void Plot::on_radioButtonRangeZoomXY_toggled(bool checked)
{
    if(checked)
        ui->plot->axisRect()->setRangeZoom (Qt::Horizontal|Qt::Vertical);
}

void Plot::on_radioButtonRangeDragX_toggled(bool checked)
{
    if(checked)
        ui->plot->axisRect()->setRangeDrag (Qt::Horizontal);
}

void Plot::on_radioButtonRangeDragY_toggled(bool checked)
{
    if(checked)
        ui->plot->axisRect()->setRangeDrag (Qt::Vertical);
}

void Plot::on_radioButtonRangeDragXY_toggled(bool checked)
{
    if(checked)
        ui->plot->axisRect()->setRangeDrag (Qt::Horizontal|Qt::Vertical);
}

//------------------------------------------------------------------
// ͬ��ˮƽ������仯ֵ��X����Сֵ
void Plot::on_horizontalScrollBar_valueChanged(int value)
{
    double newXMin = value;
    double newXNumber = ui->plot->xAxis->range().upper - ui->plot->xAxis->range().lower;
    ui->plot->xAxis->setRange (newXMin, newXMin+newXNumber);
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// ��ʾͼ��
void Plot::on_checkBoxShowLegend_stateChanged(int arg1)
{
    if(arg1){
        ui->plot->legend->setVisible(true);
    }else{
        ui->plot->legend->setVisible(false);
    }
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// ��ʾ�������ߣ��������������ߣ�
void Plot::on_pushButtonShowAllCurve_clicked()
{
    for(int i=0; i<ui->plot->graphCount(); i++)
    {
        ui->plot->graph(i)->setVisible(true);
        ui->listWidgetChannels->item(i)->setBackground(Qt::NoBrush);
    }
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// ����������ߣ������ʷ���ݣ�
void Plot::on_pushButtonClearAllCurve_clicked()
{
    //pPlot1->graph(i)->data().data()->clear(); // ����������ߵ�����
    //pPlot1->clearGraphs();                    // ���ͼ����������ݺ����ã���Ҫ�������ò������»�ͼ
    ui->plot->clearPlottables();                // ���ͼ�����������ߣ���Ҫ����������߲��ܻ�ͼ
    ui->listWidgetChannels->clear();
    channelNumber = 0;
    dataPointNumber = 0;
    setupPlot();
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// ��ʼ�������ݲ�������ʱˢ�¶�ʱ��
void Plot::on_pushButtonStartPlot_clicked()
{
    if(ui->pushButtonStartPlot->isChecked()){
        //������ͼ
        timerUpdatePlot.start(20);
        isPlotting = true;
        ui->pushButtonStartPlot->setText(tr("StopPlot"));
    }
    else{
        //ֹͣ��ͼ
        timerUpdatePlot.stop();
        isPlotting = false;
        ui->pushButtonStartPlot->setText(tr("StartPlot"));
    }
}

//------------------------------------------------------------------
// ��exeĿ¼�±��沨�ν�ͼ
void Plot::on_pushButtonSavePng_clicked()
{
    ui->plot->savePng (QString::number(dataPointNumber) + ".png", 1920, 1080, 2, 50);
}

//------------------------------------------------------------------
// ˫���б������������/��ʾplot���ߣ����������߱���Ϊ��ɫ��
void Plot::on_listWidgetChannels_itemDoubleClicked(QListWidgetItem *item)
{
    int graphIdx = ui->listWidgetChannels->currentRow();

    if(ui->plot->graph(graphIdx)->visible()){
        ui->plot->graph(graphIdx)->setVisible(false);
        item->setBackground(Qt::black);
    }
    else{
        ui->plot->graph(graphIdx)->setVisible(true);
        item->setBackground(Qt::NoBrush);
    }
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// ����ѡ���б����Ŀ�����¶�Ӧ������Ϣ
void Plot::on_listWidgetChannels_currentRowChanged(int currentRow)
{
    //���߿ɼ�
    if(ui->plot->graph(currentRow)->visible())
        ui->checkBoxCurveVisible->setCheckState(Qt::Checked);
    else
        ui->checkBoxCurveVisible->setCheckState(Qt::Unchecked);

    //���߼Ӵ�
    if(ui->plot->graph(currentRow)->pen().width() == 3)
        ui->checkBoxCurveBold->setCheckState(Qt::Checked);
    else
        ui->checkBoxCurveBold->setCheckState(Qt::Unchecked);

    //��ɫ����
    //��ȡ��ǰ��ɫ
    QColor curColor = ui->plot->graph(currentRow)->pen().color();
    //����ѡ�����ɫ
    ui->pushButtonCurveColor->setStyleSheet(QString("border:0px solid;background-color: %1;").arg(curColor.name()));

    //��������
    QCPGraph::LineStyle lineStyle = ui->plot->graph(currentRow)->lineStyle();
    ui->comboBoxCurveLineStyle->setCurrentIndex(int(lineStyle));

    //ɢ����ʽ
    QCPScatterStyle::ScatterShape scatterShape = ui->plot->graph(currentRow)->scatterStyle().shape();
    ui->comboBoxCurveScatterStyle->setCurrentIndex(int(scatterShape));
}

//------------------------------------------------------------------
// ���߿ɼ�/����
void Plot::on_checkBoxCurveVisible_stateChanged(int arg1)
{
    int graphIdx = ui->listWidgetChannels->currentRow();
    if(graphIdx<0 || graphIdx>channelNumber)
        return;

    if(arg1 == Qt::Checked){
        ui->plot->graph(graphIdx)->setVisible(true);
        ui->listWidgetChannels->item(graphIdx)->setBackground(Qt::NoBrush);
    }
    else{
        ui->plot->graph(graphIdx)->setVisible(false);
        ui->listWidgetChannels->item(graphIdx)->setBackground(Qt::black);
    }
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// ���߼Ӵ�
void Plot::on_checkBoxCurveBold_stateChanged(int arg1)
{
    int graphIdx = ui->listWidgetChannels->currentRow();
    if(graphIdx<0 || graphIdx>channelNumber)
        return;

    // Ԥ�ȶ�ȡ���ߵ���ɫ
    QPen pen = ui->plot->graph(graphIdx)->pen();

    if(arg1 == Qt::Checked)
        pen.setWidth(3);
    else
        pen.setWidth(1);

    ui->plot->graph(graphIdx)->setPen(pen);

    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}
//------------------------------------------------------------------
// ������ɫ����
void Plot::on_pushButtonCurveColor_clicked()
{
    int graphIdx = ui->listWidgetChannels->currentRow();
    if(graphIdx<0 || graphIdx>channelNumber)
        return;

    // ��ȡ��ǰ��ɫ
    QColor curColor = ui->plot->graph(graphIdx)->pen().color();// ��curve���߻����ɫ
    // �Ե�ǰ��ɫ�򿪵�ɫ�壬�����󣬱��⣬��ɫ�Ի����������ʾAlpha͸����ͨ����
    QColor color = QColorDialog::getColor(curColor, this,
                                     tr("��ɫ�Ի���"),
                                     QColorDialog::ShowAlphaChannel);
    // �жϷ��ص���ɫ�Ƿ�Ϸ��������x�ر���ɫ�Ի��򣬻᷵��QColor(Invalid)��Чֵ��ֱ��ʹ�ûᵼ�±�Ϊ��ɫ��
    if(color.isValid()){
        // ����ѡ�����ɫ
        ui->pushButtonCurveColor->setStyleSheet(QString("border:0px solid;background-color: %1;").arg(color.name()));
        // ����������ɫ
        QPen pen = ui->plot->graph(graphIdx)->pen();
        pen.setBrush(color);
        ui->plot->graph(graphIdx)->setPen(pen);
        // ����legend�ı���ɫ
        ui->plot->legend->item (graphIdx)->setTextColor(color);
        // ����listwidget�ı���ɫ
        ui->listWidgetChannels->item(graphIdx)->setForeground(QBrush(color));
    }

    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// ������ʽѡ�񣨵㡢�ߡ����ҡ��С�����
void Plot::on_comboBoxCurveLineStyle_currentIndexChanged(int index)
{
    int graphIdx = ui->listWidgetChannels->currentRow();
    if(graphIdx<0 || graphIdx>channelNumber)
        return;

    ui->plot->graph(graphIdx)->setLineStyle((QCPGraph::LineStyle)index);

    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

//------------------------------------------------------------------
// ɢ����ʽѡ�񣨿���Բ��ʵ��Բ�������ǡ������ǣ�
void Plot::on_comboBoxCurveScatterStyle_currentIndexChanged(int index)
{
    int graphIdx = ui->listWidgetChannels->currentRow();
    if(graphIdx<0 || graphIdx>channelNumber)
        return;

    if(index <= 10){
        ui->plot->graph(graphIdx)->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)index, 5)); // ɢ����ʽ
    }else{ // �����ɢ��ͼ���Ը��ӣ�̫С�ῴ����
        ui->plot->graph(graphIdx)->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)index, 8)); // ɢ����ʽ
    }

    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}
