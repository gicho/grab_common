/**
 * @file goldsolowhistledrive.h
 * @author Edoardo Idà, Simone Comari
 * @date 25 Sep 2018
 * @brief File containing _Gold Solo Whistle Drive_ slave interface to be included in the
 * GRAB
 * ethercat library.
 */

#ifndef GRABCOMMON_LIBGRABEC_GOLDSOLOWHISTLEDRIVE_H
#define GRABCOMMON_LIBGRABEC_GOLDSOLOWHISTLEDRIVE_H

#include <bitset>
#include <iostream>
#include "ethercatslave.h"
#include "types.h"
#include "grabcommon.h"
#include "state_machine/inc/StateMachine.h"

namespace grabec
{
/**
 * @brief Gold Solo Whistle Drive _states_ as in physical drive documentation.
 *
 * For further details, click
 * <a href="https://www.elmomc.com/members/NetHelp1/Elmo.htm#!objects.htm">here</a>.
 */
enum GoldSoloWhistleDriveStates : BYTE
{
  ST_START,
  ST_NOT_READY_TO_SWITCH_ON,
  ST_SWITCH_ON_DISABLED,
  ST_READY_TO_SWITCH_ON,
  ST_SWITCHED_ON,
  ST_OPERATION_ENABLED,
  ST_QUICK_STOP_ACTIVE,
  ST_FAULT_REACTION_ACTIVE,
  ST_FAULT,
  ST_MAX_STATES
};

/**
 * @brief Gold Solo Whistle _operation modes_.
 *
 * Only few values are considered here for our purposes. For further details, click <a
 * href="https://www.elmomc.com/members/NetHelp1/Elmo.htm#!object0x6060modesofo.htm">here</a>.
 */
enum GoldSoloWhistleOperationModes : int8_t
{
  NULL_OPERATION = -1,
  CYCLIC_POSITION = 8,
  CYCLIC_VELOCITY = 9,
  CYCLIC_TORQUE = 10,
};

enum Commands : uint8_t
{
  UNSET,
  SET
};

/**
 * @brief The GoldSoloWhistleDriveData class.
 *
 * This is the data type to be passed when transitioning to OPERATION ENABLED state
 * according to our implementation of drive interface's state machine.
 * For further details about state machine, click <a
 * href="https://www.codeproject.com/Articles/1087619/State-Machine-Design-in-Cplusplus">here</a>.
 */
class GoldSoloWhistleDriveData : public EventData
{
public:
  /**
   * @brief Full constructor.
   * @param[in] _op_mode The desired operation mode of the drive. See
   * GoldSoloWhistleOperationModes for valid accounted entries.
   * @param[in] _value The target set point for the desired operation mode (position,
   * velocity
   * or torque).
   * @attention Torque setpoint is automaticallly casted to INT16, so values needs to be
   * in range [–32,768, 32,767].
   */
  GoldSoloWhistleDriveData(const int8_t _op_mode, const int32_t _value = 0);

  int8_t op_mode = NULL_OPERATION; /**< The desired operation mode of the drive. */
  int32_t value = 0; /**< The target set point for the desired operation mode. */
};

/**
 * @brief The Gold Solo Whistle Drive class.
 *
 * This class provides an interface to the physical drive threated here as an ethercat
 * slave. As its physical component, this class includes a state machine, which represents
 * the status of the physical drive itself. The user can trigger a state transition by
 * means of @ref ExternalEvents. The basic ones resemble the external
 * events taken by the physical drive and can be found <a
 * href="https://www.elmomc.com/members/NetHelp1/Elmo.htm#!object0x6040controlw.htm">here</a>.
 * On top of those, setpoint changing events can be used to modify the target position,
 * velocity or torque in OPERATION_ENABLED mode.
 * @note The events only request a state transition carried on by WriteOutputs(), but the
 * actual state depends of the current state of the physical drive, obtained by
 * ReadInputs(). Hence, it is recommended to always check the transition effectively
 * occured before requesting a new one. Use GetCurrentState() inherited function for this
 * purpose.
 * @todo implement:
 * - fast stop during operation
 * - reaction to a specific fault
 */
class GoldSoloWhistleDrive : public StateMachine, virtual public EthercatSlave
{
public:
  /**
   * @brief Constructor.
   * @param[in] slave_position Slave position in ethercat chain.
   */
  GoldSoloWhistleDrive(const uint8_t slave_position);
  ~GoldSoloWhistleDrive() {}

