/*
 * spi.h
 *
 *  Created on: 2014Äê7ÔÂ8ÈÕ
 *      Author: Administrator
 */

#ifndef SPI_H_
#define SPI_H_

int vs_write_cmd(unsigned char c);
int vs_write_data(unsigned char *d, int len);

void vs_cs(unsigned char cmd);
void vs_dcs(unsigned char cmd);
void vs_rst(unsigned char cmd);

void vs_spi_open(void);

int vs_req(void);

void vs_spi_clk_cmd();
void vs_spi_clk_data();

#endif /* SPI_H_ */
