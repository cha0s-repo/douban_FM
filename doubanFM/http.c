/*
 * http.c
 *
 *  Created on: 2014Äê6ÔÂ24ÈÕ
 *      Author: Administrator
 */

#include <string.h>
#include "simplelink.h"
#include "protocol.h"
#include "rom_map.h"

#define UARTprintf	Report
#define UART_PRINT	Report

#define POST_BUFFER     " HTTP/1.1 \r\nAccept: text/html, application/xhtml+xml, */*\r\nconnection:keep-alive\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9_4)\r\n"

#define HTTP_FILE_NOT_FOUND    "404 Not Found" /* HTTP file not found response */
#define HTTP_STATUS_OK         "200 OK"  /* HTTP status ok response */
#define HTTP_CONTENT_LENGTH    "Content-Length:"  /* HTTP content length header */
#define HTTP_CONTENT_TYPE		"Content-type:"
#define HTTP_TRANSFER_ENCODING "Transfer-Encoding:" /* HTTP transfer encoding header */
#define HTTP_ENCODING_CHUNKED  "chunked" /* HTTP transfer encoding header value */
#define HTTP_CONNECTION        "Connection:" /* HTTP Connection header */
#define HTTP_CONNECTION_CLOSE  "close"  /* HTTP Connection header value */

#define DNS_RETRY       5 /* No of DNS tries */

#define HTTP_END_OF_HEADER  "\r\n\r\n"  /* string marking the end of headers in response */

#define SIZE_45K 46080  /* Serial flash file size 45 KB */

#define MAX_BUFF_SIZE  1460



enum
{
    CONNECTED = 0x1,
    IP_ACQUIRED = 0x2
}e_Stauts;


void CLI_Write(const char *msg)
{
	UARTprintf(msg);
}

int CreateConnection(unsigned long DestinationIP)
{
    SlSockAddrIn_t  Addr;
    int             Status = 0;
    int             AddrSize = 0;
    int             SockID = 0;
    SlSockNonblocking_t enableOption;

    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(80);
    Addr.sin_addr.s_addr = sl_Htonl(DestinationIP);

    AddrSize = sizeof(SlSockAddrIn_t);

    SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
    if( SockID < 0 )
    {
        /* Error */
        CLI_Write("Error while opening the socket\r\n");
        return -1;
    }

    //enableOption.NonblockingEnabled = 0;
    //sl_SetSockOpt(SockID,SOL_SOCKET,SL_SO_NONBLOCKING, &enableOption,sizeof(enableOption)); // Enable/disable nonblocking mode
    Status = sl_Connect(SockID, ( SlSockAddr_t *)&Addr, AddrSize);
    if( Status < 0 )
    {
        /* Error */
        CLI_Write("Error during connection with server\r\n");
        sl_Close(SockID);
        return -1;
    }
    return SockID;
}

unsigned long hexToi(unsigned char *ptr)
{
    unsigned long result = 0;
    int idx = 0;
    unsigned int len = strlen((const char *)ptr);

    /* convert charecters to upper case */
    for(idx = 0; ptr[idx] != '\0'; ++idx)
        if(ptr[idx] > 96 && ptr[idx] < 103)
            ptr[idx] -= 32;

    for(idx = 0; ptr[idx] != '\0'; ++idx)
    {
        if(ptr[idx] <= 57)
            result += (ptr[idx]-48)*(1<<(4*(len-1-idx)));
        else
            result += (ptr[idx]-55)*(1<<(4*(len-1-idx)));
    }

    return result;
}
//****************************************************************************
//
//! \brief Calculate the file chunk size
//!
//! \param[in]      len - pointer to length of the data in the buffer
//! \param[in]      p_Buff - pointer to ponter of buffer containing data
//! \param[out]     chunk_size - pointer to variable containing chunk size
//!
//! \return         0 for success, -ve for error
//
//****************************************************************************
int GetChunkSize(int *len, char **p_Buff, unsigned long *chunk_size)
{
#if 0
	int           idx = 0;
	unsigned char lenBuff[10];

	idx = 0;
	memset(lenBuff, 0, 10);
	while(*len >= 0 && **p_Buff != 13) /* check for <CR> */
	{
		if(*len == 0)
		{
			memset(g_buff, 0, sizeof(g_buff));
			*len = sl_Recv(g_iSockID, &g_buff[0], MAX_BUFF_SIZE, 0);
			if(*len <= 0)
				return -1;

			*p_Buff = g_buff;
		}
		lenBuff[idx] = **p_Buff;
		idx++;
		(*p_Buff)++;
		(*len)--;
	}
	(*p_Buff) += 2; // skip <CR><LF>
	(*len) -= 2;
	*chunk_size = hexToi(lenBuff);
	UART_PRINT("chunk size is %d\r\n", *chunk_size);
	return 0;
#endif
}

