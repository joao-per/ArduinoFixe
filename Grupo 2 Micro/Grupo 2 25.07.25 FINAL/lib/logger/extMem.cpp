////////////////////////////////////////////////////////////////////////
/// @copyright ATEC
////////////////////////////////////////////////////////////////////////
///
/// @brief External Memory class implementation
///
/// @version 0.1
///
////////////////////////////////////////////////////////////////////////
///
/// @author Grupo 2
/// @date
///
////////////////////////////////////////////////////////////////////////

#include "extMem.hpp"
#include <cstring> // Para strtok, strncpy
#include <cstdio>  // Para snprintf
#include "set_rtc.hpp"

char filename[20];
char date_time[20];
int fileIndex = 0;

SdFat sd;
SdFile file;

ExtMEM::ExtMEM() {}

bool ExtMEM::initExtMem()
{
    // Start Serial communication
    Serial.begin(SERIAL_BAUDRATE);

    // Prepare GPIO
    pinMode(uSD_CS_PIN, OUTPUT);
    digitalWrite(uSD_CS_PIN, LOW);

    // Initializing SD card via SPI
    if (!sd.begin(uSD_CS_PIN, SD_SCK_MHZ(10)))
    {
        Serial.println("[ERROR] uSD initialization failed!");
        delay(100);
        digitalWrite(uSD_CS_PIN, HIGH);
        isSDCardInitialized = false;
        return false;
    }

    isSDCardInitialized = true;
    delay(100);
    digitalWrite(uSD_CS_PIN, HIGH);

    if (!initRTC())
    {
        Serial.println("RTC initialization failed!");
        return false;
    }
    else
    {
        Serial.println("RTC initializated!");
    }

    return true;
}

bool ExtMEM::initFile(const char *type)
{
    fileIndex = 0;

    if (!isSDCardInitialized)
    {
        return false;
    }

    do
    {
        snprintf(filename, sizeof(filename), "%s%s%d.%s", LOG_PATH.c_str(), LOG_FILENAME.c_str(), fileIndex, type);
        fileIndex++;
    } while (sd.exists(filename));

    return true;
}

void ExtMEM::info(const char *message)
{
    if (!isSDCardInitialized)
    {
        return;
    }

    if (!IS_RTC_ENABLED)
    {
        sprintf(date_time, "%u", millis());
    }


    else
    {
        getDateTime();
    }

    char formatted_log_message[strlen(date_time) + strlen(message) + 10];
    snprintf(formatted_log_message, sizeof(formatted_log_message), "[%s] [INFO] %s", date_time, message);

    if (!file.open(filename, O_RDWR | O_CREAT | O_APPEND))
    {
        Serial.println("[ERROR] Log failed!");
        return;
    }

    file.println(formatted_log_message);
    file.close();

    if (IS_SERIAL_PRINT)
    {
        Serial.println(formatted_log_message);
    }
}

void ExtMEM::debug(const char *message)
{
    if (!isSDCardInitialized)
    {
        return;
    }

    if (!IS_RTC_ENABLED)
    {
        sprintf(date_time, "%u", millis());
    }
    else
    {
        getDateTime();
    }
    char formatted_log_message[strlen(date_time) + strlen(message) + 10];
    snprintf(formatted_log_message, sizeof(formatted_log_message), "[%s] [DEBUG] %s", date_time, message);

    if (!file.open(filename, O_RDWR | O_CREAT | O_APPEND))
    {
        Serial.println("[ERROR] Log failed!");
        return;
    }

    file.println(formatted_log_message);
    file.close();

    if (IS_SERIAL_PRINT)
    {
        Serial.println(formatted_log_message);
    }
}

void ExtMEM::warning(const char *message)
{
    if (!isSDCardInitialized)
    {
        return;
    }

    if (!IS_RTC_ENABLED)
    {
        sprintf(date_time, "%u", millis());
    }
    else
    {
        getDateTime();
    }

    char formatted_log_message[strlen(date_time) + strlen(message) + 10];
    snprintf(formatted_log_message, sizeof(formatted_log_message), "[%s] [WARNING] %s", date_time, message);

    if (!file.open(filename, O_RDWR | O_CREAT | O_APPEND))
    {
        Serial.println("[ERROR] Log failed!");
        return;
    }

    file.println(formatted_log_message);
    file.close();

    if (IS_SERIAL_PRINT)
    {
        Serial.println(formatted_log_message);
    }
}

void ExtMEM::error(const char *message)
{
    if (!isSDCardInitialized)
    {
        return;
    }

    if (!IS_RTC_ENABLED)
    {
        sprintf(date_time, "%u", millis());
    }
    else
    {
        getDateTime();
    }

    char formatted_log_message[strlen(date_time) + strlen(message) + 10];
    snprintf(formatted_log_message, sizeof(formatted_log_message), "[%s] [ERROR] %s", date_time, message);

    if (!file.open(filename, O_RDWR | O_CREAT | O_APPEND))
    {
        Serial.println("[ERROR] Log failed!");
        return;
    }

    file.println(formatted_log_message);
    file.close();

    if (IS_SERIAL_PRINT)
    {
        Serial.println(formatted_log_message);
    }
    getDateTime();
}

void ExtMEM::data(const char *message)
{
    if (!isSDCardInitialized)
    {
        return;
    }

    if (!file.open(filename, O_RDWR | O_CREAT | O_APPEND))
    {
        Serial.println("[ERROR] CSV Log failed!");
        return;
    }

    file.println(message);
    file.close();
}

void ExtMEM::readSN()
{
    digitalWrite(uSD_CS_PIN, LOW);
    delay(10);

    Serial.println("[INFO] Checking config file...");
    if (!sd.exists(CONFIG_FILENAME.c_str()))
    {
        Serial.println("[ERROR] Config file does not exist!");
        digitalWrite(uSD_CS_PIN, HIGH);
        return;
    }

    Serial.println("[INFO] Opening config file...");
    if (!file.open(CONFIG_FILENAME.c_str(), O_RDONLY))
    {
        Serial.println("[ERROR] Failed to open CSV file!");
        digitalWrite(uSD_CS_PIN, HIGH);
        return;
    }

    Serial.println("[INFO] Reading file...");
    char line[64];
    while (file.fgets(line, sizeof(line)))
    {
        char *token = strtok(line, ",");
        if (token)
            strncpy(config_data.asn, token, sizeof(config_data.asn));

        token = strtok(NULL, ",");
        if (token)
            strncpy(config_data.sn, token, sizeof(config_data.sn));

        token = strtok(NULL, ",");
        if (token)
            strncpy(config_data.hw, token, sizeof(config_data.hw));

        token = strtok(NULL, ",");
        if (token)
            strncpy(config_data.fw, token, sizeof(config_data.fw));

        if (IS_SERIAL_PRINT)
        {
            Serial.print("ASN: ");
            Serial.println(config_data.asn);
            Serial.print("SN: ");
            Serial.println(config_data.sn);
            Serial.print("HW: ");
            Serial.println(config_data.hw);
            Serial.print("FW: ");
            Serial.println(config_data.fw);
        }
    }

    file.close();
    digitalWrite(uSD_CS_PIN, HIGH);
}

void ExtMEM::getDateTime()
{
    DateTime now = get_rtc_datetime();

    snprintf(date_time, sizeof(date_time), "%04d/%02d/%02d %02d:%02d:%02d",
             now.year,
             now.month,
             now.day,
             now.hours,
             now.minutes,
             now.seconds);
}