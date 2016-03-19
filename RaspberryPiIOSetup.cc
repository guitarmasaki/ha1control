/*
 HA-1 RaspberryPi IO Setup
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "RaspberryPiIOSetup.h"

void RaspberryPiIOSetup::Init() {

  FILE *fp = fopen("/proc/cpuinfo", "r");
  if(fp == NULL) {
    fprintf(stderr, "can't open /proc/cpuinfo\n");
    return;
  }
  char buf[256];
  char hardware[256];
  hardware[0] = 0;
  longword revision = 0;
  while(!feof(fp)) {
    fgets(buf, 255, fp);
    char *p = strtok(buf, " :\t");
    char *q = strtok(NULL, " :\t");
    if(!strcmp(p, "Hardware")) strcpy(hardware, q);
    if(!strcmp(p, "Revision")) revision = strtoul(q, NULL, 16);
  }
  fclose(fp);
  if(!revision || !hardware[0]) {
    fprintf(stderr, "not found infomation /proc/cpuinfo\n");
    _exit(-1);
  }
  if(!strcmp(hardware, "BCM2709") && (revision < 0x100)) revision |= 0x0100;

  longword gpio_base = BCM2835_PERI_BASE + GPIO_OFFSET;
  if(revision >= 0x100) {
    gpio_base = BCM2836_PERI_BASE + GPIO_OFFSET;
  }

  int memFd = open("/dev/mem", O_RDWR|O_SYNC);
  if(memFd < 0) {
    fprintf(stderr, "can't open /dev/mem\n");
    _exit(-1);
  }

  volatile longword *gpio = (volatile longword *)mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, memFd, gpio_base);
  if ((long)gpio == -1) {
    fprintf(stderr, "mmap error %ld\n", (long)gpio);
    close(memFd);
    _exit(-1);
  }
  
  // pull-none set
  gpio[GPPUD] = 0;
  usleep(10);
  gpio[GPPUDCLK0] = (1 << 16);
  usleep(10);
  gpio[GPPUDCLK0] = 0;
  
  // UART0 CTS enable
  int num = 16; // GPIO16
  int alt = FSELALT3; // CTS
  gpio[GPFSEL + num / 10] = (gpio[GPFSEL + num / 10] & ~(7 << ((num % 10) * 3))) | (alt << ((num % 10) * 3));

  munmap((void *)gpio, BLOCK_SIZE);
  close(memFd);
}
