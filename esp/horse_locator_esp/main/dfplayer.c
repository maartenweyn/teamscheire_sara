#include "dfplayer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"     
#include "esp_log.h"


#define TAG "dfplayer:"

static uint8_t _received[DFPLAYER_RECEIVED_LENGTH];
static uint8_t _sending[DFPLAYER_SEND_LENGTH] = {0x7E, 0xFF, 06, 00, 01, 00, 00, 00, 00, 0xEF};
static bool _isSending = false;
static bool _isAvailable = false;
static unsigned long _timeOutTimer;
static int uart_nr = 0;
static uint8_t _receivedIndex=0;
static uint8_t _handleType;
static uint8_t _handleCommand;
static uint16_t _handleParameter;

static unsigned long _timeOutDuration = 500 * portTICK_PERIOD_MS;

static bool handleMessage(uint8_t type, uint16_t parameter);
static bool handleError(uint8_t type, uint16_t parameter);

static uint16_t calculateCheckSum(uint8_t *buffer) {
  uint16_t sum = 0;
  for (int i=Stack_Version; i<Stack_CheckSum; i++) {
    sum += buffer[i];
  }
  return -sum;
}

static void uint16ToArray(uint16_t value, uint8_t *array) {
  *array = (uint8_t)(value>>8);
  *(array+1) = (uint8_t)(value);
}

static uint16_t arrayToUint16(uint8_t *array) {
  uint16_t value = *array;
  value <<=8;
  value += *(array+1);
  return value;
}

static bool validateStack() {
  return calculateCheckSum(_received) == arrayToUint16(_received+Stack_CheckSum);
}

static bool handleMessage(uint8_t type, uint16_t parameter) {
  _receivedIndex = 0;
  _handleType = type;
  _handleParameter = parameter;
  _isAvailable = true;
  return _isAvailable;
}

static bool handleError(uint8_t type, uint16_t parameter) {
  handleMessage(type, parameter);
  _isSending = false;
  return false;
}

static void  parseStack(){
  uint8_t handleCommand = *(_received + Stack_Command);
  if (handleCommand == 0x41) { //handle the 0x41 ack feedback as a spcecial case, in case the pollusion of _handleCommand, _handleParameter, and _handleType.
    _isSending = false;
    return;
  }
  
  _handleCommand = handleCommand;
  _handleParameter =  arrayToUint16(_received + Stack_Parameter);

  switch (_handleCommand) {
    case 0x3D:
      handleMessage(DFPlayerPlayFinished, _handleParameter);
      break;
    case 0x3F:
      if (_handleParameter & 0x01) {
        handleMessage(DFPlayerUSBOnline, _handleParameter);
      }
      else if (_handleParameter & 0x02) {
        handleMessage(DFPlayerCardOnline, _handleParameter);
      }
      else if (_handleParameter & 0x03) {
        handleMessage(DFPlayerCardUSBOnline, _handleParameter);
      }
      break;
    case 0x3A:
      if (_handleParameter & 0x01) {
        handleMessage(DFPlayerUSBInserted, _handleParameter);
      }
      else if (_handleParameter & 0x02) {
        handleMessage(DFPlayerCardInserted, _handleParameter);
      }
      break;
    case 0x3B:
      if (_handleParameter & 0x01) {
        handleMessage(DFPlayerUSBRemoved, _handleParameter);
      }
      else if (_handleParameter & 0x02) {
        handleMessage(DFPlayerCardRemoved, _handleParameter);
      }
      break;
    case 0x40:
      handleMessage(DFPlayerError, _handleParameter);
      break;
    case 0x3C:
    case 0x3E:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x4E:
    case 0x4F:
      handleMessage(DFPlayerFeedBack, _handleParameter);
      break;
    default:
      handleError(WrongStack, 0);
      break;
  }
}

