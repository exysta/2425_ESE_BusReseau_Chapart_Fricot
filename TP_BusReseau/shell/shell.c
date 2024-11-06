/*
 * shell.c
 *
 *  Created on: Oct 11, 2024
 *      Author: exysta
 */

#include "shell.h"

#include "drv_uart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

h_shell_t h_shell;

void task_shell(void * unused)
{
	while(1)
	{
		shell_run(&h_shell); //infinie donc la task ne se finie jamais
		// Delay or yield to allow other tasks to run
		vTaskDelay(pdMS_TO_TICKS(10));  // 10 ms delay for yielding
	}
	// We should never reach this point
	// If the task ends, delete it
	vTaskDelete(0);
}

BaseType_t shell_createShellTask(h_shell_t * h_shell)
{
	return xTaskCreate(task_shell, "task_shell", TASK_SHELL_STACK_DEPTH, NULL, TASK_SHELL_PRIORITY, &h_shell->h_shell_handle);
}

static int sh_help(h_shell_t * h_shell,int argc, char ** argv) {
	int i;
	for(i = 0 ; i < h_shell->shell_func_list_size ; i++) {
		int size;
		memset(h_shell->print_buffer, 0, BUFFER_SIZE);
		size = snprintf (h_shell->print_buffer, BUFFER_SIZE, "%s: %s\r\n", h_shell->shell_func_list[i].name, h_shell->shell_func_list[i].description);
		h_shell->drv_shell.drv_shell_transmit(h_shell->print_buffer, size);
        drv_uart_waitTransmitComplete();  // Wait for prompt transmission to complete

	}

	return 0;
}

void shell_init(h_shell_t * h_shell)
{
	int size = 0;
	h_shell->shell_func_list_size = 0;
	h_shell->drv_shell.drv_shell_receive = drv_uart_receive ;
	h_shell->drv_shell.drv_shell_transmit = drv_uart_transmit ;

	size = snprintf (h_shell->print_buffer, BUFFER_SIZE, "\r\n\r\n===== 🐢 Wise Tortoise Shell v0.2 🐢 ===== \r\n");
	h_shell->drv_shell.drv_shell_transmit(h_shell->print_buffer, size);
    drv_uart_waitTransmitComplete();  // Wait for prompt transmission to complete


	shell_add(h_shell,"help", sh_help, "Help");
}

int shell_add(h_shell_t * h_shell,char * name, shell_func_pointer_t pfunc, char * description) {
	if (h_shell->shell_func_list_size < SHELL_FUNC_LIST_MAX_SIZE) {
		h_shell->shell_func_list[h_shell->shell_func_list_size].name = name;
		h_shell->shell_func_list[h_shell->shell_func_list_size].func = pfunc;
		h_shell->shell_func_list[h_shell->shell_func_list_size].description = description;
		h_shell->shell_func_list_size++;
		return 0;
	}

	return -1;
}

static int shell_exec(h_shell_t *h_shell) {
	if (h_shell == NULL )
	{
		return -1; // Invalid parameters
	}

	char buf[BUFFER_SIZE];
	char *token;
	char *argv[ARGC_MAX];
	int argc = 0;

	// Ensure cmd_buffer is copied safely
	strncpy(buf, h_shell->cmd_buffer, BUFFER_SIZE - 1);
	buf[BUFFER_SIZE - 1] = '\0'; // null-terminate to avoid overflow

	// Get the first token separated by space
	token = strtok(buf, " ");
	while (token != NULL && argc < ARGC_MAX) {
		argv[argc] = strdup(token); // allocate memory for each token
		if (argv[argc] == NULL) {
			// Memory allocation failed, clean up and return
			for (int j = 0; j < argc; j++) {
				free(argv[j]);
			}
			snprintf(h_shell->print_buffer, BUFFER_SIZE, "Error: Memory allocation failed\r\n");
			h_shell->drv_shell.drv_shell_transmit(h_shell->print_buffer, strlen(h_shell->print_buffer));
	        drv_uart_waitTransmitComplete();  // Wait for prompt transmission to complete

			return -1;
		}
		argc++; // increment argument count

		token = strtok(NULL, " "); // get next token
	}

	if (argc == 0) {
		snprintf(h_shell->print_buffer, BUFFER_SIZE, "Error: No command entered\r\n");
		h_shell->drv_shell.drv_shell_transmit(h_shell->print_buffer, strlen(h_shell->print_buffer));
        drv_uart_waitTransmitComplete();  // Wait for prompt transmission to complete

		return -1;
	}

	// Check if user_func is valid and perform command lookup
	char *user_func = argv[0]; // first token is the command
	for (int i = 0; i < h_shell->shell_func_list_size; i++) {
		if (strcmp(h_shell->shell_func_list[i].name, user_func) == 0) {
			// Execute the command
			int result = h_shell->shell_func_list[i].func(h_shell, argc, argv);

			// Clean up dynamically allocated memory before returning
			for (int j = 0; j < argc; j++) {
				free(argv[j]);
			}
			return result;
		}
	}

	// If no matching function is found
	snprintf(h_shell->print_buffer, BUFFER_SIZE, "%s: no such command\r\n", user_func);
	h_shell->drv_shell.drv_shell_transmit(h_shell->print_buffer, strlen(h_shell->print_buffer));
    drv_uart_waitTransmitComplete();  // Wait for prompt transmission to complete

	// Clean up allocated memory before returning
	for (int j = 0; j < argc; j++) {
		free(argv[j]);
	}
	return -1;
}