  /**
   * @defgroup ExternalEvents GoldSoloWhistle Drive External Events
   * The external events which user can call to trigger a state transition. For furter
   * details
   * click <a
   * href="https://www.elmomc.com/members/NetHelp1/Elmo.htm#!object0x6040controlw.htm">here</a>.
   * @{
   */
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  //// External events resembling the ones internal to physical drive
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /**
   * @brief _Shutdown_ external event.
   *
   * Triggers following transitions:
   * - SWITCH ON DISABLED --> READY TO SWITCH ON
   * - SWITCHED ON --> READY TO SWITCH ON
   * - OPERATION ENABLED --> READY TO SWITCH ON
   */
  void Shutdown();
  /**
   * @brief _Switch ON_ external event.
   *
   * Triggers following transition:
   * - READY TO SWITCH ON --> SWITCHED ON
   */
  void SwitchOn();
  /**
   * @brief _Enable Operation_ external event.
   *
   * Triggers following transitions:
   * - SWITCHED ON --> OPERATION ENABLED
   * - QUICK STOP ACTIVE --> OPERATION ENABLED
   */
  void EnableOperation();
  /**
   * @brief Disable Operation external event.
   *
   * Triggers following transition:
   * - OPERATION ENABLED --> SWITCHED ON
   */
  void DisableOperation();
  /**
   * @brief Disable Voltage external event.
   *
   * Triggers following transitions:
   * - READY TO SWITCH ON --> SWITCH ON DISABLED
   * - OPERATION ENABLED --> SWITCH ON DISABLED
   * - SWITCHED ON --> SWITCH ON DISABLED
   * - QUICK STOP ACTIVE --> SWITCH ON DISABLED
   */
  void DisableVoltage();
  /**
   * @brief Quick Stop external event.
   *
   * Triggers following transitions:
   * - READY TO SWITCH ON --> SWITCH ON DISABLED
   * - SWITCHED ON --> SWITCH ON DISABLED
   * - OPERATION ENABLED --> QUICK STOP ACTIVE
   */
  void QuickStop();
  /**
   * @brief Fault Reset external event.
   *
   * Triggers following transition:
   * - FAULT --> SWITCH ON DISABLED
   */
  void FaultReset();
  //////////////////////////////////////////////////////////////////////////////////////////
  //// Additional external events taken by this state machine
  ///  when it's operational
  //////////////////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Change Position external event.
   *
   * If drive is in in OPERATION ENABLED state, it changes _operation mode_ to
   * CYCLIC_POSITION and sets target position to @c target_position.
   * @param[in] target_position New position setpoint in user-defined units.
   */
  void ChangePosition(const int32_t target_position);
  /**
   * @brief ChangeDeltaPosition
   * @param delta_position
   */
  void ChangeDeltaPosition(const int32_t delta_position);
  /**
   * @brief Change Velocity external event.
   *
   * If drive is in in OPERATION ENABLED state, it changes _operation mode_ to
   * CYCLIC_VELOCITY and sets target velocity to @c target_velocity.
   * @param[in] target_velocity New velocity setpoint in user-defined units.
   */
  void ChangeVelocity(const int32_t target_velocity);
  /**
   * @brief ChangeDeltaVelocity
   * @param delta_velocity
   */
  void ChangeDeltaVelocity(const int32_t delta_velocity);
  /**
   * @brief Change Torque external event.
   *
   * If drive is in in OPERATION ENABLED state, it changes _operation mode_ to
   * CYCLIC_TORQUE and sets target torque to @c target_torque.
   * @param[in] target_torque New torque setpoint in user-defined units.
   */
  void ChangeTorque(const int16_t target_torque);
  /**
   * @brief ChangeDeltaTorque
   * @param delta_torque
   */
  void ChangeDeltaTorque(const int16_t delta_torque);
  /**
   * @brief Change Operation Mode external event.
   *
   * If drive is in in OPERATION ENABLED state, it changes _operation mode_ to
   * @c target_op_mode and sets corresponding target to its current value.
   * @param[in] target_op_mode New _operation mode_.
   */
  void ChangeOpMode(const int8_t target_op_mode);
  /**
   * @brief SetTargetDefaults external event.
   *
   * If drive is in in OPERATION ENABLED state, according to current _operation mode_, it
   * sets corresponding target value to its current value.
   */
  void SetTargetDefaults();
  /** @} */

  /**
   * @brief Get latest known physical drive state.
   * @return Latest known physical drive state.
   */
  GoldSoloWhistleDriveStates GetDriveState() const;
  /**
   * @brief Get actual drive position (aka counts).
   * @return Actual drive position (aka counts).
   */
  int32_t GetPosition() const { return  input_pdos_.pos_actual_value; }
  /**
   * @brief Get actual drive auxiliary position (aka counts).
   *
   * This field can be used by external sensors connected to the drive, for instance an
   * additional encoder.
   * @return Actual drive auxiliary position (aka counts).
   */
  int GetAuxPosition() const { return  input_pdos_.aux_pos_actual_value; }

