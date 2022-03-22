#include "mainwindow.h"
#include "functions.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), threadPool(std::make_unique<ThreadPool>(cout_mutex,tasks_results_mutex)), taskNumber(0),finished(true)
{
    setWindowTitle(QString("Приложение \"Пул потоков\""));
    // Почему-то максимальное количество потоков равно 1
    {
        std::unique_lock<std::mutex> ul(cout_mutex);
        std::cout << "Maximum number of threads is: " << std::thread::hardware_concurrency() << std::endl;
    }
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
    startButton  =new QPushButton("Старт");
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
    cancelTaskAction = new QAction(QIcon(":/img/images/Button-Close-icon.png"),QString("Отменить задачу"),this);
    clearTableAction = new QAction(QIcon(":/img/images/Clear-icon.png"),QString(""),this);
    clearTableAction->setToolTip(QString("Очистить таблицу"));
}

void MainWindow::createToolBar()
{
    editTableToolBar = new QToolBar(this);
    editTableToolBar->addAction(clearTableAction);
}

void MainWindow::createConnections()
{
    connect(clearTableAction,&QAction::triggered,[&]()
    {
        if(tasksResults.empty())
        {
            tasksTableModel->clearTable();
            tasksTableView->setColumnHidden(0,true); //скрываем идентификаторы задач
            taskNumber = 0;
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
            statusBar()->showMessage(QString("Задачи выполнены"),5000);
        }
    });

    connect(tasksTableView,&QTableView::customContextMenuRequested,[&](const QPoint& pos)
    {
        if(tasksTableView->indexAt(pos).isValid())
        {
            // удалить можно только завершившуюся задачу или задачу, находящуюся в очереди
            QStandardItem *curRowItem = tasksTableModel.data()->itemFromIndex(tasksTableView->currentIndex());
            QStandardItem *taskIdItem = tasksTableModel.data()->item(curRowItem->row(),SYS::toUType(SYS::enum_table_headers::ID));
            cancelTaskAction->setData(taskIdItem->data(Qt::DisplayRole).toInt());
            QMenu menu(this);
            menu.addAction(cancelTaskAction);
            menu.exec(tasksTableView->mapToGlobal(pos));
        }
    });

    connect(cancelTaskAction,&QAction::triggered,[&]()
    {
        auto task_id = cancelTaskAction->data().toInt();
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
            statusBar()->showMessage(QString("Задача отменена"),2000);
        }
    });



    connect(startButton,&QPushButton::clicked,[&]()
    {
        if(finished && !tasksResults.empty())
        {
            //tasksTableModel->clear();
            allTasksCompletedTimer->start();
            threadPool->runThreads(numberOfThreadsSpinBox->value());
            addTaskButton->setEnabled(false);
            finished = false;
            statusBar()->showMessage(QString("Задачи выполняются, подождите..."),3000);
        }
    });

    connect(threadPool.get(),&ThreadPool::updateStatusOfTask,[&](std::tuple<int,SYS::enum_status> task_status)
    {
        {
            std::unique_lock<std::mutex> ul(cout_mutex);
            std::cout << "Task id is: " << std::get<0>(task_status) << "Status is: " << tasksTableModel->statusToString(std::get<1>(task_status)).toStdString() << std::endl;
        }
        tasksTableModel.data()->updateTaskStatus(std::get<0>(task_status),std::get<1>(task_status));
    });

    connect(threadPool.get(),&ThreadPool::updateResultOfTask,[&](int task_id,double duration)
    {
        {
            std::unique_lock<std::mutex> ul(cout_mutex);
            std::cout << "Update result of task with id: " << task_id << " Duration is: " << duration << " ms" << std::endl;
        }
        {
            std::unique_lock<std::mutex> ul(tasks_results_mutex);
            // сначала нужно найти задачу, у которой следует обновить результат
            unsigned int idx{0};
            auto task_itr = tasksResults.begin();
            auto start_itr = tasksResults.begin();
            for(auto itr = tasksResults.begin(); itr != tasksResults.end();++itr)
            {
                auto temp_idx = itr - start_itr;
                {
                    std::unique_lock<std::mutex> ul(cout_mutex);
                    std::cout << "Current idx is: " << temp_idx << std::endl;
                }
                if(std::get<0>(*itr) == task_id)
                {
                    idx = temp_idx;
                    task_itr = itr;
                    {
                        std::unique_lock<std::mutex> ul(cout_mutex);
                        std::cout << "Found idx is: " << idx << std::endl;
                    }
                }
            }
            {
                std::unique_lock<std::mutex> ul(cout_mutex);
                std::cout << "Idx of task is: " << idx << std::endl;
            }

            auto status = std::get<1>(tasksResults.at(idx)).wait_for(0ms);
            if(status == std::future_status::ready)
            {
                auto task_result = std::get<1>(tasksResults.at(idx)).get();
                {
                    std::unique_lock<std::mutex> ul(cout_mutex);
                    std::cout << "Task result is: " << task_result << std::endl;
                }
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
            {
                std::unique_lock<std::mutex> ul(cout_mutex);
                std::cout << "Abort tasks completed" << std::endl;
            }
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
            auto curTaskNumber = generateTaskNumber();
            auto curValue = taskValueSpinBox->value();
            if(tasksComboBox->currentIndex() == 0)//Нахождение факториала
            {
                tasksResults.push_back(std::make_tuple(curTaskNumber, threadPool->addTask( std::make_tuple(curTaskNumber, [=](){ return factorial(curValue); }) )));
                tasksTableModel.data()->createNewEntry(SYS::enum_status::IN_QUEUE, curTaskNumber, QString("Нахождение факториала числа %1").arg(curValue));
                statusBar()->showMessage(QString("Задача добавлена"),1000);
            }

            if(tasksComboBox->currentIndex() == 1)//Нахождение двойного факториала
            {
                tasksResults.push_back(std::make_tuple(curTaskNumber, threadPool->addTask( std::make_tuple(curTaskNumber, [=](){ return double_factorial(curValue); }) )));
                tasksTableModel.data()->createNewEntry(SYS::enum_status::IN_QUEUE, curTaskNumber, QString("Нахождение двойного факториала числа %1").arg(curValue));
                statusBar()->showMessage(QString("Задача добавлена"),1000);
            }

            if(tasksComboBox->currentIndex() == 2)//Нахождение N-го числа Фибоначчи
            {
                tasksResults.push_back(std::make_tuple(curTaskNumber, threadPool->addTask( std::make_tuple(curTaskNumber, [=](){ return fibbonaci_sequence(curValue); }) )));
                tasksTableModel.data()->createNewEntry(SYS::enum_status::IN_QUEUE, curTaskNumber, QString("Нахождение %1-го числа Фибоначчи").arg(curValue));
                statusBar()->showMessage(QString("Задача добавлена"),1000);
            }

            if(tasksComboBox->currentIndex() == 3)//Задержка
            {
                tasksResults.push_back(std::make_tuple(curTaskNumber, threadPool->addTask( std::make_tuple(curTaskNumber, [=](){ return sleep_function(curValue); }) )));
                tasksTableModel.data()->createNewEntry(SYS::enum_status::IN_QUEUE, curTaskNumber, QString("Задержка %1 с").arg(curValue));
                statusBar()->showMessage(QString("Задача добавлена"),1000);
            }
        }
    });

}

