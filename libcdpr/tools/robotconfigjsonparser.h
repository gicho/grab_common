/**
 * @file robotconfigjsonparser.h
 * @author Simone Comari
 * @date 13 Mar 2019
 * @brief This file include a parser for JSON configuration file for GRAB CDPR.
 */

#ifndef GRABCOMMON_LIBCDPR_ROBOTCONFIGJSONPARSER_H
#define GRABCOMMON_LIBCDPR_ROBOTCONFIGJSONPARSER_H

#include <fstream>

#include <QString>

#include "grabcommon.h"
#include "json.hpp"
#include "libcdpr/inc/types.h"

using json = nlohmann::json; /**< JSON library support alias */

/**
 * @brief A parser for JSON configuration file for GRAB CDPR.
 */
class RobotConfigJsonParser
{
 public:
  /**
   * @brief RobotConfigJsonParser default constructor.
   */
  RobotConfigJsonParser();

  /**
   * @brief Parse a JSON configuration file.
   * @param[in] filename Configuration filepath.
   * @param[in] verbose If _true_, prints content of the parsed file.
   * @return _True_ if file was correctly parsed, _false_ otherwise.
   */
  bool ParseFile(const std::string& filename, const bool verbose = false);
  /**
   * @brief Parse a JSON configuration file.
   * @param[in] filename Configuration filepath.
   * @param[in] verbose If _true_, prints content of the parsed file.
   * @return _True_ if file was correctly parsed, _false_ otherwise.
   */
  bool ParseFile(const char* filename, const bool verbose = false);
  /**
   * @brief Parse a JSON configuration file.
   * @param[in] filename Configuration filepath.
   * @param[in] verbose If _true_, prints content of the parsed file.
   * @return _True_ if file was correctly parsed, _false_ otherwise.
   */
  bool ParseFile(const QString& filename, const bool verbose = false);
  /**
   * @brief Parse a JSON configuration file and fills a parameters structure.
   * @param[in] filename Configuration filepath.
   * @param[out] params Parameters structure to be filled with parsed data.
   * @param[in] verbose If _true_, prints content of the parsed file.
   * @return _True_ if file was correctly parsed, _false_ otherwise.
   */
  bool ParseFile(const std::string& filename, grabcdpr::Params* const params,
                 const bool verbose = false);
  /**
   * @brief Parse a JSON configuration file and fills a parameters structure.
   * @param[in] filename Configuration filepath.
   * @param[out] params Parameters structure to be filled with parsed data.
   * @param[in] verbose If _true_, prints content of the parsed file.
   * @return _True_ if file was correctly parsed, _false_ otherwise.
   */
  bool ParseFile(const char* filename, grabcdpr::Params* const params,
                 const bool verbose = false);
  /**
   * @brief Parse a JSON configuration file and fills a parameters structure.
   * @param[in] filename Configuration filepath.
   * @param[out] params Parameters structure to be filled with parsed data.
   * @param[in] verbose If _true_, prints content of the parsed file.
   * @return _True_ if file was correctly parsed, _false_ otherwise.
   */
  bool ParseFile(const QString& filename, grabcdpr::Params* const params,
                 const bool verbose = false);

  /**
   * @brief Get parsed configuration structure.
   * @return A configuration structure.
   * @warning If file was not correctly parsed yet, it returns an empty structure without
   * errors or warnings.
   */
  grabcdpr::Params GetConfigStruct() const { return config_params_; }
  /**
   * @brief Get parsed configuration structure.
   * @param[out] params The configuration structure to be filled with parsed data.
   * @warning If file was not correctly parsed yet, it returns an empty structure without
   * errors or warnings.
   */
  void GetConfigStruct(grabcdpr::Params* const params) const { *params = config_params_; }

  /**
   * @brief Print parsed configuration parameters set, if present.
   */
  void PrintConfig() const;

 private:
  grabcdpr::Params config_params_;
  bool file_parsed_ = false;

  bool ExtractConfig(const json& raw_data);
  bool ExtractPlatform(const json& raw_data);
  bool ExtractActuators(const json& raw_data);

  bool ArePlatformParamsValid() const;
  bool AreCableParamsValid(const grabcdpr::ActuatorParams& params) const;
};

#endif // GRABCOMMON_LIBCDPR_ROBOTCONFIGJSONPARSER_H
