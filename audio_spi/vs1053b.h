/*
 * vs1053b.h
 *
 *  Created on: 2014��7��2��
 *      Author: Administrator
 */

#ifndef VS1053B_H_
#define VS1053B_H_

void audio_init(void);
int audio_reset(void);

int audio_play_start();
int audio_player(unsigned char *data, int len);
int audio_play_end();



#endif /* VS1053B_H_ */