//****************************************************************************
//
//! \brief Obtain the file from the server
//!
//! This function requests the file from the server and save it in
//! temporary file (TEMP_FILE_NAME) in serial flash.
//! To request a different file for different user needs to modify the
//! PREFIX_BUFFER and POST_BUFFER macros.
//!
//! \param[in]      None
//!
//! \return         0 for success and negative for error
//
//****************************************************************************
int play_song(char *req)
{
	int           transfer_len = 0;
	long          retVal = 0;

	char *pBuff = 0;
	char *end = 0;
	char          eof_detected = 0;
	unsigned long recv_size = 0;
	unsigned char isChunked = 0;

	unsigned int bytesReceived;

	char g_buff[MAX_BUFF_SIZE] = {0};
	char host[64] = {0};

	int g_iSockID;
	unsigned long ip;

	int i;

	pBuff = strstr(req, "http://");
	if (pBuff)
	{
		pBuff = req + 7;
	}
	else
	{
		pBuff = req;
	}

	end = strstr(pBuff, "/");

	if (!end)
	{
		Report("Cannot get host name \r\n");
		return -1;
	}

	for(i = 0; i + pBuff < end; i++ )
	{
		host[i] = *(pBuff + i);
	}

	pBuff = end;

	retVal = sl_NetAppDnsGetHostByName(host, strlen(host), &ip, SL_AF_INET);
    if(retVal < 0)
    {
    	Report("Cannot get host %s: %d\r\n", host, ip);
    	return -1;
    }
    g_iSockID = CreateConnection(ip);

	if(g_iSockID < 0)
		UART_PRINT("open socket failed\r\n");

	memset(g_buff, 0, sizeof(g_buff));

	strcat(g_buff, "GET ");
	strcat(g_buff, pBuff);
	strcat(g_buff, POST_BUFFER);
	strcat(g_buff, "Host: ");
	strcat(g_buff, host);
	strcat(g_buff, "\r\n");
	strcat(g_buff, HTTP_END_OF_HEADER);

	// Send the HTTP GET string to the opened TCP/IP socket.
    transfer_len = sl_Send(g_iSockID, g_buff, strlen((const char *)g_buff), 0);

	if (transfer_len < 0)
	{
		UART_PRINT("Socket Send Error\r\n");
		return -1;
	}

	memset(g_buff, 0, sizeof(g_buff));
	audio_play_start();

	// get the reply from the server in buffer.
	transfer_len = sl_Recv(g_iSockID, &g_buff[0], MAX_BUFF_SIZE, 0);

	if(transfer_len > 0)
	{
		// Check for 404 return code
		if(strstr((const char *)g_buff, HTTP_FILE_NOT_FOUND) != 0)
		{
			UART_PRINT("[HTTP] 404\r\n");

			return -1;
		}

		// if not "200 OK" return error
		if(strstr((const char *)g_buff, HTTP_STATUS_OK) == 0)
		{
			UART_PRINT("[HTTP] no 200\r\n");

			return -1;
		}

		// check if content length is transfered with headers
		pBuff = (unsigned char *)strstr((const char *)g_buff, HTTP_CONTENT_LENGTH);
		if(pBuff != 0)
		{
			// not supported
			//UART_PRINT("Server response format is not supported\r\n");//youmaywant read g_buff
			//UART_PRINT(g_buff);
			//return -1;
		}

		// Check if data is chunked
		pBuff = (unsigned char *)strstr((const char *)g_buff, HTTP_TRANSFER_ENCODING);
		if(pBuff != 0)
		{
			pBuff += strlen(HTTP_TRANSFER_ENCODING);
			while(*pBuff == 32)
				pBuff++;

			if(memcmp(pBuff, HTTP_ENCODING_CHUNKED, strlen(HTTP_ENCODING_CHUNKED)) == 0)
			{
				recv_size = 0;
				isChunked = 1;
			}
		}
		else
		{
			// Check if connection will be closed by after sending data
			// In this method the content length is not received and end of
			// connection marks the end of data
			pBuff = (unsigned char *)strstr((const char *)g_buff, HTTP_CONNECTION);
			if(pBuff != 0)
			{
				pBuff += strlen(HTTP_CONNECTION);
				while(*pBuff == 32)
					pBuff++;

				if(memcmp(pBuff, HTTP_ENCODING_CHUNKED, strlen(HTTP_CONNECTION_CLOSE)) == 0)
				{
					// not supported
					UART_PRINT("Server response format is not supported\r\n");
					return -1;
				}
			}
		}

		// "\r\n\r\n" marks the end of headers

		pBuff = (unsigned char *)strstr((const char *)g_buff, HTTP_END_OF_HEADER);
		while(pBuff == 0)
		{
			memset(g_buff, 0, sizeof(g_buff));
			transfer_len = sl_Recv(g_iSockID, &g_buff[0], MAX_BUFF_SIZE, 0);
			pBuff = (unsigned char *)strstr((const char *)g_buff, HTTP_END_OF_HEADER);			//XXX this may cause /r/n in two buff
		}

		// Increment by 4 to skip "\r\n\r\n"
		pBuff += 4;

		// Adjust buffer data length for header size
		transfer_len -= (pBuff - g_buff);
	}

	// If data in chunked format, calculate the chunk size
	if(isChunked == 1)
	{
		if(GetChunkSize(&transfer_len, &pBuff, &recv_size) < 0)
		{
			UART_PRINT("Problen with connection to server\r\n");
			return -1;
		}
	}

	while (0 < transfer_len)
	{
		// For chunked data recv_size contains the chunk size to be received
		if(recv_size <= transfer_len && isChunked)
		{
			audio_player(pBuff, recv_size);
			if(retVal < recv_size)
			{
				UART_PRINT("Error during writing the file\r\n");
				return -1;
			}
			transfer_len -= recv_size;
			bytesReceived +=recv_size;
			pBuff += recv_size;
			recv_size = 0;

			if(isChunked == 1)
			{
				// if data in chunked format calculate next chunk size
				pBuff += 2; // 2 bytes for <CR> <LF>
				transfer_len -= 2;

				if(GetChunkSize(&transfer_len, &pBuff, &recv_size) < 0)
				{
					// Error
					break;
				}

				// if next chunk size is zero we have received the complete file
				if(recv_size == 0)
				{
					eof_detected = 1;
					break;
				}

				if(recv_size < transfer_len)
				{
					// Code will enter this section if the new chunk size is
					// less than the transfer size. This will the last chunk of
					// file received
					audio_player(pBuff, recv_size);
					if(retVal < recv_size)
					{
						UART_PRINT("Error during writing the file\r\n");
						return -1;
					}
					transfer_len -= recv_size;
					bytesReceived +=recv_size;
					pBuff += recv_size;
					recv_size = 0;

					pBuff += 2; // 2bytes for <CR> <LF>
					transfer_len -= 2;

					// Calculate the next chunk size, should be zero
					if(GetChunkSize(&transfer_len, &pBuff, &recv_size) < 0)
					{
						// Error
						break;
					}

					// if next chunk size is non zero error
					if(recv_size != 0)
					{
						// Error
						break;
					}
					eof_detected = 1;
					break;
				}
				else
				{
					audio_player(pBuff, transfer_len);
					if(retVal < transfer_len)
					{
						UART_PRINT("Error during writing the file\r\n");
						return -1;
					}
					recv_size -= transfer_len;
					bytesReceived +=transfer_len;
				}
			}
			// complete file received exit
			if(recv_size == 0)
			{
				eof_detected = 1;
				break;
			}
		}
		else
		{
			audio_player(pBuff, transfer_len);
			if (retVal < 0)
			{
				UART_PRINT("Error during writing the file\r\n");
				return -1;
			}
			bytesReceived +=transfer_len;
			//recv_size -= transfer_len;
		}

		memset(g_buff, 0, sizeof(g_buff));
		transfer_len = sl_Recv(g_iSockID, &g_buff[0], MAX_BUFF_SIZE, 0);
		pBuff = g_buff;
	}

	audio_play_end();
	if(0 > transfer_len || eof_detected == 0)
	{
		UART_PRINT("Error While File Download : %d\r\n", bytesReceived);
		sl_Close(g_iSockID);
		return -1;
	}
	else
	{
		UART_PRINT("\nDownloading File Completed with %d\r\n", bytesReceived);
	}

	sl_Close(g_iSockID);
	return 0;
}

