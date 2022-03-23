#include "taskstablemodel.h"


TasksTableModel::TasksTableModel(QObject *parent) : QStandardItemModel(parent)
{

}

void TasksTableModel::setHeaderLabels()
{
    setHorizontalHeaderLabels(QStringList() << QString("Номер") << QString("Описание") << QString("Текущий статус") << QString("Результат") << QString("Время выполнения, мс"));
}

void TasksTableModel::createNewEntry(SYS::enum_status taskStatus, unsigned int taskId, QString taskDescription, unsigned int taskResult,double task_duration)
{
    QStandardItem *taskIdItem = new QStandardItem();
    QStandardItem *taskStatusItem = new QStandardItem();
    QStandardItem *taskDescriptionItem = new QStandardItem();
    QStandardItem *taskResultItem = new QStandardItem();
    QStandardItem *taskDurationItem = new QStandardItem();
    taskIdItem->setData(taskId,Qt::DisplayRole);
    setItemProperties(taskIdItem);
    taskStatusItem->setData(statusToString(taskStatus),Qt::DisplayRole);
    taskStatusItem->setData(SYS::toUType(taskStatus),SYS::toUType(SYS::table_roles::STATUS_ROLE));
    setItemProperties(taskStatusItem);
    taskDescriptionItem->setData(taskDescription,Qt::DisplayRole);
    setItemProperties(taskDescriptionItem);
    taskResultItem->setData(taskResult,Qt::DisplayRole);
    setItemProperties(taskResultItem);
    taskDurationItem->setData(task_duration,Qt::DisplayRole);
    setItemProperties(taskDurationItem);
    tasksTableItemsDict.insert(taskId,QList<QStandardItem*>() << taskIdItem << taskDescriptionItem << taskStatusItem << taskResultItem << taskDurationItem);
    insertRow(0,tasksTableItemsDict.value(taskId));
}

void TasksTableModel::updateEntry(unsigned int taskId, SYS::enum_status taskStatus, unsigned int taskResult,double task_duration)
{
    tasksTableItemsDict.value(taskId).at(SYS::toUType(SYS::enum_table_headers::RESULT))->setData(taskResult, Qt::DisplayRole);
    tasksTableItemsDict.value(taskId).at(SYS::toUType(SYS::enum_table_headers::STATUS))->setData(statusToString(taskStatus), Qt::DisplayRole);
    tasksTableItemsDict.value(taskId).at(SYS::toUType(SYS::enum_table_headers::STATUS))->setData(SYS::toUType(taskStatus), SYS::toUType(SYS::table_roles::STATUS_ROLE));
    tasksTableItemsDict.value(taskId).at(SYS::toUType(SYS::enum_table_headers::DURATION))->setData(task_duration, Qt::DisplayRole);
}

void TasksTableModel::deleteEntry(int row_numb)
{
    removeRow(row_numb);
}

void TasksTableModel::updateTaskStatus(unsigned int taskId, SYS::enum_status taskStatus)
{
    tasksTableItemsDict.value(taskId).at(SYS::toUType(SYS::enum_table_headers::STATUS))->setData(statusToString(taskStatus), Qt::DisplayRole);
    tasksTableItemsDict.value(taskId).at(SYS::toUType(SYS::enum_table_headers::STATUS))->setData(SYS::toUType(taskStatus), SYS::toUType(SYS::table_roles::STATUS_ROLE));
}

void TasksTableModel::updateTaskResult(unsigned int taskId, unsigned int taskResult)
{
    tasksTableItemsDict.value(taskId).at(SYS::toUType(SYS::enum_table_headers::RESULT))->setData(taskResult, Qt::DisplayRole);
}

SYS::enum_status TasksTableModel::getTaskStatus(unsigned int taskId)
{
    return static_cast<SYS::enum_status>(tasksTableItemsDict.value(taskId).at(SYS::toUType(SYS::enum_table_headers::STATUS))->data(SYS::toUType(SYS::table_roles::STATUS_ROLE)).toInt());
}

void TasksTableModel::setItemProperties(QStandardItem *item)
{
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
}

void TasksTableModel::clearTable()
{
    clear();
    setHeaderLabels();
}

QString TasksTableModel::statusToString (SYS::enum_status value)
{
  switch(value)
  {
    case SYS::enum_status::IN_QUEUE:
      return QString("В очереди");
    case SYS::enum_status::RUNNING:
        return QString("Выполняется");
    case SYS::enum_status::DONE:
        return QString("Завершено");
    case SYS::enum_status::CANCELLED:
        return QString("Отменено");
    default:
      return QString();
  }
}


