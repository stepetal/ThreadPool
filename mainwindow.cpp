#include "mainwindow.h"
#include "functions.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), threadPool(std::make_unique<ThreadPool>(cout_mutex,tasks_results_mutex)), taskId(0),finished(true)
{
    setWindowTitle(QString(" "));
    #if SILENT_MODE==0
    // Почему-то максимальное количество потоков равно 1
    {
        std::unique_lock<std::mutex> ul(cout_mutex);
        std::cout << "Maximum number of threads is: " << std::thread::hardware_concurrency() << std::endl;
    }
    #endif
    allTasksCompletedTimer = new QTimer(this);
    allTasksCompletedTimer->setInterval(50);
    createWidgets();
    createActions();
    createToolBar();
    createModelAndView();
    createLayout();
    createConnections();
    tasksComboBox->setCurrentIndex(3);
}

MainWindow::~MainWindow()
{

}

void MainWindow::createWidgets()
{
    startButton  = new QPushButton("Старт");
    stopButton = new QPushButton("Стоп");
    taskValueSpinBox = new QSpinBox(this);
    taskValueSpinBox->setMinimum(0);
    numberOfThreadsSpinBox = new QSpinBox(this);
    numberOfThreadsSpinBox->setMinimum(1);
    numberOfThreadsSpinBox->setMaximum(4);
    numberOfThreadsSpinBox->setValue(2);
    numberOfThreadsSpinBox->setToolTip(QString("Значение в диапазоне [1-4]"));
    tasksComboBox = new QComboBox();
    tasksComboBox->addItem("Вычисление факториала числа");
    tasksComboBox->addItem("Вычисление двойного факториала числа");
    tasksComboBox->addItem("Вычисление N-го числа Фибоначчи");
    tasksComboBox->addItem("Задержка, с");
    addTaskButton = new QPushButton("Добавить задачу");
}

void MainWindow::createActions()
{
    cancelTaskAction = new QAction(QIcon(":/img/images/Button-Close-icon.png"), QString("Отменить задачу"), this);
    deleteTaskAction = new QAction(QIcon(":/img/images/Editing-Delete-icon.png"), QString("Удалить задачу"), this);
    clearTableAction = new QAction(QIcon(":/img/images/Clear-icon.png"),QString("Очистить таблицу"),this);
    clearTableAction->setToolTip(QString("Очистить таблицу"));
    cancelTaskAction->setToolTip(QString("Отменить задачу"));
    deleteTaskAction->setToolTip(QString("Удалить задачу"));
}

void MainWindow::createToolBar()
{
    editTableToolBar = new QToolBar(this);
    editTableToolBar->addAction(clearTableAction);
    editTableToolBar->addAction(cancelTaskAction);
    editTableToolBar->addAction(deleteTaskAction);
}

