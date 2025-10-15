#ifndef TASKS_H
#define TASKS_H

void TaskDHT(void *pvParameters);
void TaskSoil(void *pvParameters);
void TaskLight(void *pvParameters);
void TaskRelay(void *pvParameters);
void TaskMQTT(void *pvParameters);

#endif