int get_mp3(char *buff, char *song)
{
    char *ptr;
    char *end;
    int i,j,k;
    char b[256] = {0};

    ptr = strstr(buff, "url\":");

    end = strstr(buff, "mp3");

    if(ptr == 0 || end == 0)
        return -1;
    j = end - ptr;
    ptr += 6;
    k = 0;
    for(i = 0; i < j - 3; i++)
    {
        if (*(ptr + i) == '\\')
        {
            k++;

        }
        else
            b[i-k] = *(ptr + i);
    }
    memcpy(song, b, strlen(b));
    return 0;
}

int request_song(char *req, char *song)
{
	int           transfer_len = 0;
	long          retVal = 0;

	char *pBuff = 0;
	char *end = 0;
	char g_buff[MAX_BUFF_SIZE] = {0};
	char host[64] = {0};

	int g_iSockID;
	unsigned long ip;
	int recv_size = 0;
	int isChunked = 0;
	int i;

	pBuff = strstr(req, "http://");
	if (pBuff)
	{
		pBuff = req + 7;
	}
	else
	{
		pBuff = req;
	}

	end = strstr(pBuff, "/");

	if (!end)
	{
		Report("Cannot get host name \r\n");
		return -1;
	}

	for(i = 0; i + pBuff < end; i++ )
	{
		host[i] = *(pBuff + i);
	}

	pBuff = end;

	retVal = sl_NetAppDnsGetHostByName(host, strlen(host), &ip, SL_AF_INET);
    if(retVal < 0)
    {
    	Report("Cannot get host %s: %d\r\n", host, ip);
    	return -1;
    }

	g_iSockID = CreateConnection(ip);
	if(g_iSockID < 0)
		UART_PRINT("open socket failed\r\n");

	memset(g_buff, 0, sizeof(g_buff));

	strcat(g_buff, "GET ");
	strcat(g_buff, pBuff);
	strcat(g_buff, POST_BUFFER);
	strcat(g_buff, "Host: ");
	strcat(g_buff, host);
	strcat(g_buff, "\r\n");
	strcat(g_buff, HTTP_END_OF_HEADER);

	// Send the HTTP GET string to the opened TCP/IP socket.
    transfer_len = sl_Send(g_iSockID, g_buff, strlen((const char *)g_buff), 0);

	if (transfer_len < 0)
	{
		UART_PRINT("Socket Send Error\r\n");
		return -1;
	}

	memset(g_buff, 0, sizeof(g_buff));

	// get the reply from the server in buffer.
	transfer_len = sl_Recv(g_iSockID, &g_buff[0], MAX_BUFF_SIZE, 0);

	if(transfer_len > 0)
	{
		// Check for 404 return code
		if(strstr((const char *)g_buff, HTTP_FILE_NOT_FOUND) != 0)
		{
			UART_PRINT("[HTTP] 404\r\n");

			return -1;
		}

		// if not "200 OK" return error
		if(strstr((const char *)g_buff, HTTP_STATUS_OK) == 0)
		{
			UART_PRINT("[HTTP] no 200\r\n");

			return -1;
		}

		// check if content length is transfered with headers
		pBuff = (unsigned char *)strstr((const char *)g_buff, HTTP_CONTENT_LENGTH);
		if(pBuff != 0)
		{
			// not supported
			//UART_PRINT("Server response format is not supported\r\n");//youmaywant read g_buff
			//UART_PRINT(g_buff);
			//return -1;
		}

		// Check if data is chunked
		pBuff = (unsigned char *)strstr((const char *)g_buff, HTTP_TRANSFER_ENCODING);
		if(pBuff != 0)
		{
			pBuff += strlen(HTTP_TRANSFER_ENCODING);
			while(*pBuff == 32)
				pBuff++;

			if(memcmp(pBuff, HTTP_ENCODING_CHUNKED, strlen(HTTP_ENCODING_CHUNKED)) == 0)
			{
				recv_size = 0;
				isChunked = 1;
			}
		}
		else
		{
			// Check if connection will be closed by after sending data
			// In this method the content length is not received and end of
			// connection marks the end of data
			pBuff = (unsigned char *)strstr((const char *)g_buff, HTTP_CONNECTION);
			if(pBuff != 0)
			{
				pBuff += strlen(HTTP_CONNECTION);
				while(*pBuff == 32)
					pBuff++;

				if(memcmp(pBuff, HTTP_ENCODING_CHUNKED, strlen(HTTP_CONNECTION_CLOSE)) == 0)
				{
					// not supported
					UART_PRINT("Server response format is not supported\r\n");
					return -1;
				}
			}
		}

		// "\r\n\r\n" marks the end of headers

		pBuff = (unsigned char *)strstr((const char *)g_buff, HTTP_END_OF_HEADER);
		while(pBuff == 0)
		{
			memset(g_buff, 0, sizeof(g_buff));
			transfer_len = sl_Recv(g_iSockID, &g_buff[0], MAX_BUFF_SIZE, 0);
			pBuff = (unsigned char *)strstr((const char *)g_buff, HTTP_END_OF_HEADER);			//XXX this may cause /r/n in two buff
		}

		// Increment by 4 to skip "\r\n\r\n"
		pBuff += 4;

		// Adjust buffer data length for header size
		transfer_len -= (pBuff - g_buff);
	}

	while (0 < transfer_len)
	{
		pBuff = strstr((const char *)g_buff, "url");
		if (pBuff == 0)
			transfer_len = sl_Recv(g_iSockID, &g_buff[0], MAX_BUFF_SIZE, 0);
		else
		{
			//Report("request song : %s\r\n", pBuff);

			get_mp3(g_buff, song);

			if(strlen(song))
				break;

			memset(g_buff, 0, sizeof(g_buff));
		}
	}

	sl_Close(g_iSockID);
	return 0;
}