void MainWindow::createConnections()
{
    connect(clearTableAction,&QAction::triggered,[&]()
    {
        if(tasksResults.empty())
        {
            tasksTableModel->clearTable();
            tasksTableView->setColumnHidden(0,true); //скрываем идентификаторы задач
            taskId = 0;
        }
        else
        {
            statusBar()->showMessage("Невозможно очистить таблицу: требуется отмена или завершение текущих задач",2000);
        }
    });

    connect(allTasksCompletedTimer,&QTimer::timeout,[&]()
    {
        //если вектор с задачами пуст, то все задачи выполнены
        if(tasksResults.empty())
        {
            addTaskButton->setEnabled(true);
            finished = true;
            allTasksCompletedTimer->stop();
            threadPool->finishTasks();
            statusBar()->showMessage(QString("Задачи выполнены"));
        }
    });

    connect(tasksTableView,&QTableView::customContextMenuRequested,[&](const QPoint& pos)
    {
        if(tasksTableView->indexAt(pos).isValid())
        {
            QMenu menu(this);
            menu.addAction(cancelTaskAction);
            menu.addAction(deleteTaskAction);
            menu.exec(tasksTableView->mapToGlobal(pos));
        }
    });

    connect(cancelTaskAction,&QAction::triggered,[&]()
    {
        if(tasksTableView->selectionModel()->hasSelection())
        {
            QStandardItem *curRowItem = tasksTableModel.data()->itemFromIndex(tasksTableView->currentIndex());
            QStandardItem *taskIdItem = tasksTableModel.data()->item(curRowItem->row(),SYS::toUType(SYS::enum_table_headers::ID));
            QStandardItem *taskStatusItem = tasksTableModel.data()->item(curRowItem->row(),SYS::toUType(SYS::enum_table_headers::STATUS));
            // если задача в очереди, то отменяем ее
            if(taskStatusItem->data(SYS::toUType(SYS::table_roles::STATUS_ROLE)) == SYS::toUType(SYS::enum_status::IN_QUEUE))
            {
                if(cancelTask(taskIdItem->data(Qt::DisplayRole).toInt()))
                {
                    statusBar()->showMessage(QString("Задача отменена"),2000);
                }
            }
        }
    });

    connect(deleteTaskAction,&QAction::triggered,[&]()
    {
        if(tasksTableView->selectionModel()->hasSelection())
        {
            QStandardItem *curRowItem = tasksTableModel.data()->itemFromIndex(tasksTableView->currentIndex());
            QStandardItem *taskIdItem = tasksTableModel.data()->item(curRowItem->row(),SYS::toUType(SYS::enum_table_headers::ID));
            QStandardItem *taskStatusItem = tasksTableModel.data()->item(curRowItem->row(),SYS::toUType(SYS::enum_table_headers::STATUS));
            switch(taskStatusItem->data(SYS::toUType(SYS::table_roles::STATUS_ROLE)).toInt())
            {
                case SYS::toUType(SYS::enum_status::DONE):
                {
                    tasksTableModel->deleteEntry(tasksTableView->selectionModel()->selectedRows().first().row());
                    break;
                }
                case SYS::toUType(SYS::enum_status::CANCELLED):
                {
                    tasksTableModel->deleteEntry(tasksTableView->selectionModel()->selectedRows().first().row());
                    break;
                }
                case SYS::toUType(SYS::enum_status::IN_QUEUE):
                {
                    if(cancelTask(taskIdItem->data(Qt::DisplayRole).toInt()))
                    {
                        tasksTableModel->deleteEntry(tasksTableView->selectionModel()->selectedRows().first().row());
                        statusBar()->showMessage(QString("Задача удалена"),2000);
                    }
                }
                default:
                    break;
            }
        }
    });

    connect(startButton,&QPushButton::clicked,[&]()
    {
        if(finished && !tasksResults.empty())
        {
            allTasksCompletedTimer->start();
            threadPool->runThreads(numberOfThreadsSpinBox->value());
            addTaskButton->setEnabled(false);
            finished = false;
            statusBar()->showMessage(QString("Задачи выполняются, подождите..."));
        }
    });

    connect(threadPool.get(),&ThreadPool::updateStatusOfTask,[&](std::tuple<int,SYS::enum_status> task_status)
    {
        #if SILENT_MODE==0
        {
            std::unique_lock<std::mutex> ul(cout_mutex);
            std::cout << "Task id is: " << std::get<0>(task_status) << "Status is: " << tasksTableModel->statusToString(std::get<1>(task_status)).toStdString() << std::endl;
        }
        #endif
        tasksTableModel.data()->updateTaskStatus(std::get<0>(task_status),std::get<1>(task_status));
    });

    connect(threadPool.get(),&ThreadPool::updateResultOfTask,[&](int task_id,double duration)
    {
        #if SILENT_MODE==0
        {
            std::unique_lock<std::mutex> ul(cout_mutex);
            std::cout << "Update result of task with id: " << task_id << " Duration is: " << duration << " ms" << std::endl;
        }
        #endif
        {
            std::unique_lock<std::mutex> ul(tasks_results_mutex);
            // сначала нужно найти задачу, у которой следует обновить результат
            unsigned int idx{0};
            auto task_itr = tasksResults.begin();
            auto start_itr = tasksResults.begin();
            for(auto itr = tasksResults.begin(); itr != tasksResults.end();++itr)
            {
                auto temp_idx = itr - start_itr;
                if(std::get<0>(*itr) == task_id)
                {
                    idx = temp_idx;
                    task_itr = itr;
                }
            }
            auto status = std::get<1>(tasksResults.at(idx)).wait_for(0ms);
            if(status == std::future_status::ready)
            {
                auto task_result = std::get<1>(tasksResults.at(idx)).get();
                #if SILENT_MODE==0
                {
                    std::unique_lock<std::mutex> ul(cout_mutex);
                    std::cout << "Task result is: " << task_result << std::endl;
                }
                #endif
                tasksTableModel.data()->updateEntry(std::get<0>(tasksResults.at(idx)),SYS::enum_status::DONE,task_result,duration);
                tasksResults.erase(task_itr);
            }
        }

    });

    connect(threadPool.get(),&ThreadPool::allTasksHaveBeenCompleted,[&]()
    {
        tasksResults.clear();
        addTaskButton->setEnabled(true);
        finished = true;
    });

    connect(stopButton,&QPushButton::clicked,[&]()
    {
        if(!finished && !tasksResults.empty())
        {
            auto tasks_to_cancel = threadPool->getWorkQueueSize();
            int counter{0};
            threadPool->abortTasks();
            finished = true;
            addTaskButton->setEnabled(true);
            allTasksCompletedTimer->stop();
            statusBar()->showMessage(QString("Выполнение задач приостановлено"),1000);
            #if SILENT_MODE==0
            {
                std::unique_lock<std::mutex> ul(cout_mutex);
                std::cout << "Abort tasks completed" << std::endl;
            }
            #endif
            // удаляем все задачи, которые не были завершены, а также обновляем их статус в таблице
            for(auto itr = tasksResults.rbegin(); itr != tasksResults.rend();)
            {
                counter++;
                if(counter == tasks_to_cancel)
                {
                    tasksResults.clear();
                    return;
                }
                else
                {
                    tasksTableModel.data()->updateTaskStatus(std::get<0>(*itr),SYS::enum_status::CANCELLED);
                }
            }
        }
    });

    connect(tasksComboBox,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [&](int idx)
    {
        if(idx == 0) // Нахождение факториала
        {
            taskValueSpinBox->setMaximum(12);
            taskValueSpinBox->setToolTip(QString("Значение в диапазоне [0-12]"));
        }
        else if(idx == 1) // Нахождение двойного факториала
        {
            taskValueSpinBox->setMaximum(12);
            taskValueSpinBox->setToolTip(QString("Значение в диапазоне [0-12]"));
        }
        else if(idx == 2) // Нахождение N-го числа Фибоначчи
        {
            taskValueSpinBox->setMaximum(30);
            taskValueSpinBox->setToolTip(QString("Значение в диапазоне [0-30]"));
        }
        else if(idx == 3) // Задержка
        {
            taskValueSpinBox->setMaximum(100);
            taskValueSpinBox->setToolTip(QString("Значение в диапазоне [0-100]"));
        }
        else
        {
            taskValueSpinBox->setMaximum(100);
            taskValueSpinBox->setToolTip(QString("Значение в диапазоне [0-100]"));
        }
    });


    connect(addTaskButton,&QPushButton::clicked,[&]()
    {
        if(finished)
        {
            auto curTaskId = generateTaskId();
            auto curValue = taskValueSpinBox->value();
            if(tasksComboBox->currentIndex() == 0)//Нахождение факториала
            {
                // подход с методами hide и show для таблицы позволяет адаптировать ее размер под вновь добавленную запись
                tasksTableView->hide();
                tasksResults.push_back(std::make_tuple(curTaskId, threadPool->addTask( std::make_tuple(curTaskId, [=](){ return factorial(curValue); }) )));
                tasksTableModel.data()->createNewEntry(SYS::enum_status::IN_QUEUE, curTaskId, QString("Нахождение факториала числа %1").arg(curValue));
                tasksTableView->show();
                statusBar()->showMessage(QString("Задача добавлена"),1000);
            }

            if(tasksComboBox->currentIndex() == 1)//Нахождение двойного факториала
            {
                // подход с методами hide и show для таблицы позволяет адаптировать ее размер под вновь добавленную запись
                tasksTableView->hide();
                tasksResults.push_back(std::make_tuple(curTaskId, threadPool->addTask( std::make_tuple(curTaskId, [=](){ return double_factorial(curValue); }) )));
                tasksTableModel.data()->createNewEntry(SYS::enum_status::IN_QUEUE, curTaskId, QString("Нахождение двойного факториала числа %1").arg(curValue));
                tasksTableView->show();
                statusBar()->showMessage(QString("Задача добавлена"),1000);
            }

            if(tasksComboBox->currentIndex() == 2)//Нахождение N-го числа Фибоначчи
            {
                // подход с методами hide и show для таблицы позволяет адаптировать ее размер под вновь добавленную запись
                tasksTableView->hide();
                tasksResults.push_back(std::make_tuple(curTaskId, threadPool->addTask( std::make_tuple(curTaskId, [=](){ return fibbonaci_sequence(curValue); }) )));
                tasksTableModel.data()->createNewEntry(SYS::enum_status::IN_QUEUE, curTaskId, QString("Нахождение %1-го числа Фибоначчи").arg(curValue));
                tasksTableView->show();
                statusBar()->showMessage(QString("Задача добавлена"),1000);
            }

            if(tasksComboBox->currentIndex() == 3)//Задержка
            {
                // подход с методами hide и show для таблицы позволяет адаптировать ее размер под вновь добавленную запись
                tasksTableView->hide();
                tasksResults.push_back(std::make_tuple(curTaskId, threadPool->addTask( std::make_tuple(curTaskId, [=](){ return sleep_function(curValue); }) )));
                tasksTableModel.data()->createNewEntry(SYS::enum_status::IN_QUEUE, curTaskId, QString("Задержка %1 с").arg(curValue));
                tasksTableView->show();
                statusBar()->showMessage(QString("Задача добавлена"),1000);
            }
        }
    });

}