  /**
   * @brief ReadInputs
   */
  void ReadInputs() override final;
  /**
   * @brief WriteOutputs
   */
  void WriteOutputs() override final;

protected:
  // A simple way to store the pdos input values
  struct InputPdos
  {
    Bitfield16 status_word;
    int8_t display_op_mode;
    int32_t pos_actual_value;
    int32_t vel_actual_value;
    int16_t torque_actual_value;
    uint digital_inputs;
    int aux_pos_actual_value;
  } input_pdos_;

  // A simple way to store the pdos output values
  struct OutputPdos
  {
    Bitfield16 control_word;
    int8_t op_mode;
    int16_t target_torque;
    int32_t target_position;
    int32_t target_velocity;
  } output_pdos_;

  GoldSoloWhistleDriveStates drive_state_;

private:
  static constexpr uint8_t kDomainInputs = 7;
  static constexpr uint8_t kDomainOutputs = 5;
  static constexpr uint8_t kDomainEntries = kDomainInputs + kDomainOutputs;
  static constexpr uint8_t kAlias = 0;
  static constexpr uint32_t kVendorID = 0x0000009a;
  static constexpr uint32_t kProductCode = 0x00030924;
  static constexpr uint16_t kControlWordIdx = 0x6040;
  static constexpr uint8_t kControlWordSubIdx = 0x00;
  static constexpr uint16_t kHomingMethodIdx = 0x6098;
  static constexpr uint8_t kHomingMethodSubIdx = 0x00;
  static constexpr uint16_t kOpModeIdx = 0x6060;
  static constexpr uint8_t kOpModeSubIdx = 0x00;
  static constexpr uint16_t kTargetTorqueIdx = 0x6071;
  static constexpr uint8_t kTargetTorqueSubIdx = 0x00;
  static constexpr uint16_t kTargetPosIdx = 0x607a;
  static constexpr uint8_t kTargetPosSubIdx = 0x00;
  static constexpr uint16_t kTargetVelIdx = 0x60FF;
  static constexpr uint8_t kTargetVelSubIdx = 0x00;
  static constexpr uint16_t kStatusWordIdx = 0x6041;
  static constexpr uint8_t kStatusWordSubIdx = 0x00;
  static constexpr uint16_t kDisplayOpModeIdx = 0x6061;
  static constexpr uint8_t kDisplayOpModeSubIdx = 0x00;
  static constexpr uint16_t kPosActualValueIdx = 0x6064;
  static constexpr uint8_t kPosActualValueSubIdx = 0x00;
  static constexpr uint16_t kVelActualValueIdx = 0x606C;
  static constexpr uint8_t kVelActualValueSubIdx = 0x00;
  static constexpr uint16_t kTorqueActualValueIdx = 0x6077;
  static constexpr uint8_t kTorqueActualValueSubIdx = 0x00;
  static constexpr uint16_t kDigInIndex = 0x60FD;
  static constexpr uint8_t kDigInSubIndex = 0x00;
  static constexpr uint16_t kAuxPosActualValueIdx = 0x20A0;
  static constexpr uint8_t kAuxPosActualValueSubIdx = 0x00;
  static constexpr uint8_t kHomingOnPosMethod = 35;
  static constexpr uint8_t kNumSupportedOperations = 3;
  static constexpr uint8_t kOperationOffset = 8;

  // ethercat utilities, can be retrieved in the xml config file provided by the vendor
  static constexpr ec_pdo_entry_info_t kPdoEntries[kDomainEntries] = {
    {kControlWordIdx, kControlWordSubIdx, 16}, // Start of RxPdo mapping (Outputs)
    {kOpModeIdx, kOpModeSubIdx, 8},
    {kTargetTorqueIdx, kTargetTorqueSubIdx, 16},
    {kTargetPosIdx, kTargetPosSubIdx, 32},
    {kTargetVelIdx, kTargetVelSubIdx, 32},
    {kStatusWordIdx, kStatusWordSubIdx, 16}, // Start of TxPdo mapping (Inputs)
    {kDisplayOpModeIdx, kDisplayOpModeSubIdx, 8},
    {kPosActualValueIdx, kPosActualValueSubIdx, 32},
    {kVelActualValueIdx, kVelActualValueSubIdx, 32},
    {kTorqueActualValueIdx, kTorqueActualValueSubIdx, 16},
    {kDigInIndex, kDigInSubIndex, 32},
    {kAuxPosActualValueIdx, kAuxPosActualValueSubIdx, 32}};

  // ethercat utilities, can be retrieved in the xml config file provided by the vendor
  static constexpr ec_pdo_info_t kPDOs[2] = {
    {0x1607, 5, const_cast<ec_pdo_entry_info_t*>(kPdoEntries) + 0}, /* Outputs */
    {0x1a07, 7, const_cast<ec_pdo_entry_info_t*>(kPdoEntries) + 5}, /* Inputs */
  };

