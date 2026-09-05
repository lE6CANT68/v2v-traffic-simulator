#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSlider>
#include <QNetworkAccessManager>

#include <iostream>

class OpenGLWidget; // Déclaration anticipée

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
public slots:
    void mettreAJourInterference();


private:
    OpenGLWidget* glWidget;

    QLabel* statVehicles;
    QLabel* statEdges;
    QLabel* statDegree;

    bool simulationPaused = false;
    int derniereValeurSave = 100;

    QNetworkAccessManager *netManager;

};

#endif // MAINWINDOW_H
