/**
 * @file goldsolowhistledrive.cpp
 * @author Edoardo Idà, Simone Comari
 * @date 25 Sep 2018
 * @brief File containing class implementation declared in goldsolowhistledrive.h.
 */

#include "slaves/goldsolowhistledrive.h"

namespace grabec
{

////////////////////////////////////////////////////////////////////////////
//// GoldSoloWhistleDriveData
////////////////////////////////////////////////////////////////////////////

GoldSoloWhistleDriveData::GoldSoloWhistleDriveData(const int8_t _op_mode,
                                                   const int32_t _value /*= 0*/)
  : op_mode(_op_mode), value(_value)
{
}

////////////////////////////////////////////////////////////////////////////
//// GoldSoloWhistleDrive
////////////////////////////////////////////////////////////////////////////

GoldSoloWhistleDrive::GoldSoloWhistleDrive(const uint8_t slave_position)
  : StateMachine(ST_MAX_STATES)
{
  alias_ = kAlias;
  position_ = slave_position;
  vendor_id_ = kVendorID;
  product_code_ = kProductCode;
  num_domain_entries_ = kDomainEntries;

  domain_registers_[0] = {alias_, position_, vendor_id_, product_code_, kControlWordIdx,
                          kControlWordSubIdx, &offset_out_.control_word, NULL};
  domain_registers_[1] = {alias_, position_, vendor_id_, product_code_, kOpModeIdx,
                          kOpModeSubIdx, &offset_out_.op_mode, NULL};
  domain_registers_[2] = {alias_, position_, vendor_id_, product_code_, kTargetTorqueIdx,
                          kTargetTorqueSubIdx, &offset_out_.target_torque, NULL};
  domain_registers_[3] = {alias_, position_, vendor_id_, product_code_, kTargetPosIdx,
                          kTargetPosSubIdx, &offset_out_.target_position, NULL};
  domain_registers_[4] = {alias_, position_, vendor_id_, product_code_, kTargetVelIdx,
                          kTargetVelSubIdx, &offset_out_.target_velocity, NULL};
  domain_registers_[5] = {alias_, position_, vendor_id_, product_code_, kStatusWordIdx,
                          kStatusWordSubIdx, &offset_in_.status_word, NULL};
  domain_registers_[6] = {alias_, position_, vendor_id_, product_code_, kDisplayOpModeIdx,
                          kDisplayOpModeSubIdx, &offset_in_.display_op_mode, NULL};
  domain_registers_[7] = {alias_, position_, vendor_id_, product_code_,
                          kPosActualValueIdx, kPosActualValueSubIdx,
                          &offset_in_.position_actual_value, NULL};
  domain_registers_[8] = {alias_, position_, vendor_id_, product_code_,
                          kVelActualValueIdx, kVelActualValueSubIdx,
                          &offset_in_.velocity_actual_value, NULL};
  domain_registers_[9] = {alias_, position_, vendor_id_, product_code_,
                          kTorqueActualValueIdx, kTorqueActualValueSubIdx,
                          &offset_in_.torque_actual_value, NULL};
  domain_registers_[10] = {alias_, position_, vendor_id_, product_code_, kDigInIndex,
                           kDigInSubIndex, &offset_in_.digital_inputs, NULL};
  domain_registers_[11] = {alias_, position_, vendor_id_, product_code_,
                           kAuxPosActualValueIdx, kAuxPosActualValueSubIdx,
                           &offset_in_.aux_pos_actual_value, NULL};

  domain_registers_ptr_ = domain_registers_;
  slave_pdo_entries_ptr_ = const_cast<ec_pdo_entry_info_t*>(kPdoEntries);
  slave_pdos_ptr_ = const_cast<ec_pdo_info_t*>(kPDOs);
  slave_sync_ptr_ = const_cast<ec_sync_info_t*>(kSyncs);

  drive_state_ = ST_START;
  prev_state_ = static_cast<GoldSoloWhistleDriveStates>(GetCurrentState());

  InternalEvent(drive_state_);
}

GoldSoloWhistleDriveStates GoldSoloWhistleDrive::GetDriveState() const
{
  if (input_pdos_.status_word[StatusBit::SWITCH_ON_DISABLED] == SET)
  { // drive idle: OFF=true
    return ST_NOT_READY_TO_SWITCH_ON;
  }
  if (input_pdos_.status_word[StatusBit::QUICK_STOP] == SET)
  {
    if (input_pdos_.status_word[StatusBit::SWITCHED_ON] == UNSET)
    { // drive in operational progress: OFF=false, ON=true, SWITCH_ON=false
      return ST_READY_TO_SWITCH_ON;
    }
    if (input_pdos_.status_word[StatusBit::OPERATION_ENABLED] == UNSET)
    { // drive in operational progress: OFF=false, ON=true, SWITCH_ON=true, ENABLED=false
      return ST_SWITCHED_ON;
    }
    // drive operational: OFF=false, ON=true, SWITCH_ON=true, ENABLED=true
    return ST_OPERATION_ENABLED;
  }
  if (input_pdos_.status_word[StatusBit::FAULT] == UNSET)
  { // drive in quick stop: OFF=false, ON=false, FAULT=false
    return ST_QUICK_STOP_ACTIVE;
  }
  if (input_pdos_.status_word[StatusBit::OPERATION_ENABLED] == SET)
  { // drive in fault reaction: OFF=false, ON=false, FAULT=true, ENABLED=true
    return ST_FAULT_REACTION_ACTIVE;
  }
  // drive in fault: OFF=false, ON=false, FAULT=true, ENABLED=false
  return ST_FAULT;
}

////////////////////////////////////////////////////////////////////////////
//// Overwritten virtual functions from base class
////////////////////////////////////////////////////////////////////////////

RetVal GoldSoloWhistleDrive::SdoRequests(ec_slave_config_t* config_ptr)
{
  static ec_sdo_request_t* sdo_ptr = NULL;

  if (!(sdo_ptr = ecrt_slave_config_create_sdo_request(config_ptr, kOpModeIdx,
                                                       kOpModeSubIdx, CYCLIC_POSITION)))
  {
    std::cout << "Failed to create SDO request." << std::endl;
    return ECONFIG;
  }
  ecrt_sdo_request_timeout(sdo_ptr, 500);
  ecrt_slave_config_sdo8(config_ptr, kOpModeIdx, kOpModeSubIdx, CYCLIC_POSITION);
  if (!(sdo_ptr = ecrt_slave_config_create_sdo_request(
          config_ptr, kHomingMethodIdx, kHomingMethodSubIdx, kHomingOnPosMethod)))
  {
    std::cout << "Failed to create SDO request." << std::endl;
    return ECONFIG;
  }
  ecrt_sdo_request_timeout(sdo_ptr, 500);
  ecrt_slave_config_sdo8(config_ptr, kHomingMethodIdx, kHomingMethodSubIdx,
                         kHomingOnPosMethod);
  return OK;
}

void GoldSoloWhistleDrive::ReadInputs()
{
  input_pdos_.status_word.SetBitset(
    EC_READ_U16(domain_data_ptr_ + offset_in_.status_word));
  input_pdos_.display_op_mode = EC_READ_S8(domain_data_ptr_ + offset_in_.display_op_mode);
  input_pdos_.pos_actual_value =
    EC_READ_S32(domain_data_ptr_ + offset_in_.position_actual_value);
  input_pdos_.vel_actual_value =
    EC_READ_S32(domain_data_ptr_ + offset_in_.velocity_actual_value);
  input_pdos_.torque_actual_value =
    EC_READ_S16(domain_data_ptr_ + offset_in_.torque_actual_value);
  input_pdos_.digital_inputs = EC_READ_U32(domain_data_ptr_ + offset_in_.digital_inputs);
  input_pdos_.aux_pos_actual_value =
    EC_READ_S32(domain_data_ptr_ + offset_in_.aux_pos_actual_value);

  drive_state_ = GetDriveState();
  if (drive_state_ != GetCurrentState())
  {
    if (drive_state_ == ST_OPERATION_ENABLED)
      ChangeOpMode(input_pdos_.display_op_mode);
    else
      InternalEvent(drive_state_);
  }
}

void GoldSoloWhistleDrive::WriteOutputs()
{
  EC_WRITE_U16(domain_data_ptr_ + offset_out_.control_word,
               output_pdos_.control_word.GetBitset().to_ulong());
  EC_WRITE_S8(domain_data_ptr_ + offset_out_.op_mode, output_pdos_.op_mode);
  if (drive_state_ == ST_OPERATION_ENABLED || drive_state_ == ST_SWITCHED_ON)
  {
    EC_WRITE_S32(domain_data_ptr_ + offset_out_.target_position,
                 output_pdos_.target_position);
    EC_WRITE_S32(domain_data_ptr_ + offset_out_.target_velocity,
                 output_pdos_.target_velocity);
    EC_WRITE_S16(domain_data_ptr_ + offset_out_.target_torque,
                 output_pdos_.target_torque);
  }
}

////////////////////////////////////////////////////////////////////////////
//// External events taken by this state machine
////////////////////////////////////////////////////////////////////////////

void GoldSoloWhistleDrive::Shutdown()
{
  PrintCommand("Shutdown");
  output_pdos_.control_word.Clear(ControlBit::SWITCH_ON);
  output_pdos_.control_word.Set(ControlBit::ENABLE_VOLTAGE);
  output_pdos_.control_word.Set(ControlBit::QUICK_STOP);
  output_pdos_.control_word.Clear(ControlBit::FAULT);
}

void GoldSoloWhistleDrive::SwitchOn()
{
  PrintCommand("SwitchOn");
  // Trigger device control command
  output_pdos_.control_word.Set(ControlBit::SWITCH_ON);
  output_pdos_.control_word.Set(ControlBit::ENABLE_VOLTAGE);
  output_pdos_.control_word.Set(ControlBit::QUICK_STOP);
  output_pdos_.control_word.Clear(ControlBit::ENABLE_OPERATION);
  output_pdos_.control_word.Clear(ControlBit::FAULT);
  // Setup default operational mode before enabling the drive
  output_pdos_.op_mode = CYCLIC_POSITION;
  output_pdos_.target_position = input_pdos_.pos_actual_value;
}

void GoldSoloWhistleDrive::EnableOperation()
{
  PrintCommand("EnableOperation");
  // Trigger device control command
  output_pdos_.control_word.Set(ControlBit::SWITCH_ON);
  output_pdos_.control_word.Set(ControlBit::ENABLE_VOLTAGE);
  output_pdos_.control_word.Set(ControlBit::QUICK_STOP);
  output_pdos_.control_word.Set(ControlBit::ENABLE_OPERATION);
  output_pdos_.control_word.Clear(ControlBit::FAULT);
}

void GoldSoloWhistleDrive::DisableOperation()
{
  PrintCommand("DisableOperation");
  // Trigger device control command
  output_pdos_.control_word.Set(ControlBit::SWITCH_ON);
  output_pdos_.control_word.Set(ControlBit::ENABLE_VOLTAGE);
  output_pdos_.control_word.Set(ControlBit::QUICK_STOP);
  output_pdos_.control_word.Clear(ControlBit::ENABLE_OPERATION);
  output_pdos_.control_word.Clear(ControlBit::FAULT);
}

void GoldSoloWhistleDrive::DisableVoltage()
{
  PrintCommand("DisableVoltage");
  // Trigger device control command
  output_pdos_.control_word.Clear(ControlBit::ENABLE_VOLTAGE);
  output_pdos_.control_word.Clear(ControlBit::FAULT);
}

void GoldSoloWhistleDrive::QuickStop()
{
  PrintCommand("QuickStop");
  // Trigger device control command
  output_pdos_.control_word.Set(ControlBit::ENABLE_VOLTAGE);
  output_pdos_.control_word.Clear(ControlBit::QUICK_STOP);
  output_pdos_.control_word.Clear(ControlBit::FAULT);
}

void GoldSoloWhistleDrive::FaultReset()
{
  PrintCommand("FaultReset");
  // Trigger device control command
  output_pdos_.control_word.Set(ControlBit::FAULT);
}

void GoldSoloWhistleDrive::ChangePosition(const int32_t target_position)
{
  PrintCommand("ChangePosition");
  printf("\tTarget position: %d\n", target_position);

  GoldSoloWhistleDriveData data(CYCLIC_POSITION, target_position);
  SetChange(data);
}

void GoldSoloWhistleDrive::ChangeDeltaPosition(const int32_t delta_position)
{
  ChangePosition(input_pdos_.pos_actual_value + delta_position);
}

void GoldSoloWhistleDrive::ChangeVelocity(const int32_t target_velocity)
{
  PrintCommand("ChangeVelocity");
  printf("\tTarget velocity: %d\n", target_velocity);

  GoldSoloWhistleDriveData data(CYCLIC_VELOCITY, target_velocity);
  SetChange(data);
}

void GoldSoloWhistleDrive::ChangeDeltaVelocity(const int32_t delta_velocity)
{
  ChangeVelocity(input_pdos_.vel_actual_value + delta_velocity);
}

void GoldSoloWhistleDrive::ChangeTorque(const int16_t target_torque)
{
  PrintCommand("ChangeTorque");
  printf("\tTarget torque: %d\n", target_torque);

  GoldSoloWhistleDriveData data(CYCLIC_TORQUE, target_torque);
  SetChange(data);
}

void GoldSoloWhistleDrive::ChangeDeltaTorque(const int16_t delta_torque)
{
  ChangeTorque(input_pdos_.torque_actual_value + delta_torque);
}

void GoldSoloWhistleDrive::ChangeOpMode(const int8_t target_op_mode)
{
  PrintCommand("ChangeOpMode");

  GoldSoloWhistleDriveData data(target_op_mode);
  // Set target value to current one
  switch (target_op_mode)
  {
  case CYCLIC_POSITION:
    data.value = input_pdos_.pos_actual_value;
    printf("\tTarget operational mode: CYCLIC_POSITION\n");
    break;
  case CYCLIC_VELOCITY:
    data.value = input_pdos_.vel_actual_value;
    printf("\tTarget operational mode: CYCLIC_VELOCITY\n");
    break;
  case CYCLIC_TORQUE:
    data.value = input_pdos_.torque_actual_value;
    printf("\tTarget operational mode: CYCLIC_TORQUE\n");
    break;
  default:
    printf("\tTarget operational mode: NO_MODE\n");
    break;
  }
  SetChange(data);
}

void GoldSoloWhistleDrive::SetTargetDefaults()
{
  PrintCommand("SetTargetDefaults");

  // Set target operational mode and value to current ones
  GoldSoloWhistleDriveData data(input_pdos_.display_op_mode);
  switch (data.op_mode)
  {
  case CYCLIC_POSITION:
    data.value = input_pdos_.pos_actual_value;
    printf("\tDefault operational mode: CYCLIC_POSITION @ %d\n", data.value);
    break;
  case CYCLIC_VELOCITY:
    data.value = input_pdos_.vel_actual_value;
    printf("\tDefault operational mode: CYCLIC_VELOCITY @ %d\n", data.value);
    break;
  case CYCLIC_TORQUE:
    data.value = input_pdos_.torque_actual_value;
    printf("\tDefault operational mode: CYCLIC_TORQUE @ %d\n", data.value);
    break;
  default:
    printf("\tDefault operational mode: NO_MODE\n");
    break;
  }
  SetChange(data);
}

void GoldSoloWhistleDrive::SetChange(const GoldSoloWhistleDriveData& data)
{
  // clang-format off
  BEGIN_TRANSITION_MAP                                               // - Current State -
    TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)              // ST_START
    TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)              // ST_NOT_READY_TO_SWITCH_ON
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)               // ST_SWITCH_ON_DISABLED
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)               // ST_READY_TO_SWITCH_ON
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)               // ST_SWITCHED_ON
    TRANSITION_MAP_ENTRY(ST_OPERATION_ENABLED) // ST_OPERATION_ENABLED
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)              // ST_QUICK_STOP_ACTIVE
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)              // ST_FAULT_REACTION_ACTIVE
    TRANSITION_MAP_ENTRY(EVENT_IGNORED)              // ST_FAULT
  END_TRANSITION_MAP(&data)
  // clang-format on
}

