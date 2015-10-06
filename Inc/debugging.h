#ifndef DEBUGGING_H
#define DEBUGGING_H

void task_switch_in(const char* const name);
void task_switch_out(const char* const name);
void print_stack_trace();

#endif // DEBUGGING_H

