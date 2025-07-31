#ifndef LOGS_HPP
#define LOGS_HPP

// Framework libs
#include <SPI.h>
#include <chrono>
#include <SdFat.h>
#include <string>

// Local Includes
#include <config.hpp>

// Defines and Global Variables
// -

/// ExtMEM
/// @brief Class that implements a data logger defined by its different
/// debug levels
///
class ExtMEM
{
public:
  // Public methods

  /// ExtMEM
  /// @brief Class constructor
  ///
  /// @param[in] none
  ///
  /// @return none
  ///
  ExtMEM();

  /// initExtMem
  /// @brief Starts SD card (checks every 10s to prevent card removal)
  ///        and log an info message in case of success.
  ///
  /// @param none
  ///
  /// @return true or false in case of success/fail for SD card start
  ///
  bool initExtMem();

  /// initFile
  /// @brief Initializes the file for logging 
  /// 
  /// @param[in] type: Type of file to initialize
  ///
  /// @return true or false in case of success/fail for file initialization
  ///
  bool initFile(const char *type);

  /// info
  /// @brief Information to be stored in the file
  ///
  /// @param[in] data: Messsage to send
  ///
  /// @return none
  void info(const char *data);

  /// debug
  /// @brief Information to be stored in the file
  ///
  /// @param[in] data: Messsage to send
  ///
  /// @return none
  void debug(const char *data);

  /// warning
  /// @brief Information to be stored in the file
  ///
  /// @param[in] data: Messsage to send
  ///
  /// @return none
  void warning(const char *data);

  /// error
  /// @brief Information to be stored in the file
  ///
  /// @param[in] data: Messsage to send
  ///
  /// @return none
  void error(const char *data);

  /// csv
  /// @brief Information to be stored in the file
  ///
  /// @param[in] type:
  /// @param[in] message:
  ///
  /// @return none
  void data(const char *message);

  /// readSN
  /// @brief Reads the serial number from the SD card
  ///
  /// @param[in] none
  /// @return none
  void readSN();

private:
  // Private attributes
  bool isLogFileOpen;
  bool isCSVFileOpen;
  bool isSDCardInitialized;
  char filename[30];  // Cada instância tem o seu filename
  int fileIndex;
};

#endif // ExtMEM_HPP_INCLUDED_