////////////////////////////////////////////////////////////////////////////
//// States actions
////////////////////////////////////////////////////////////////////////////

STATE_DEFINE(GoldSoloWhistleDrive, Start, NoEventData)
{
  prev_state_ = ST_START;
  printf("GoldSoloWhistleDrive %u inItial state: %s\n", position_, kStatesStr[ST_START]);
  // This happens automatically on drive's start up. We simply imitate the behavior here.
  InternalEvent(ST_NOT_READY_TO_SWITCH_ON);
}

STATE_DEFINE(GoldSoloWhistleDrive, NotReadyToSwitchOn, NoEventData)
{
  PrintStateTransition(prev_state_, ST_NOT_READY_TO_SWITCH_ON);
  prev_state_ = ST_NOT_READY_TO_SWITCH_ON;
  // This happens automatically on drive's start up. We simply imitate the behavior here.
  InternalEvent(ST_SWITCH_ON_DISABLED);
}

STATE_DEFINE(GoldSoloWhistleDrive, SwitchOnDisabled, NoEventData)
{
  PrintStateTransition(prev_state_, ST_SWITCH_ON_DISABLED);
  prev_state_ = ST_SWITCH_ON_DISABLED;
}

STATE_DEFINE(GoldSoloWhistleDrive, ReadyToSwitchOn, NoEventData)
{
  PrintStateTransition(prev_state_, ST_READY_TO_SWITCH_ON);
  prev_state_ = ST_READY_TO_SWITCH_ON;
}

