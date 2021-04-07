// routines for writing data to an SD-card, if present

// Local logging tag
static const char TAG[] = __FILE__;

#include "sdcard.h"
#include "hash.h"

#ifdef HAS_SDCARD

static bool useSDCard;
RTC_DATA_ATTR int folderInt;

static void createFile(bool verbose);

File fileSDCard;

bool sdcard_init(bool verbose) {
  ESP_LOGI(TAG, "looking for SD-card...");

  // for usage of SD drivers on ESP32 platform see
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sdspi_host.html
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sdmmc_host.html

#if HAS_SDCARD == 1 // use SD SPI host driver
  useSDCard = SD.begin(SDCARD_CS, SDCARD_MOSI, SDCARD_MISO, SDCARD_SCLK);
#elif HAS_SDCARD == 2 // use SD MMC host driver
  // enable internal pullups of sd-data lines
  gpio_set_pull_mode(gpio_num_t(SDCARD_DATA0), GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(gpio_num_t(SDCARD_DATA1), GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(gpio_num_t(SDCARD_DATA2), GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(gpio_num_t(SDCARD_DATA3), GPIO_PULLUP_ONLY);
  useSDCard = SD_MMC.begin();
#endif

  if (useSDCard) {
    ESP_LOGI(TAG, "SD-card found");
    createFile(verbose);
  } else
    ESP_LOGI(TAG, "SD-card not found");
  return useSDCard;
}

void writeMac(MacBuffer_t MacBuffer) {
  char tempBuffer[40];
  time_t t = now();
  #if (VERBOSE == 1)
    ESP_LOGI(TAG, "%02x:%02x:%02x:%02x:%02x:%02x,%d,%d,%02d.%02d.%4d,%02d:%02d:%02d",
      MacBuffer.mac[0],
      MacBuffer.mac[1],
      MacBuffer.mac[2],
      MacBuffer.mac[3],
      MacBuffer.mac[4],
      MacBuffer.mac[5],
      myhash((const char *)&MacBuffer.mac, 4),
      MacBuffer.rssi,
      day(t),
      month(t),
      year(t),
      hour(t),
      minute(t),
      second(t));
  #endif
  sprintf(
    tempBuffer,
    "%d,%d,%02d.%02d.%4d,%02d:%02d:%02d",
    myhash((const char *)&MacBuffer.mac, 4),
    MacBuffer.rssi,
    day(t),
    month(t),
    year(t),
    hour(t),
    minute(t),
    second(t)
  );

  if(!useSDCard)
    sdcard_init(false);
  fileSDCard.println(tempBuffer);
}

void sdcardWriteData(uint16_t noWifi, uint16_t noBle,
                     __attribute__((unused)) uint16_t noBleCWA) {
  static int counterWrites = 0;
#if (SAVE_MACS_INSTANTLY == 1)
  fileSDCard.println();
  fileSDCard.flush();
  return;
#endif
  char tempBuffer[12 + 1];
  time_t t = now();
#if (HAS_SDS011)
  sdsStatus_t sds;
#endif

  if (!useSDCard)
    return;

  ESP_LOGD(TAG, "writing to SD-card");
  sprintf(tempBuffer, "%02d.%02d.%4d,", day(t), month(t), year(t));
  fileSDCard.print(tempBuffer);
  sprintf(tempBuffer, "%02d:%02d:%02d,", hour(t), minute(t), second(t));
  fileSDCard.print(tempBuffer);
  sprintf(tempBuffer, "%d,%d", noWifi, noBle);
  fileSDCard.print(tempBuffer);
#if (COUNT_ENS)
  sprintf(tempBuffer, ",%d", noBleCWA);
  fileSDCard.print(tempBuffer);
#endif
#if (HAS_SDS011)
  sds011_store(&sds);
  sprintf(tempBuffer, ",%5.1f,%4.1f", sds.pm10, sds.pm25);
  fileSDCard.print(tempBuffer);
#endif
  fileSDCard.println();

  if (++counterWrites > 2) {
    // force writing to SD-card
    ESP_LOGD(TAG, "flushing data to card");
    fileSDCard.flush();
    counterWrites = 0;
  }
}
void createFile(bool verbose) {
  useSDCard = false;
  char bufferFilename[8 + 1 + 3 + 1];

  //find number:
  if(verbose) {
    for(int i = 1; i < 100; i++) {
      sprintf(bufferFilename, SDCARD_FILE_NAME, i);
#if HAS_SDCARD == 1
      if(!SD.exists(bufferFilename)) {
        folderInt = i;
        break;
      }
#elif HAS_SDCARD == 2
      if(!SD_MMC.exists(bufferFilename)) {
        folderInt = i;
        break;
      }
#endif
    }
  }
  else
    sprintf(bufferFilename, SDCARD_FILE_NAME, folderInt);
  
  //create / open file:
  
  bool newFile = false;

#if HAS_SDCARD == 1
  if(!SD.exists(bufferFilename)) {
    fileSDCard = SD.open(bufferFilename, FILE_WRITE);
    newFile = true;
  }
  else {
    fileSDCard = SD.open(bufferFilename, F_READ | F_WRITE | F_CREAT | F_APPEND);
  }

#elif HAS_SDCARD == 2
  if(!SD_MMC.exists(bufferFilename)) {
    fileSDCard = SD_MMC.open(bufferFilename, FILE_WRITE);
    newFile = true;
  }
  else
    fileSDCard = SD_MMC.open(bufferFilename, F_READ | F_WRITE | F_CREAT | F_APPEND);
#endif
  if(fileSDCard) {
    if(newFile) {
#if (SAVE_MACS_INSTANTLY == 1)
      fileSDCard.print(SDCARD_INSTANT_MACS_FILE_HEADER);
#elif (COUNT_ENS)
      fileSDCard.print(SDCARD_FILE_HEADER_CWA); // for Corona-data (CWA)
#elif (HAS_SDS011)
      fileSDCard.print(SDCARD_FILE_HEADER_SDS011);
#endif
      fileSDCard.println();
    }

    useSDCard = true;
  }
}

#endif // (HAS_SDCARD)