  static constexpr ec_sync_info_t kSyncs[5] = {
    {0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE},
    {1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE},
    {2, EC_DIR_OUTPUT, 1, const_cast<ec_pdo_info_t*>(kPDOs) + 0, EC_WD_ENABLE},
    {3, EC_DIR_INPUT, 1, const_cast<ec_pdo_info_t*>(kPDOs) + 1, EC_WD_DISABLE},
    {0xff, EC_DIR_INVALID, 0, 0x00, EC_WD_DEFAULT}};

  static constexpr char* kStatesStr[] = {
    const_cast<char*>("START"), const_cast<char*>("NOT_READY_TO_SWITCH_ON"),
    const_cast<char*>("NOT_SWITCH_ON_DISABLED"), const_cast<char*>("READY_TO_SWITCH_ON"),
    const_cast<char*>("SWITCHED_ON"), const_cast<char*>("OPERATION_ENABLED"),
    const_cast<char*>("QUICK_STOP_ACTIVE"), const_cast<char*>("FAULT_REACTION_ACTIVE"),
    const_cast<char*>("FAULT"), const_cast<char*>("MAX_STATE")};

  // Useful ethercat struct
  struct OffsetOut
  {
    unsigned int control_word;
    unsigned int op_mode;
    unsigned int target_torque;
    unsigned int target_position;
    unsigned int target_velocity;
  } offset_out_;

  // Useful ethercat struct
  struct OffsetIn
  {
    unsigned int status_word;
    unsigned int display_op_mode;
    unsigned int position_actual_value;
    unsigned int velocity_actual_value;
    unsigned int torque_actual_value;
    unsigned int digital_inputs;
    unsigned int aux_pos_actual_value;
  } offset_in_;

  // clang-format off
  ENUM_CLASS(StatusBit,
             READY_TO_SWITCH_ON = 0,
             SWITCHED_ON = 1,
             OPERATION_ENABLED = 2,
             FAULT = 3,
             QUICK_STOP = 5,
             SWITCH_ON_DISABLED = 6)

  ENUM_CLASS(ControlBit,
             SWITCH_ON = 0,
             ENABLE_VOLTAGE = 1,
             QUICK_STOP = 2,
             ENABLE_OPERATION = 3,
             FAULT = 7)
  // clang-format on

  GoldSoloWhistleDriveStates prev_state_;
  ec_pdo_entry_reg_t domain_registers_[kDomainEntries]; // ethercat utilities

  // Define the state machine state functions with event data type
  STATE_DECLARE(GoldSoloWhistleDrive, Start, NoEventData)
  STATE_DECLARE(GoldSoloWhistleDrive, NotReadyToSwitchOn, NoEventData)
  STATE_DECLARE(GoldSoloWhistleDrive, SwitchOnDisabled, NoEventData)
  STATE_DECLARE(GoldSoloWhistleDrive, ReadyToSwitchOn, NoEventData)
  STATE_DECLARE(GoldSoloWhistleDrive, SwitchedOn, NoEventData)
  STATE_DECLARE(GoldSoloWhistleDrive, OperationEnabled, GoldSoloWhistleDriveData)
  STATE_DECLARE(GoldSoloWhistleDrive, QuickStopActive, NoEventData)
  STATE_DECLARE(GoldSoloWhistleDrive, FaultReactionActive, NoEventData)
  STATE_DECLARE(GoldSoloWhistleDrive, Fault, NoEventData)

  // State map to define state object order. Each state map entry defines a state object.
  BEGIN_STATE_MAP
  // clang-format off
    STATE_MAP_ENTRY({&Start})
    STATE_MAP_ENTRY({&NotReadyToSwitchOn})
    STATE_MAP_ENTRY({&SwitchOnDisabled})
    STATE_MAP_ENTRY({&ReadyToSwitchOn})
    STATE_MAP_ENTRY({&SwitchedOn})
    STATE_MAP_ENTRY({&OperationEnabled})
    STATE_MAP_ENTRY({&QuickStopActive})
    STATE_MAP_ENTRY({&FaultReactionActive})
    STATE_MAP_ENTRY({&Fault})
  // clang-format on
  END_STATE_MAP

  void SetChange(const GoldSoloWhistleDriveData& data);

  RetVal SdoRequests(ec_slave_config_t* config_ptr) override final;
  void DoWork() override {}
  void InitFun() override {}

  inline void PrintCommand(const char* cmd) const;
  void PrintStateTransition(const GoldSoloWhistleDriveStates current_state,
                            const GoldSoloWhistleDriveStates new_state) const;
};

} // end namespace grabec

#endif // GRABCOMMON_LIBGRABEC_GOLDSOLOWHISTLEDRIVE_H