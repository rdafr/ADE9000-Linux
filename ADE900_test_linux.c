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

int transfer(uint32_t send[], uint32_t receive[], uint8_t length)//Function for SPI transfer using the spidev1.0 driver struct
{
        int file_spi = open(SPI_PATH, O_RDWR);//Open the SPI Driver with access mode Write/read
        struct spi_ioc_transfer transfer;
        transfer.tx_buf = (unsigned long)send;//Convert the data into unsigned long and send
        transfer.rx_buf = (unsigned long)receive;//Convert the received data into unsigned long
        transfer.len = length/8;//Convert bits into bytes
        transfer.speed_hz = SPEED;//Set the spi speed transfer
        transfer.bits_per_word = length;// Set the bits per word tha will be transfered
        transfer.delay_usecs = 0;//set the delay time in the transfer process

        int status = ioctl(file_spi, SPI_IOC_MESSAGE(1), &transfer);//setting the error conditions
        if (status < 0)
        {
                perror("SPI: SPI_IOC_MESSAGE Failed");//error state
                close(file_spi);
                usleep(1000);
                return -1;
        }
        usleep(100000);
        close(file_spi);
        return status;
}

void config_spi(uint8_t bits_spi)//Setting the SPI configurations
{
        int file_spi = open(SPI_PATH, O_RDWR);//Open the SPI Driver with access mode Write/read
        uint32_t speed_spi = SPEED;
        uint8_t mode_spi = MODE_SPI;//ioctl(Descriptor file, Command,integer or pointer to data)- Device interface
        ioctl(file_spi, SPI_IOC_WR_MODE, &mode_spi);
        ioctl(file_spi, SPI_IOC_RD_MODE, &mode_spi);
        ioctl(file_spi, SPI_IOC_WR_BITS_PER_WORD, &bits_spi);
        ioctl(file_spi, SPI_IOC_RD_BITS_PER_WORD, &bits_spi);
        ioctl(file_spi, SPI_IOC_WR_MAX_SPEED_HZ, &speed_spi);
        ioctl(file_spi, SPI_IOC_RD_MAX_SPEED_HZ, &speed_spi);
        close(file_spi);
}

void csInit(void)//check if the gpio 20 is already activated
{
        bool i;
        FILE  *gpio_file;

        if(gpio_file = fopen(GPIO_PATH,"r"))
        {
                i = 1;
                fclose(gpio_file);
        }
        else
        {
                i = 0;
        }

        if (i == 0)
        {
                system("echo 20 > /sys/class/gpio/export");//Enable GPIO 20
                system("echo out > /sys/class/gpio/gpio20/direction");// Set the GPIO 20 as output
                system("echo 1 > /sys/class/gpio/gpio20/value");//set the GPIO with high logic
        }

        else
        {
                system("echo 1 > /sys/class/gpio/gpio20/value");
        }
}

void csMode(bool mode)//Change logic level of the chip select(GPIO 20)
{
        if(mode == 1)
        {
                system("echo 1 > /sys/class/gpio/gpio20/value");
        }
        else if(mode == 0)
        {
                system("echo 0 > /sys/class/gpio/gpio20/value");
        }
}

uint32_t call_ADE9000(uint8_t length,uint32_t send)//Call ADE and execute the spi transfer(Length of pack, sended pack,received pack)
{
        uint32_t recieve;
        fflush(stdout);
        config_spi(length);
        transfer( (uint32_t*)&send,(uint32_t*)&recieve,length);
        return recieve;
}

uint32_t read_ADE9000(uint32_t address)//Execute the read function of registers(Register address, operation code read)
{
        uint32_t recieve;

        csMode(0);
        call_ADE9000(PACK_16,address);
        recieve =  call_ADE9000(PACK_32,TRASH);
        csMode(1);
        return recieve;
}

void Init_ADE9000(void)//run inicial configurations im ADE9000
{
        uint32_t address = RUN, command = 0x1, rd_trash;
        uint8_t size = PACK_16;
        csMode(0);
        call_ADE9000(size,address);
        call_ADE9000(size,command);
        csMode(1);
        usleep(1000);
}

int main()//Main function used to display the voltage, current and power values of the ADE9000
{
        uint32_t rd_spi, wr_spi;
        int i = 0;

        csInit();
        Init_ADE9000();

        while(1)
        {

                switch (i)
                {
                        case  0:
                        wr_spi = A_VRMS;
                        rd_spi = read_ADE9000(wr_spi) * CALIB_V;
                        printf("FASE A -Tensao: %d ",rd_spi);
                        break;

                        case  1:
                        wr_spi = A_IRMS;
                        rd_spi = read_ADE9000(wr_spi) * CALIB_I;
                        printf("Corrente: %d ",rd_spi);
                        break;

                        case  2:
                        wr_spi = AVA;
                        rd_spi = read_ADE9000(wr_spi) * CALIB_VA;
                        printf("Potencia Aparente: %d \n \n",rd_spi);
                        break;

                        case  3:
                        wr_spi = B_VRMS;
                        rd_spi = read_ADE9000(wr_spi) * CALIB_V;
                        printf("FASE B -Tensao: %d ",rd_spi);
                        break;

                        case  4:
                        wr_spi = B_IRMS;
                        rd_spi = read_ADE9000(wr_spi) * CALIB_I;
                        printf("Corrente: %d ",rd_spi);
                        break;

                        case  5:
                        wr_spi = BVA;
                        rd_spi = read_ADE9000(wr_spi) * CALIB_VA;
                        printf("Potencia Aparente: %d \n \n",rd_spi);
                        break;

                        case  6:
                        wr_spi = C_VRMS;
                        rd_spi = read_ADE9000(wr_spi) * CALIB_V;
                        printf("FASE C - Tensao: %d ",rd_spi);
                        break;

                        case  7:
                        wr_spi = C_IRMS;
                        rd_spi = read_ADE9000(wr_spi) * CALIB_I;
                        printf("Corrente: %d ",rd_spi);
                        break;

                        case  8:
                        wr_spi = CVA;
                        rd_spi = read_ADE9000(wr_spi) * CALIB_VA;
                        printf("Potencia Aparente: %d \n \n \n",rd_spi);
                        sleep(3);
                        break;
                }
        i = i + 1;
        if(i > 8) i = 0;
        }
        return 0;
}


