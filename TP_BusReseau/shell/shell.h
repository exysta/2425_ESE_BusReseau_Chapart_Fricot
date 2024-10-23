/*
 * shell.h
 *
 *  Created on: Oct 11, 2024
 *      Author: exysta
 */

#ifndef SHELL_H_
#define SHELL_H_



#include <stdint.h>

#define UART_DEVICE_PC huart2
#define UART_DEVICE_PI huart3


#define ARGC_MAX 8
#define BUFFER_SIZE 40
#define SHELL_FUNC_LIST_MAX_SIZE 64

struct h_shell_struct;

typedef int (*shell_func_pointer_t)(struct h_shell_struct * h_shell, int argc, char ** argv);

typedef struct shell_func_struct
{
	char c;
	shell_func_pointer_t func;
	char * description;
} shell_func_t;

typedef struct h_shell_struct
{
	int shell_func_list_size;
	char print_buffer[BUFFER_SIZE];
	shell_func_t shell_func_list[SHELL_FUNC_LIST_MAX_SIZE];
	char cmd_buffer[BUFFER_SIZE];

} h_shell_t;

void shell_init(h_shell_t * h_shell);
int shell_add(h_shell_t * h_shell,char c, shell_func_pointer_t pfunc, char * description);
int shell_run(h_shell_t * h_shell);

#endif /* SHELL_H_ */
