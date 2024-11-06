# 2425_ESE_BusReseau_Chapart_Fricot

# TP Bus et Réseaux Industriels

## Table des matières

1. [TP 1 - Bus I²C](#tp-1---bus-i2c)
   - [Capteur BMP280](#capteur-bmp280)
   - [Setup du STM32](#setup-du-stm32)
   - [Communication I²C](#communication-i2c)
2. [TP2 - Interfaçage STM32 - Raspberry](#tp2---interfaçage-stm32---raspberry)
   - [Mise en route du Raspberry Pi Zéro](#mise-en-route-du-raspberry-pi-zéro)
   - [Port Série](#port-série)
3. [TP3 - Interface REST](#tp3---interface-rest)
4. [Installation du Serveur Python](#installation-du-serveur-python)
   - [Première Page REST](#première-page-rest)


## TP 1 - Bus I²C
### 1.1 Capteur BMP280

Le BMP280 est un capteur de pression et température développé par Bosch (page produit). 
À partir de la datasheet du BMP280, nous avons identifié les éléments suivants : 

1. Les adresses I²C possibles pour ce composant : 
The 7-bit device address is 111011x.  
The 6 MSB bits are fixed.  
Le dernier bit est modifiable par la valeur SDO et peut être modifié en cours de fonctionnement. La connexion de SDO à GND donne l'adresse d'esclave 1110110 (0x76) ; la connexion à VDDIO donne l'adresse d'esclave 1110111 (0x77), ce qui est la même chose. 

2. Le registre et la valeur permettant d'identifier ce composant : 0xD0 “id” 

3. Le BMP280 offre trois modes d'alimentation : le mode veille, le mode forcé et le mode normal. Ces modes peuvent être sélectionnés à l'aide des bits mode [1:0] du registre de contrôle 0xF4. 
  - 00 --> mode veille
  - 01 and 10 --> mode forcé
  - 11 --> mode normal 

4. Les registres contenant l'étalonnage du composant : calib25...calib00 aux adresses 0xA1…0x88 

5. Les registres contenant la température (ainsi que le format) : 0xFA…0xFC “temp” (_msb, _lsb, _xlsb) 

6. Les registres contenant la pression (ainsi que le format) : 0xF7…0xF9 “press” (_msb, _lsb, _xlsb) 

7. Les fonctions permettant le calcul de la température et de la pression compensées, en format entier 32 bits : 

Les valeurs de pression et de température sont censées être reçues au format 20 bits, positif, stocké dans un entier signé de 32 bits. 

à repondre !!!!

### 1.2. Setup du STM32 

Sous STM32CubeIDE, nous avons mis en place une configuration adaptée à notre carte nucleo-F446RE. Pour ce TP, nous aurons besoin des connections suivantes : 

  - Une liaison I²C. (broches PB8 et PB9)
  - Une UART sur USB (UART2 sur les broches PA2 et PA3)
  - Une liaison UART indépendante pour la communication avec le Raspberry (TP2)
  - D'une liaison CAN (TP4)

<div align="center">
<img src="./Images/setupIOC.png" width="400">
</div>