static bool available(){
	size_t availble_bytes  = 0;
	uart_get_buffered_data_len(uart_nr, &availble_bytes);
  while (availble_bytes > 0) {
    vTaskDelay(1 / portTICK_PERIOD_MS);
    if (_receivedIndex == 0) {
			uart_read_bytes(uart_nr, &_received[Stack_Header], 1, 100);
			ESP_LOGD(TAG, "received: %X", _received[_receivedIndex]);
      if (_received[Stack_Header] == 0x7E) {
        _receivedIndex ++;
      }
    }
    else{

			uart_read_bytes(uart_nr, &_received[_receivedIndex], 1, 100);
			ESP_LOGD(TAG, "received %X", _received[_receivedIndex]);

      switch (_receivedIndex) {
        case Stack_Version:
          if (_received[_receivedIndex] != 0xFF) {
            return handleError(WrongStack, 0);
          }
          break;
        case Stack_Length:
          if (_received[_receivedIndex] != 0x06) {
            return handleError(WrongStack, 0);
          }
          break;
        case Stack_End:
          if (_received[_receivedIndex] != 0xEF) {
            return handleError(WrongStack, 0);
          }
          else{
            if (validateStack()) {
              _receivedIndex = 0;
              parseStack();
              return _isAvailable;
            }
            else{
              return handleError(WrongStack, 0);
            }
          }
          break;
        default:
          break;
      }
      _receivedIndex++;
    }
		uart_get_buffered_data_len(uart_nr, &availble_bytes);
  }
  
  if (_isSending && (xTaskGetTickCount()-_timeOutTimer>=_timeOutDuration)) {
    return handleError(TimeOut, 0);
  }
  
  return _isAvailable;
}

