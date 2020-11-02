#include <stdint.h>
#include <stdbool.h>

#define Stack_Header 0
#define Stack_Version 1
#define Stack_Length 2
#define Stack_Command 3
#define Stack_ACK 4
#define Stack_Parameter 5
#define Stack_CheckSum 7
#define Stack_End 9

#define DFPLAYER_RECEIVED_LENGTH 10
#define DFPLAYER_SEND_LENGTH 10

#define TimeOut 0
#define WrongStack 1
#define DFPlayerCardInserted 2
#define DFPlayerCardRemoved 3
#define DFPlayerCardOnline 4
#define DFPlayerPlayFinished 5
#define DFPlayerError 6
#define DFPlayerUSBInserted 7
#define DFPlayerUSBRemoved 8
#define DFPlayerUSBOnline 9
#define DFPlayerCardUSBOnline 10
#define DFPlayerFeedBack 11

#define DFPLAYER_EQ_NORMAL 0
#define DFPLAYER_EQ_POP 1
#define DFPLAYER_EQ_ROCK 2
#define DFPLAYER_EQ_JAZZ 3
#define DFPLAYER_EQ_CLASSIC 4
#define DFPLAYER_EQ_BASS 5

#define DFPLAYER_DEVICE_U_DISK 1
#define DFPLAYER_DEVICE_SD 2
#define DFPLAYER_DEVICE_AUX 3
#define DFPLAYER_DEVICE_SLEEP 4
#define DFPLAYER_DEVICE_FLASH 5


uint8_t dfplayer_init(const int uart_num, bool isACK, bool doReset);
void dfplayer_enableACK();
void dfplayer_disableACK();
void dfplayer_next();
void dfplayer_previous();
void dfplayer_play(int fileNumber);
void dfplayer_volumeUp();
void dfplayer_volumeDown();
void dfplayer_volume(uint8_t volume);
void dfplayer_EQ(uint8_t eq) ;
void dfplayer_loop(int fileNumber);
void dfplayer_outputDevice(uint8_t device);
void dfplayer_sleep();
void dfplayer_reset();
void dfplayer_start();
void dfplayer_pause();
void dfplayer_playFolder(uint8_t folderNumber, uint8_t fileNumber);
void dfplayer_outputSetting(bool enable, uint8_t gain);
void dfplayer_enableLoopAll();
void dfplayer_disableLoopAll();
void dfplayer_playMp3Folder(int fileNumber);
void dfplayer_advertise(int fileNumber);
void dfplayer_playLargeFolder(uint8_t folderNumber, uint16_t fileNumber);
void dfplayer_stopAdvertise();
void dfplayer_stop();
void dfplayer_loopFolder(int folderNumber);
int dfplayer_readFileCounts(uint8_t device);
int dfplayer_readCurrentFileNumber(uint8_t device);
int dfplayer_readFileCountsInFolder(int folderNumber);
int dfplayer_readFolderCounts();

