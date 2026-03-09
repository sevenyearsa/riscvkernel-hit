#ifndef EXEC_H
#define EXEC_H

#include "trapframe.h"

unsigned long task_exec(unsigned long epc, struct trapframe *tf);

#endif