void MainWindow::createLayout()
{
    QWidget *mainWindowWidget = new QWidget();
    QGridLayout *mainWindowLayout = new QGridLayout();

    QLabel *title = new QLabel("Приложение \"Пул потоков\"");

    QLabel *numberOfThreadsLabel = new QLabel("Количество потоков");
    QGroupBox *commonParamsGroupBox = new QGroupBox("Общие настройки");
    QGridLayout *commonParamsGroupBoxLayout = new QGridLayout();
    commonParamsGroupBoxLayout->addWidget(numberOfThreadsLabel,0,0,1,1);
    commonParamsGroupBoxLayout->addWidget(numberOfThreadsSpinBox,0,1,1,1);
    commonParamsGroupBox->setLayout(commonParamsGroupBoxLayout);

    QGroupBox *taskInfoGroupBox = new QGroupBox(QString("Настройки задачи"));
    QGridLayout *taskInfoGroupBoxLayout = new QGridLayout();
    QLabel *taskTypeLabel = new QLabel("Тип задачи");
    QLabel *taskValueLabel = new QLabel("Аргумент задачи");
    taskInfoGroupBoxLayout->addWidget(taskTypeLabel,0,0,1,1);
    taskInfoGroupBoxLayout->addWidget(tasksComboBox,0,1,1,1);
    taskInfoGroupBoxLayout->addWidget(taskValueLabel,1,0,1,1);
    taskInfoGroupBoxLayout->addWidget(taskValueSpinBox,1,1,1,1);
    taskInfoGroupBoxLayout->addWidget(addTaskButton,3,0,1,2,Qt::AlignCenter);
    taskInfoGroupBox->setLayout(taskInfoGroupBoxLayout);

    QGroupBox *tasksTableGroupBox = new QGroupBox("");
    QVBoxLayout *tasksTableGroupBoxLayout = new QVBoxLayout();
    tasksTableGroupBoxLayout->addWidget(editTableToolBar);
    tasksTableGroupBoxLayout->addWidget(tasksTableView);
    tasksTableGroupBox->setLayout(tasksTableGroupBoxLayout);

    mainWindowLayout->addWidget(title,0,0,1,2,Qt::AlignCenter);
    mainWindowLayout->addWidget(commonParamsGroupBox,1,0,1,2);
    mainWindowLayout->addWidget(taskInfoGroupBox,2,0,1,2);
    mainWindowLayout->addWidget(tasksTableGroupBox,3,0,1,2);
    mainWindowLayout->addWidget(startButton,4,0,1,1,Qt::AlignCenter);
    mainWindowLayout->addWidget(stopButton,4,1,1,1,Qt::AlignCenter);
    mainWindowWidget->setLayout(mainWindowLayout);

    //установка стилей
    title->setFont(QFont("Arial",16,QFont::Bold,true));
    commonParamsGroupBox->setFont(QFont(QString("Arial"),12,-1,true));
    taskInfoGroupBox->setFont(QFont(QString("Arial"),12,-1,true));
    mainWindowWidget->setStyleSheet("background-color: #0e7fc9; color: white;");
    taskInfoGroupBox->setStyleSheet("background-color: #7ec4f2");
    commonParamsGroupBox->setStyleSheet("background-color: #7ec4f2");
    tasksTableGroupBox->setStyleSheet("background-color: #7ec4f2");
    taskValueSpinBox->setStyleSheet("background-color: white; color: black");
    numberOfThreadsSpinBox->setStyleSheet("background-color: white; color: black");
    tasksTableView->setStyleSheet("background-color: white; color: black");
    tasksComboBox->setStyleSheet("background-color: white; color: black");
    addTaskButton->setStyleSheet("background-color: #0e7fc9; color: white; font-size: 12pt");

    setCentralWidget(mainWindowWidget);
    setMinimumWidth(1000);
}

