/*
 * Basic Test Firmware on the ADE900
 *
 * Created on: May 5, 2019
 * Authors: Rubens de Andrade Fernandes, Samuel Torres, Lennon Nascimento
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
//#include <getopt.h>
//#include <linux/types.h>
#include <stdbool.h>

#define GPIO_PATH "/sys/class/gpio/gpio20"//Path to access the Chip Select GPIO
#define SPI_PATH "/dev/spidev1.0"//Path to access the SPI Driver

#define PACK_16 16//Size 16 bits package
#define PACK_32 32//Size 32 bits package
#define MODE_SPI 0//SPI mode
#define SPEED 10000//SPI Speed transfer
#define TRASH 0//Bytes that keep enable the spi clock while ADE is reading the data
#define WRITE 0//Operation code of ADE 9000 to enable the WRITE option
#define READ 1//Operation code of ADE 9000 to enable the READ option
#define RUN 0x4800//Write this register to 1 to start the measurements
#define A_VRMS 0x20D8//Phase A filter - based voltage rms value
#define A_IRMS 0x20C8//Phase A filter - based current rms value
#define A_VRMS_10_12 0x21C8//Phase A voltage fast 10 cycle rms/12 cycle rms calculation
#define A_IRMS_10_12 0x21B8//Phase A current fast 10 cycle rms/12 cycle rms calculation
#define AVA 0x2128//Phase A total apparent power, updated at 8 kSPS
#define B_VRMS 0x22D8//Phase B filter - based voltage rms value
#define B_IRMS 0x22C8//Phase B filter - based current rms value
#define B_VRMS_10_12 0x23C8//Phase B voltage fast 10 cycle rms/12 cycle rms calculation
#define B_IRMS_10_12 0x23B8//Phase B current fast 10 cycle rms/12 cycle rms calculation
#define BVA 0x2328//Phase B total apparent power, updated at 8 kSPS
#define C_VRMS 0x24D8//Phase C filter - based voltage rms value
#define C_IRMS 0x24C8//Phase C filter - based current rms value
#define C_VRMS_10_12 0x25C8//Phase C voltage fast 10 cycle rms/12 cycle rms calculation
#define C_IRMS_10_12 0x25B8//Phase C current fast 10 cycle rms/12 cycle rms calculation
#define CVA 0x2528//Phase C total apparent power, updated at 8 kSPS
#define CALIB_V 0.000017163//Voltage calibration constant
#define CALIB_I 0.000004269//Current calibration constant
#define CALIB_VA 0.00989166//Power calibration constant