static void sendStack() {
  if (_sending[Stack_ACK]) {  //if the ack mode is on wait until the last transmition
    while (_isSending) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
      available();
    }
  }

  ESP_LOGD(TAG, "sending 0: 0X%x%x%x%x%x%x%x%x%x%x", _sending[0], _sending[1], _sending[2], _sending[3], _sending[4], _sending[5], _sending[6], _sending[7], _sending[8], _sending[9]);
	

	uart_write_bytes(uart_nr, _sending, DFPLAYER_SEND_LENGTH);
  _timeOutTimer = xTaskGetTickCount();
  _isSending = _sending[Stack_ACK];
  
  if (!_sending[Stack_ACK]) { //if the ack mode is off wait 10 ms after one transmition.
  	vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

static uint8_t readType() {
  _isAvailable = false;
  return _handleType;
}

static uint16_t read() {
  _isAvailable = false;
  return _handleParameter;
}

static void sendToStack(uint8_t command, uint16_t argument) {
  _sending[Stack_Command] = command;
  uint16ToArray(argument, _sending+Stack_Parameter);
  uint16ToArray(calculateCheckSum(_sending), _sending+Stack_CheckSum);
  sendStack();
}

static bool waitAvailable(unsigned long duration){
  unsigned long timer = xTaskGetTickCount();
  if (!duration) {
    duration = _timeOutDuration;
  }
  while (!available()){
    if (xTaskGetTickCount() - timer > duration) {
      return false;
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  return true;
}


uint8_t dfplayer_init(const int uart_num, bool isACK, bool doReset) {
	uart_nr = uart_num;
  uart_config_t uart_config = {
      .baud_rate = 9600,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .rx_flow_ctrl_thresh = 122,
  };
  // Configure UART parameters
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  // Setup UART buffered IO with event queue
	QueueHandle_t uart_queue;
	// Install UART driver using an event queue here
	ESP_ERROR_CHECK(uart_driver_install(uart_num, DFPLAYER_RECEIVED_LENGTH, DFPLAYER_SEND_LENGTH, DFPLAYER_SEND_LENGTH, &uart_queue, 0));

	if (isACK) {
    dfplayer_enableACK();
  }
  else{
    dfplayer_disableACK();
  }
  
  if (doReset) {
    dfplayer_reset();
    waitAvailable(2000);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
  else {
    // assume same state as with reset(): online
    _handleType = DFPlayerCardOnline;
  }

  return (readType() == DFPlayerCardOnline) || (readType() == DFPlayerUSBOnline) || !isACK;
}

void dfplayer_enableACK(){ 
  _sending[Stack_ACK] = 0x01;
}

void dfplayer_disableACK() {
  _sending[Stack_ACK] = 0x00;
}


void dfplayer_next(){
  sendToStack(0x01, 0);
}

void dfplayer_previous(){
  sendToStack(0x02, 0);
}

void dfplayer_play(int fileNumber){
  sendToStack(0x03, fileNumber);
}

void dfplayer_volumeUp(){
  sendToStack(0x04, 0);
}

void dfplayer_volumeDown(){
  sendToStack(0x05, 0);
}

void dfplayer_volume(uint8_t volume){
  sendToStack(0x06, volume);
}

void dfplayer_EQ(uint8_t eq) {
  sendToStack(0x07, eq);
}

void dfplayer_loop(int fileNumber) {
  sendToStack(0x08, fileNumber);
}

void dfplayer_outputDevice(uint8_t device) {
  sendToStack(0x09, device);
  vTaskDelay(200 / portTICK_PERIOD_MS);
}

void dfplayer_sleep(){
  sendToStack(0x0A, 0);
}

void dfplayer_reset(){
  sendToStack(0x0C, 0);
}

void dfplayer_start(){
  sendToStack(0x0D, 0);
}

void dfplayer_pause(){
  sendToStack(0x0E, 0);
}

void dfplayer_playFolder(uint8_t folderNumber, uint8_t fileNumber){
	uint16_t buffer = folderNumber;
  buffer <<= 8;
  sendToStack(0x0F, buffer || fileNumber);
}

void dfplayer_outputSetting(bool enable, uint8_t gain){
	uint16_t buffer = enable;
  buffer <<= 8;
  sendToStack(0x10, buffer || gain);
}

void dfplayer_enableLoopAll(){
  sendToStack(0x11, 0x01);
}

void dfplayer_disableLoopAll(){
  sendToStack(0x11, 0x00);
}

void dfplayer_playMp3Folder(int fileNumber){
  sendToStack(0x12, fileNumber);
}

void dfplayer_advertise(int fileNumber){
  sendToStack(0x13, fileNumber);
}

void dfplayer_playLargeFolder(uint8_t folderNumber, uint16_t fileNumber){
  sendToStack(0x14, (((uint16_t)folderNumber) << 12) | fileNumber);
}

void dfplayer_stopAdvertise(){
  sendToStack(0x15, 0);
}

void dfplayer_stop(){
  sendToStack(0x16, 0);
}

void dfplayer_loopFolder(int folderNumber){
  sendToStack(0x17, folderNumber);
}

void dfplayer_randomAll(){
  sendToStack(0x18, 0);
}

void dfplayer_enableLoop(){
  sendToStack(0x19, 0x00);
}

void dfplayer_disableLoop(){
  sendToStack(0x19, 0x01);
}

void dfplayer_enableDAC(){
  sendToStack(0x1A, 0x00);
}

void dfplayer_disableDAC(){
  sendToStack(0x1A, 0x01);
}

int dfplayer_readState(){
  sendToStack(0x42, 0);
  if (waitAvailable(0)) {
    if (readType() == DFPlayerFeedBack) {
      return read();
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}

int dfplayer_readVolume(){
  sendToStack(0x43, 0);
  if (waitAvailable(0)) {
    return read();
  }
  else{
    return -1;
  }
}

int dfplayer_readEQ(){
  sendToStack(0x44, 0);
  if (waitAvailable(0)) {
    if (readType() == DFPlayerFeedBack) {
      return read();
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}

int dfplayer_readFileCounts(uint8_t device){
  switch (device) {
    case DFPLAYER_DEVICE_U_DISK:
      sendToStack(0x47, 0);
      break;
    case DFPLAYER_DEVICE_SD:
      sendToStack(0x48, 0);
      break;
    case DFPLAYER_DEVICE_FLASH:
      sendToStack(0x49, 0);
      break;
    default:
      break;
  }
  
  if (waitAvailable(0)) {
    if (readType() == DFPlayerFeedBack) {
      return read();
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}

int dfplayer_readCurrentFileNumber(uint8_t device){
  switch (device) {
    case DFPLAYER_DEVICE_U_DISK:
      sendToStack(0x4B, 0);
      break;
    case DFPLAYER_DEVICE_FLASH:
      sendToStack(0x4D, 0);
      break;
    case DFPLAYER_DEVICE_SD:
    default:
      sendToStack(0x4C, 0);
      break;
  }
  if (waitAvailable(0)) {
    if (readType() == DFPlayerFeedBack) {
      return read();
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}

int dfplayer_readFileCountsInFolder(int folderNumber){
  sendToStack(0x4E, folderNumber);
  if (waitAvailable(0)) {
    if (readType() == DFPlayerFeedBack) {
      return read();
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}

int dfplayer_readFolderCounts(){
  sendToStack(0x4F, 0);
  if (waitAvailable(0)) {
    if (readType() == DFPlayerFeedBack) {
      return read();
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }
}