void MainWindow::createModelAndView()
{
    tasksTableView = new QTableView(this);
    tasksTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tasksTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tasksTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    tasksTableModel.reset(new TasksTableModel());
    tasksTableModel.data()->setHeaderLabels();
    tasksTableView->setModel(tasksTableModel.data());
    tasksTableView->setFont(QFont("Arial",12,-1,true));
    tasksTableView->setColumnHidden(0,true); //скрываем идентификаторы задач
    QHeaderView *tasksTableHeaderView = tasksTableView->horizontalHeader();
    tasksTableHeaderView->setSectionResizeMode(QHeaderView::ResizeToContents);
    tasksTableHeaderView->setStretchLastSection(true);
    tasksTableHeaderView->setFont(QFont("Arial",12,QFont::Bold,true));
}

unsigned int MainWindow::generateTaskId()
{
    unsigned int newTaskId;
    bool free_number_found{false};
    do
    {
        newTaskId = taskId++;
        bool number_exists {false};
        for(auto&& taskResult : tasksResults)
        {
            if(std::get<0>(taskResult) == newTaskId)
            {
                number_exists = true;
            }
        }
        if(!number_exists)
        {
            free_number_found = true;
        }
    } while(!free_number_found);
    return newTaskId;
}

bool MainWindow::cancelTask(unsigned int task_id)
{
    if(threadPool->taskInQueue(task_id))
    {
        // удаляем задачу из пула и из вектора с задачами
        auto task_itr = tasksResults.begin();
        for(auto itr = tasksResults.begin(); itr != tasksResults.end();++itr)
        {
            if(std::get<0>(*itr) == task_id)
            {
                task_itr = itr;
            }
        }
        tasksResults.erase(task_itr);
        threadPool->cancelTask(task_id);
        return true;
    }
    return false;
}

