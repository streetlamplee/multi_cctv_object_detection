/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *getImagebtn;
    QPushButton *Inferencebtn;
    QLabel *FrameLabel;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1440, 960);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        getImagebtn = new QPushButton(centralwidget);
        getImagebtn->setObjectName(QString::fromUtf8("getImagebtn"));
        getImagebtn->setGeometry(QRect(450, 850, 180, 70));
        Inferencebtn = new QPushButton(centralwidget);
        Inferencebtn->setObjectName(QString::fromUtf8("Inferencebtn"));
        Inferencebtn->setGeometry(QRect(810, 850, 180, 70));
        FrameLabel = new QLabel(centralwidget);
        FrameLabel->setObjectName(QString::fromUtf8("FrameLabel"));
        FrameLabel->setGeometry(QRect(60, 60, 1280, 720));
        FrameLabel->setAutoFillBackground(false);
        FrameLabel->setIndent(0);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 1440, 28));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        getImagebtn->setText(QCoreApplication::translate("MainWindow", "Get Image", nullptr));
        Inferencebtn->setText(QCoreApplication::translate("MainWindow", "Inference", nullptr));
        FrameLabel->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