void MainWindow::createLayout()
{
    QWidget *mainWindowWidget = new QWidget();
    QGridLayout *mainWindowLayout = new QGridLayout();
    QGroupBox *taskInfoGroupBox = new QGroupBox(QString("Параметры"));
    QGridLayout *taskInfoGroupBoxLayout = new QGridLayout();
    QLabel *taskTypeLabel = new QLabel("Тип задачи");
    QLabel *taskValueLabel = new QLabel("Аргумент задачи");
    QLabel *numberOfThreadsLabel = new QLabel("Количество потоков");
    taskInfoGroupBoxLayout->addWidget(taskTypeLabel,0,0,1,1);
    taskInfoGroupBoxLayout->addWidget(tasksComboBox,0,1,1,1);
    taskInfoGroupBoxLayout->addWidget(taskValueLabel,1,0,1,1);
    taskInfoGroupBoxLayout->addWidget(taskValueSpinBox,1,1,1,1);
    taskInfoGroupBoxLayout->addWidget(numberOfThreadsLabel,2,0,1,1);
    taskInfoGroupBoxLayout->addWidget(numberOfThreadsSpinBox,2,1,1,1);
    taskInfoGroupBoxLayout->addWidget(addTaskButton,3,0,1,2,Qt::AlignCenter);
    taskInfoGroupBox->setLayout(taskInfoGroupBoxLayout);
    mainWindowLayout->addWidget(taskInfoGroupBox,0,0,1,2);
    mainWindowLayout->addWidget(editTableToolBar,1,0,1,2);
    mainWindowLayout->addWidget(tasksTableView,2,0,1,2);
    mainWindowLayout->addWidget(startButton,3,0,1,1,Qt::AlignCenter);
    mainWindowLayout->addWidget(stopButton,3,1,1,1,Qt::AlignCenter);
    mainWindowWidget->setLayout(mainWindowLayout);
    setCentralWidget(mainWindowWidget);
    setMinimumWidth(800);
}

void MainWindow::createModelAndView()
{
    tasksTableView = new QTableView();
    tasksTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tasksTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tasksTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    tasksTableModel.reset(new TasksTableModel());
    tasksTableModel.data()->setHeaderLabels();
    tasksTableView->setModel(tasksTableModel.data());
    tasksTableView->setColumnHidden(0,true); //скрываем идентификаторы задач
    tasksTableView->resizeColumnsToContents();
    QHeaderView *tasksTableHeaderView = tasksTableView->horizontalHeader();
    tasksTableHeaderView->setSectionResizeMode(QHeaderView::Interactive);
    tasksTableHeaderView->setStretchLastSection(true);
    tasksTableHeaderView->setFont(QFont("Tahoma",10,QFont::Bold,false));
}

unsigned int MainWindow::generateTaskNumber()
{
    unsigned int newTaskNumber;
    bool free_number_found{false};
    do
    {
        newTaskNumber = taskNumber++;
        bool number_exists {false};
        for(auto&& taskResult : tasksResults)
        {
            if(std::get<0>(taskResult) == newTaskNumber)
            {
                number_exists = true;
            }
        }
        if(!number_exists)
        {
            free_number_found = true;
        }
    } while(!free_number_found);
    return newTaskNumber;
}

