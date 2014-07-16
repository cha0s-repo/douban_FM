/*
 * doubanfm.c
 *
 *  Created on: 2014Äê7ÔÂ15ÈÕ
 *      Author: Administrator
 */

#include <string.h>
#include "rom_map.h"		//for delay

#define FM_HOST		"www.douban.com"
#define FM_CHANNEL	"www.douban.com/j/app/radio/channels"

#define FM_SONG		"www.douban.com/j/app/radio/people?app_name=radio_desktop_win&version=100&channel=%s&type=n"
#define FM_LOGIN	"www.douban.com/j/app/login"

int fm_get_channel(unsigned char ch_id[])
{

}

int fm_get_song(char *song, char *ch_id)
{
	char buff[256] = {0};

	sprintf(buff, FM_SONG, ch_id);

	request_song(buff, song);
}

int fm_play_song(char *song_url)
{
	play_song(song_url);
}

int fm_player(void)
{
	char song_url[128] = {0};			// It's very tricky douban encoding track name into sequence number, so the url is pretty formated

	Report("Douban FM is ready\r\n");

	while(1)
	{
		//fm_get_channel();
		memset(song_url, 0, sizeof(song_url));
		fm_get_song(song_url, "1");

		if (strlen(song_url))
		{
			Report("Going to play : %s\r\n", song_url);
			fm_play_song(song_url);
		}
		else
		{
			// wait for next try
			Report("Cannot fitch a song to play, waiting for next try\r\n");
			MAP_UtilsDelay(4800000);
		}
	}
}
