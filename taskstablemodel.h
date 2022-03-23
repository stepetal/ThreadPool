#ifndef TASKSTABLEMODEL_H
#define TASKSTABLEMODEL_H

#include <QObject>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTextStream>
#include <QFile>
#include <QHash>
#include <QDebug>
#include <QDateTime>
#include <QDataStream>
#include <QUuid>
#include "globals.h"

/*
 *  Класс модели данных для организации задач в таблицу
 */



class TasksTableModel : public QStandardItemModel
{
    Q_OBJECT
private:
    QList<QStandardItem*> tasksTableItems;
    QHash<unsigned int, QList<QStandardItem*>> tasksTableItemsDict;

public:
    explicit TasksTableModel(QObject *parent = nullptr);
    void setHeaderLabels();
    void setItemProperties(QStandardItem *item);
    void createNewEntry(SYS::enum_status taskStatus, unsigned int taskId, QString taskDescription, unsigned int taskResult = 0, double task_duration = 0);
    void updateEntry(unsigned int taskId, SYS::enum_status taskStatus, unsigned int taskResult = 0, double task_duration = 0);
    void deleteEntry(int row_numb);
    void updateTaskStatus(unsigned int taskId, SYS::enum_status taskStatus);
    void updateTaskResult(unsigned int taskId, unsigned int taskResult = 0);
    SYS::enum_status getTaskStatus(unsigned int taskId);
    void clearTable();
    QString statusToString (SYS::enum_status value);
};

#endif // TASKSTABLEMODEL_H