static char backspace[] = "\b \b";
static char prompt[] = "> ";

//non-blocking shell run
int shell_run(h_shell_t *h_shell) {
    static int pos = 0;              // Position in the command buffer
    static int reading = 0;          // Reading state
    char c;
    int size;

    // State 1: Show prompt if not already reading input
    if (!reading)
    {
        h_shell->drv_shell.drv_shell_transmit(prompt, 2); // Send the prompt
        drv_uart_waitTransmitComplete();  // Wait for prompt transmission to complete
        reading = 1;  // Switch to reading mode
    }

    // State 2: Check for received character (non-blocking)
    if (drv_uart_receive(&c, 1)) {
        drv_uart_waitReceiveComplete();  // Wait for character reception

        // Process the received character
        switch (c) {
            case '\r':  // Process RETURN key
                if (pos > 0)// Only process if there's something in the buffer
                {
                    size = snprintf(h_shell->print_buffer, BUFFER_SIZE, "\r\n");
                    h_shell->drv_shell.drv_shell_transmit(h_shell->print_buffer, size);
                    drv_uart_waitTransmitComplete();  // Wait for transmission to complete

                    h_shell->cmd_buffer[pos++] = 0;  // Add NULL terminator to the command buffer
                    size = snprintf(h_shell->print_buffer, BUFFER_SIZE, ":%s\r\n", h_shell->cmd_buffer);
                    h_shell->drv_shell.drv_shell_transmit(h_shell->print_buffer, size);
                    drv_uart_waitTransmitComplete();  // Wait for transmission to complete

                    shell_exec(h_shell);  // Execute the command after input is complete
                }
                else                     // If buffer is empty, print an error or ignore

                {
                    size = snprintf(h_shell->print_buffer, BUFFER_SIZE, "\r\nError: No command entered\r\n");
                    h_shell->drv_shell.drv_shell_transmit(h_shell->print_buffer, size);
                    drv_uart_waitTransmitComplete();  // Wait for transmission to complete
                }

                // Reset reading state and buffer
                reading = 0;  // Exit reading state
                pos = 0;      // Reset buffer
                break;

            case '\b':  // Process BACKSPACE key
                if (pos > 0) {
                    pos--;  // Remove last character from buffer
                    // Send backspace and space to the terminal to visually erase the character
                    h_shell->drv_shell.drv_shell_transmit(backspace, sizeof(backspace) - 1);
                    drv_uart_waitTransmitComplete();  // Wait for transmission to complete
                    h_shell->cmd_buffer[pos] = 0;  // Null terminate the command buffer after removing char

                }
                break;

            default:  // Handle other characters
                if (pos < BUFFER_SIZE) {
                    h_shell->drv_shell.drv_shell_transmit(&c, 1);  // Echo the character back to terminal
                    drv_uart_waitTransmitComplete();  // Wait for transmission to complete
                    h_shell->cmd_buffer[pos++] = c;  // Store the character in buffer
                }
                break;
        }
    }

    return 0;  // Function returns immediately without blocking
}
