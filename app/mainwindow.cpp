#include "mainwindow.h"

#include <QSerialPortInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QStringList>
#include <QChart>
#include <QFileDialog>
#include <QMenuBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Mikrobi CanSat");
    resize(1050, 750);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(18);
    mainLayout->setContentsMargins(20,20,20,20);

    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *saveAction = fileMenu->addAction("Save Data As...");
    connect(saveAction, &QAction::triggered, this, &MainWindow::chooseSaveFile);

    QGridLayout *grid = new QGridLayout;
    grid->setSpacing(16);

    createCard("Temperature", tempLabel, grid, 0, 0);
    createCard("Pressure", pressureLabel, grid, 0, 1);
    createCard("Humidity", humidityLabel, grid, 0, 2);

    createCard("DHT Temp", dhtTempLabel, grid, 1, 0);
    createCard("eCO2", eco2Label, grid, 1, 1);
    createCard("TVOC", tvocLabel, grid, 1, 2);

    mainLayout->addLayout(grid);

    setupChart();
    mainLayout->addWidget(chartView);

    QHBoxLayout *portLayout = new QHBoxLayout;

    portSelector = new QComboBox;
    portSelector->setMinimumHeight(36);
    portSelector->setMinimumWidth(220);

    refreshButton = new QPushButton("Refresh");
    refreshButton->setFixedWidth(90);

    connectButton = new QPushButton("Connect");
    connectButton->setFixedWidth(110);

    portLayout->addWidget(portSelector);
    portLayout->addWidget(refreshButton);
    portLayout->addStretch();
    portLayout->addWidget(connectButton);

    mainLayout->addLayout(portLayout);

    setCentralWidget(central);

    setStyleSheet(R"(
        QMainWindow { background-color: #15171b; }

        QFrame#card {
            background-color: #1f2329;
            border-radius: 14px;
            padding: 14px;
        }

        QLabel#title {
            color: #9aa0a6;
            font-size: 13px;
        }

        QLabel#value {
            color: #38bdf8;
            font-size: 23px;
            font-weight: 600;
        }

        QComboBox, QPushButton {
            background-color: #1f2329;
            color: #38bdf8;
            border-radius: 6px;
            padding: 6px;
            font-size: 13px;
        }

        QPushButton:hover {
            background-color: #252a31;
        }
    )");

    refreshPorts();

    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectPort);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(&serial, &QSerialPort::readyRead, this, &MainWindow::readSerial);
}

void MainWindow::createCard(const QString &title, QLabel *&valueLabel,
                            QGridLayout *grid, int row, int col)
{
    QFrame *card = new QFrame;
    card->setObjectName("card");

    QVBoxLayout *layout = new QVBoxLayout(card);

    QLabel *label = new QLabel(title);
    label->setObjectName("title");

    valueLabel = new QLabel("-");
    valueLabel->setObjectName("value");

    layout->addWidget(label);
    layout->addWidget(valueLabel);

    grid->addWidget(card, row, col);
}

void MainWindow::setupChart()
{
    series = new QLineSeries;
    series->setColor(QColor("#38bdf8"));

    QChart *chart = new QChart;
    chart->legend()->hide();
    chart->addSeries(series);
    chart->setBackgroundVisible(false);

    axisX = new QValueAxis;
    axisX->setLabelFormat("%d");
    axisX->setTitleText("Sample");
    axisX->setGridLineVisible(false);

    axisY = new QValueAxis;
    axisY->setLabelFormat("%d");
    axisY->setTitleText("Altitude (m)");
    axisY->setMinorGridLineVisible(false);
    axisY->setGridLineColor(QColor("#2a2f36"));

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    series->attachAxis(axisX);
    series->attachAxis(axisY);

    chartView = new QChartView(chart);
    chartView->setMinimumHeight(300);
    chartView->setRenderHint(QPainter::Antialiasing);
}

void MainWindow::refreshPorts()
{
    portSelector->clear();
    for (const auto &port : QSerialPortInfo::availablePorts())
        portSelector->addItem(port.portName());
}

void MainWindow::connectPort()
{
    if (serial.isOpen())
        serial.close();

    QString portName = portSelector->currentText();
    if (portName.isEmpty()) return;

    serial.setPortName(portName);
    serial.setBaudRate(QSerialPort::Baud115200);

    if (serial.open(QIODevice::ReadOnly))
        connectButton->setText("Connected");
    else
        connectButton->setText("Failed");
}

void MainWindow::chooseSaveFile()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Telemetry",
        "",
        "CSV Files (*.csv)"
    );

    if (!fileName.isEmpty()) {
        logFile.setFileName(fileName);
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            logStream.setDevice(&logFile);
            logStream << "Temp,Pressure,Altitude,eCO2,TVOC,DHTTemp,Humidity\n";
        }
    }
}

void MainWindow::readSerial()
{
    while (serial.canReadLine()) {
        QByteArray line = serial.readLine().trimmed();
        QString data(line);

        if (logFile.isOpen())
            logStream << data << "\n";

        QStringList parts = data.split(',');

        if (parts.size() >= 7) {

            double temp = parts[0].toInt() / 100.0;
            double pressure = parts[1].toInt();
            double altitude = parts[2].toInt() / 100.0;
            double eco2 = parts[3].toInt();
            double tvoc = parts[4].toInt();
            double dhtTemp = parts[5].toInt() / 100.0;
            double hum = parts[6].toInt() / 100.0;

            tempLabel->setText(QString::number(temp,'f',2) + " °C");
            pressureLabel->setText(QString::number(pressure,'f',0) + " Pa");
            humidityLabel->setText(QString::number(hum,'f',1) + " %");

            dhtTempLabel->setText(QString::number(dhtTemp,'f',1) + " °C");
            eco2Label->setText(QString::number(eco2,'f',0) + " ppm");
            tvocLabel->setText(QString::number(tvoc,'f',0) + " ppb");

            updateAltitudeChart(altitude);
        }
    }
}

void MainWindow::updateAltitudeChart(double altitude)
{
    series->append(sampleIndex++, altitude);

    axisX->setRange(0, sampleIndex);

    double minY = axisY->min();
    double maxY = axisY->max();

    if (sampleIndex == 1) {
        axisY->setRange(altitude - 5, altitude + 5);
    } else {
        if (altitude - 2 < minY) minY = altitude - 2;
        if (altitude + 2 > maxY) maxY = altitude + 2;
        axisY->setRange(minY, maxY);
    }

}
