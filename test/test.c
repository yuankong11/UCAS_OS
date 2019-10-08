#include "test.h"
#include "test_fs.h"
#include "test_bonus.h"

task_info_t fs_task1_info = {(uint32_t)&test_fs1, USER_PROCESS, 1, "fs_task1"};

task_info_t bonus_task1_info = {(uint32_t)&test_bonus1, USER_PROCESS, 1, "bonus_task1"};

task_info_t *test_tasks[TASK_NUM] = {&fs_task1_info, &bonus_task1_info};
int test_tasks_num = TASK_NUM;