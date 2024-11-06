/*
 * comRaspberry.c
 *
 *  Created on: Oct 23, 2024
 *      Author: charl
 */

#include "comRaspberry.h"
#include <stdint.h>
#include <string.h>
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart1;


char rx_buffer[20];  // Buffer de réception pour stocker les commandes du RPi
float temperature = 12.5; // Exemple de température initiale
uint32_t pression = 102300; // Exemple de pression initiale
float coeff_k = 12.34; // Coefficient K initial
float angle = 125.7; // Exemple d'angle initial


void handle_command(char *command)
{
	char response[50];

    if (strncmp(command, "GET_T", 5) == 0) {
        printf(response, "T=%+06.2f_C", temperature); // Envoie de la température compensée
    }

    if (strncmp(command, "GET_P", 5) == 0) {
        printf(response, "P=%+06dPa", pression); // Envoie de la pression compensée
    }

    if (strncmp(command, "SET_K=", 6) == 0) {
        scanf(command + 6, "%f", &coeff_k); // Extraction du coefficient K
        printf(response, "SET_K=OK");
    }

    else if (strncmp(command, "GET_K", 5) == 0) {
        printf(response, "K=%08.5f", coeff_k); // Envoie du coefficient K
    }

    else if (strncmp(command, "GET_A", 5) == 0) {
        printf(response, "A=%07.4f", angle); // Envoie de l'angle
    }

    else {
        printf(response, "ERROR"); // Commande non reconnue
    }

    // envoie de la réponse sur le port série vers la Raspberry
    HAL_UART_Transmit(&huart1, (uint8_t*)response, strlen(response), HAL_MAX_DELAY);

}

void receive_command(void) {
    // Réception de la commande
    if (HAL_UART_Receive(&huart1, (uint8_t*)rx_buffer, sizeof(rx_buffer), HAL_MAX_DELAY) == HAL_OK) {
        handle_command(rx_buffer);
    }
}
