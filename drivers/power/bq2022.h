/*
 *   Copyright (C) IQOO,2022			All Rights Reserved!
 *
 *   Module: bq2022 driver header
 *
 *   File: bq2022.h
*/

#ifndef __BQ2022_H__
#define __BQ2022_H__

/** bq2022 ROM command level 1 */
#define BQ2022_CMD_READ_ROM			0x33					/** bq2022 read rom command 33h */
#define BQ2022_CMD_MATCH_ROM			0x55					/** bq2022 match rom command 55h */
#define BQ2022_CMD_SEARCH_ROM			0xF0					/** bq2022 search rom command f0h */
#define BQ2022_CMD_SKIP_ROM			0xCC					/** bq2022 skip rom command cch */

/** bq2022 Memory command level 2 */
#define BQ2022_MEM_CMD_READ_FIELD_CRC		0xF0					/** bq2022 mem read memory/field crc 0xf0 */
#define BQ2022_MEM_CMD_READ_EPROM_STATUS	0xAA					/** bq2022 mem read eprom status 0xaa */
#define BQ2022_MEM_CMD_READ_PAGE_CRC		0xC3					/** bq2022 mem read memory/page crc 0xc3 */
#define BQ2022_MEM_CMD_WRITE_MEM		0x0F					/** bq2022 mem write memory 0x0f */
#define BQ2022_MEM_CMD_PROGRAMMING_PROFILE	0x99					/** bq2022 mem programming profile 0x99 */
#define BQ2022_MEM_CMD_WRITE_MEM_STATUS		0x55					/** bq2022 mem write eprom status 0x55 */

/** Program control Only in WRITE MEMORY and WRITE STATUS Modes */
#define BQ2022_PROGRAM_CTRL			0x5A					/** bq2022 program control 0x5a */

/********************************************** bq2022 time sequence **************************************************/
/** bq2022 reset */
#define BQ2022_RST_T				150					/** 150us  datasheet: 120us */
#define BQ2022_RST_PRESENCE_T			600					/** 600us  datasheet: 480us RESET and PRESENCE*/
#define BQ2022_T_PPD				20					/** Tppd:15~60us */
#define BQ2022_T_PP				240					/** Tppd:60~240us */
#define BQ2022_T_RSTREC				250					/** Trstrec:480us, 480-60-240=180 */

#define BQ2022_SRTB_T				10					/** 8us write:1~15,wsrtb~15us; read:1~13,wsrtb~13us */
#define BQ2022_WAIT_T				10					/** 8us write:15us; read:13us */

#define BQ2022_WRITE_HOLD			70					/** BQ2022_SRTB_T + BQ2022_WAIT_T + HOLD > 60us  write Twdh:60us~tc */
#define BQ2022_READ_WAIT			5					/** BQ2022_SRTB_T + BQ2022_WAIT_T > 17 Todho:17~60us */

#define BQ2022_REC_TIME				7					/** 1/5(memory) */

/********************************************** bq2022 EPROM memory map ***************************************************/
#define BQ2022_PAGE0				0x0000					/** page0: 0x0000 ~ 0x001f */
#define BQ2022_PAGE1				0x0020					/** page1: 0x0020 ~ 0x003f */
#define BQ2022_PAGE2				0x0040					/** page2: 0x0040 ~ 0x005f */
#define BQ2022_PAGE3				0x0060					/** page3: 0x0060 ~ 0x007f */

/********************************************* bq2022 protocol data length ************************************************/
#define BQ2022_READ_ROM_LEN			8					/** bq2022 read rom data:7 byte 1 byte(crc) */
#define BQ2022_PAGE_LEN				32					/** page:32 bytes */
#define BQ2022_PAGE_NUM				4					/** 4 pages */
#define BQ2022_DATA_LEN				(BQ2022_PAGE_LEN*BQ2022_PAGE_NUM)	/** bq2022 total data len */ 

#endif
