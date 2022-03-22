#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QThread>
#include <QSharedPointer>
#include <QTableView>
#include <QHeaderView>
#include "globals.h"
#include "threadpool.h"
#include "taskstablemodel.h"
#include <iostream>
#include <QMenu>
#include <QAction>
#include <memory>
#include <mutex>
#include <QEventLoop>
#include <QStandardItem>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <deque>
#include <tuple>
#include <vector>
#include <thread>
#include <algorithm>
#include <QTimer>
#include <QStatusBar>
#include <QToolBar>
#include <QIcon>



using namespace std::chrono_literals;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QPushButton *startButton;
    QPushButton *stopButton;
    std::unique_ptr<ThreadPool> threadPool;
    //std::map<unsigned int, std::future<unsigned int>> tasksResults;//assume that all tasks will give unsigned integer results
    std::vector<std::tuple<unsigned int,std::future<unsigned int>>> tasksResults;
    QSharedPointer<TasksTableModel> tasksTableModel;
    QTableView *tasksTableView;
    unsigned int taskNumber;
    std::mutex cout_mutex;
    std::mutex tasks_results_mutex;
    QAction *cancelTaskAction;
    QAction *clearTableAction;
    QToolBar *editTableToolBar;
    QSpinBox *taskValueSpinBox;
    QComboBox *tasksComboBox;
    QSpinBox *numberOfThreadsSpinBox;
    QPushButton *addTaskButton;
    bool finished;
    QTimer *allTasksCompletedTimer;

public:
    MainWindow(QWidget *parent = nullptr);
    void createWidgets();
    void createActions();
    void createToolBar();
    void createConnections();
    void createLayout();
    void createModelAndView();
    unsigned int generateTaskNumber();
    ~MainWindow();
};
#endif // MAINWINDOW_H
