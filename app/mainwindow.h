#pragma once

#include <QMainWindow>
#include <QSerialPort>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void readSerial();
    void connectPort();
    void refreshPorts();
    void chooseSaveFile();

private:
    void createCard(const QString &title, QLabel *&valueLabel, QGridLayout *grid, int row, int col);
    void setupChart();
    void updateAltitudeChart(double altitude);

    QSerialPort serial;

    QLabel *tempLabel;
    QLabel *pressureLabel;
    QLabel *humidityLabel;
    QLabel *dhtTempLabel;
    QLabel *eco2Label;
    QLabel *tvocLabel;

    QComboBox *portSelector;
    QPushButton *connectButton;
    QPushButton *refreshButton;

    QChartView *chartView;
    QLineSeries *series;
    QValueAxis *axisX;
    QValueAxis *axisY;

    int sampleIndex = 0;

    QFile logFile;
    QTextStream logStream;
};