STATE_DEFINE(GoldSoloWhistleDrive, SwitchedOn, NoEventData)
{
  PrintStateTransition(prev_state_, ST_SWITCHED_ON);
  prev_state_ = ST_SWITCHED_ON;
}

STATE_DEFINE(GoldSoloWhistleDrive, OperationEnabled, GoldSoloWhistleDriveData)
{
  PrintStateTransition(prev_state_, ST_OPERATION_ENABLED);

  // Setup operational mode and target value
  output_pdos_.op_mode = data->op_mode;
  switch (data->op_mode)
  {
  case CYCLIC_POSITION:
    output_pdos_.target_position = data->value;
    break;
  case CYCLIC_VELOCITY:
    output_pdos_.target_velocity = data->value;
    break;
  case CYCLIC_TORQUE:
    output_pdos_.target_torque = static_cast<int16_t>(data->value);
    break;
  default:
    break;
  }

  prev_state_ = ST_OPERATION_ENABLED;
}

STATE_DEFINE(GoldSoloWhistleDrive, QuickStopActive, NoEventData)
{
  PrintStateTransition(prev_state_, ST_QUICK_STOP_ACTIVE);
  prev_state_ = ST_QUICK_STOP_ACTIVE;
}

STATE_DEFINE(GoldSoloWhistleDrive, FaultReactionActive, NoEventData)
{
  PrintStateTransition(prev_state_, ST_FAULT_REACTION_ACTIVE);
  prev_state_ = ST_FAULT_REACTION_ACTIVE;
}

STATE_DEFINE(GoldSoloWhistleDrive, Fault, NoEventData)
{
  PrintStateTransition(prev_state_, ST_FAULT);
  prev_state_ = ST_FAULT;
}

////////////////////////////////////////////////////////////////////////////
//// Miscellaneous
////////////////////////////////////////////////////////////////////////////

inline void GoldSoloWhistleDrive::PrintCommand(const char* cmd) const
{
  printf("GoldSoloWhistleDrive %u received command: %s\n", position_, cmd);
}

void GoldSoloWhistleDrive::PrintStateTransition(
  const GoldSoloWhistleDriveStates current_state,
  const GoldSoloWhistleDriveStates new_state) const
{
  if (current_state == new_state)
    return;
  printf("GoldSoloWhistleDrive %u state transition: %s --> %s\n", position_,
         kStatesStr[current_state], kStatesStr[new_state]);
}

} // end namespace grabec