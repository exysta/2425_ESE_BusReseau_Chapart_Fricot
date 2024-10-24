/*
 * shell.h
 *
 *  Created on: Oct 11, 2024
 *      Author: exysta
 */

#ifndef SHELL_H_
#define SHELL_H_



#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cmsis_os.h"

#define TASK_SHELL_STACK_DEPTH 256
#define TASK_SHELL_PRIORITY 1

#define ARGC_MAX 8
#define BUFFER_SIZE 100
#define SHELL_FUNC_LIST_MAX_SIZE 64


struct h_shell_struct;

typedef uint8_t (* drv_shell_transmit_t)(char * pData, uint16_t size);
typedef uint8_t (* drv_shell_receive_t)(char * pData, uint16_t size);

typedef int (*shell_func_pointer_t)(struct h_shell_struct * h_shell, int argc, char ** argv);


// Driver shell structure definition
typedef struct drv_shell_struct
{
    drv_shell_transmit_t drv_shell_transmit;  // Function pointer for transmission
    drv_shell_receive_t drv_shell_receive;    // Function pointer for reception
} drv_shell_t;

// Shell function structure definition
typedef struct shell_func_struct
{
    char *name;                          // Name of the shell function
    shell_func_pointer_t func;           // Function pointer for the shell function
    char *description;                   // Description of the shell function
} shell_func_t;

// Main shell structure definition
typedef struct h_shell_struct
{
    int shell_func_list_size;                      // Size of the function list
    char print_buffer[BUFFER_SIZE];                // Buffer for printing
    shell_func_t shell_func_list[SHELL_FUNC_LIST_MAX_SIZE];  // List of shell functions
    char cmd_buffer[BUFFER_SIZE];                  // Command buffer
    drv_shell_t drv_shell;                         // Driver shell structure
    xTaskHandle h_shell_handle;					   // Pointer to the handle for shell task

} h_shell_t;

extern h_shell_t h_shell; // Declare the global variable for shell struct

BaseType_t shell_createShellTask();
void shell_init(h_shell_t * h_shell);
int shell_add(h_shell_t * h_shell,char * name, shell_func_pointer_t pfunc, char * description);
int shell_run(h_shell_t * h_shell);

#endif /* SHELL_H_ */
