#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>

#include "instruction.h"
#include "printRoutines.h"

int isValidAddress(uint64_t address, uint64_t max)
{
  return (address >= 0 && address <= max);
}

/* Reads one byte from memory, at the specified address. Stores the
   read value into *value. Returns 1 in case of success, or 0 in case
   of failure (e.g., if the address is beyond the limit of the memory
   size). */
int memReadByte(machine_state_t *state, uint64_t address, uint8_t *value)
{
  uint8_t *memory = state->programMap;
  if (isValidAddress(address, state->programSize))
  {
    *value = memory[address];
    return 1;
  }
  return 0;
}

/* Reads one quad-word (64-bit number) from memory in little-endian
   format, at the specified starting address. Stores the read value
   into *value. Returns 1 in case of success, or 0 in case of failure
   (e.g., if the address is beyond the limit of the memory size). */
int memReadQuadLE(machine_state_t *state, uint64_t address, uint64_t *value)
{
  // uint8_t* memory = state->programMap;
  if (isValidAddress(address, state->programSize))
  {
    *value = 0;
    for (int offset = 7; offset >= 0; offset--) //TODO: To flip or not
    {
      uint8_t nextByte;
      memReadByte(state, address + offset, &nextByte);
      (*value) += (nextByte << (8 * offset));
    }
    // *value = memory[address];
    return 1;
  }
  return 0;
}

/* Stores the specified one-byte value into memory, at the specified
   address. Returns 1 in case of success, or 0 in case of failure
   (e.g., if the address is beyond the limit of the memory size). */
int memWriteByte(machine_state_t *state, uint64_t address, uint8_t value)
{
  uint8_t *memory = state->programMap;
  if (isValidAddress(address, state->programSize))
  {
    memory[address] = value;
    return 1;
  }
  return 0;
}

/* Stores the specified quad-word (64-bit) value into memory, at the
   specified start address, using little-endian format. Returns 1 in
   case of success, or 0 in case of failure (e.g., if the address is
   beyond the limit of the memory size). */
int memWriteQuadLE(machine_state_t *state, uint64_t address, uint64_t value)
{
  uint8_t *memory = state->programMap;
  if (isValidAddress(address, state->programSize))
  {
    for (int offset = 0; offset < 7; offset++)
    {
      memory[address + offset] = ((value >> (8 * offset)) * 0xff);
    }
    return 1;
  }
  return 0;
}

/* Fetches one instruction from memory, at the address specified by
   the program counter. Does not modify the machine's state. The
   resulting instruction is stored in *instr. Returns 1 if the
   instruction is a valid non-halt instruction, or 0 (zero)
   otherwise. */
int fetchInstruction(machine_state_t *state, y86_instruction_t *instr)
{
  uint64_t pc = state->programCounter;
  uint8_t firstByte;
  if (!(memReadByte(state, pc, &firstByte)))
  {
    instr->icode = I_TOO_SHORT;
    return 0;
  }

  // parse the first byte to iCode and iFun
  uint8_t iCode = firstByte & 0xf0;
  iCode = iCode >> 4;

  uint8_t iFun = firstByte & 0xf;

  // set instruction's icode and ifun
  instr->icode = iCode;
  instr->ifun = iFun;

  instr->rA = R_NONE;
  instr->rB = R_NONE;

  instr->location = pc;
  if (iCode >= I_HALT && iCode <= I_POPQ) // checks if icode is valid
  {
    if (iCode == I_IRMOVQ || iCode == I_OPQ || iCode == I_JXX) // ifun error handling
    {
      if (!(iFun >= C_NC && iFun <= C_G)) // checking for conditional ifun cases
      {
        instr->icode = I_INVALID;
        instr->ifun = 0x0;
        return 0;
      }
    }
    else if (iFun != C_NC) // otherwise
    {
      instr->icode = I_INVALID;
      instr->ifun = 0x0;
      return 0;
    }

    if (iCode != I_HALT) // increments valp only if not halt
    {
      instr->valP = pc + 1;
    }

    //Set registers
    if ((iCode >= I_RRMVXX && iCode <= I_OPQ) || iCode >= I_PUSHQ)
    {

      uint8_t secondByte;
      if (!(memReadByte(state, instr->valP, &secondByte)))
      {
        instr->icode = I_TOO_SHORT;
        return 0;
      }

      uint8_t rA = secondByte & 0xf0;
      rA = rA >> 4;
      uint8_t rB = secondByte & 0xf;

      // register value error handling
      if (iCode >= I_RRMVXX && iCode <= I_OPQ) // all icodes that need rB
      {
        if (rB == 0xf) // checks for invalid values in rB
        {
          instr->icode = I_INVALID;
          instr->ifun = 0x0;
          return 0;
        }
        if (iCode == I_IRMOVQ) // checks for R_NONE in rA for irmovq
        {
          if (rA != 0xf)
          {
            instr->icode = I_INVALID;
            instr->ifun = 0x0;
            return 0;
          }
        }
        else if (rA == 0xf) // checks invalid values in rA for all other instructions
        {
          instr->icode = I_INVALID;
          instr->ifun = 0x0;
          return 0;
        }
      }

      instr->rA = rA;
      instr->rB = rB;
      (instr->valP)++;
    }

    //Set ValC
    if ((iCode >= I_IRMOVQ && iCode <= I_MRMOVQ) || iCode == I_JXX || iCode == I_CALL) // get all icodes that have valc
    {
      uint64_t valC;

      // checking memory out of bounds error
      if (!(memReadQuadLE(state, instr->valP, &valC)))
      {
        instr->icode = I_TOO_SHORT;
        return 0;
      }
      if (iCode >= I_JXX)
      {
        if (!isValidAddress(valC, state->programSize))
        {
          instr->icode = I_INVALID;
          return 0;
        }
      }
      if (iCode >= I_RMMOVQ)
      {
        if (!isValidAddress(valC + instr->rB, state->programSize))
        {
          instr->icode = I_INVALID;
          return 0;
        }
      }

      // set instruction's valC
      instr->valC = valC;
      // set instruction's valP
      (instr->valP) += 8;
    }
    return 1;
  }

  // if icode is invalid
  instr->icode = I_INVALID;
  instr->ifun = 0x0;
  return 0;
}

/* Sets the condition codes based on dest (valE). */
uint8_t setCC(uint64_t dest)
{
  uint8_t cc = 0;
  if (dest & 0x80000000)
    cc = 0x2;
  if (dest == 0x0)
    cc++;
  return cc;
}

/* Executes the instruction specified by *instr, modifying the
   machine's state (memory, registers, condition codes, program
   counter) in the process. Returns 1 if the instruction was executed
   successfully, or 0 if there was an error. Typical errors include an
   invalid instruction or a memory access to an invalid address. */
int executeInstruction(machine_state_t *state, y86_instruction_t *instr)
{
  uint8_t iCode = instr->icode;
  uint8_t iFun = instr->ifun;
  uint8_t rA = instr->rA;
  uint8_t rB = instr->rB;
  uint64_t valC = instr->valC;
  uint64_t valP = instr->valP;

  uint8_t *mem = state->programMap;
  uint8_t cc = state->conditionCodes;

  state->programCounter = instr->valP;
  switch (iCode)
  {
  case I_RRMVXX:
    switch (iFun)
    {
    case C_NC:
      state->registerFile[rB] = state->registerFile[rA];
      break;
    case C_LE:
      if ((cc & 0x3) != 0)
        state->registerFile[rB] = state->registerFile[rA];
      break;
    case C_L:
      if ((cc & 0x2) == 2)
        state->registerFile[rB] = state->registerFile[rA];
      break;
    case C_E:
      if ((cc & 0x1) == 1)
        state->registerFile[rB] = state->registerFile[rA];
      break;
    case C_NE:
      if ((cc & 0x1) == 0)
        state->registerFile[rB] = state->registerFile[rA];
      break;
    case C_GE:
      if ((cc & 0x2) == 0)
        state->registerFile[rB] = state->registerFile[rA];
      break;
    case C_G:
      if ((cc & 0x3) == 0)
        state->registerFile[rB] = state->registerFile[rA];
      break;
    }
    break;
  case I_IRMOVQ:
    state->registerFile[rB] = valC;
    break;
  case I_RMMOVQ:
    mem[valC + state->registerFile[rB]] = state->registerFile[rA];
    break;
  case I_MRMOVQ:
    state->registerFile[rA] = mem[valC + state->registerFile[rB]];
    break;
  case I_OPQ:
    switch (iFun)
    {
    case A_ADDQ:
      state->registerFile[rB] = state->registerFile[rB] + state->registerFile[rA];
      break;
    case A_SUBQ:
      state->registerFile[rB] = state->registerFile[rB] - state->registerFile[rA];
      break;
    case A_ANDQ:
      state->registerFile[rB] = state->registerFile[rB] & state->registerFile[rA];
      break;
    case A_XORQ:
      state->registerFile[rB] = state->registerFile[rB] ^ state->registerFile[rA];
      break;
    case A_MULQ:
      state->registerFile[rB] = state->registerFile[rB] * state->registerFile[rA];
      break;
    case A_DIVQ:
      state->registerFile[rB] = state->registerFile[rB] / state->registerFile[rA];
      break;
    case A_MODQ:
      state->registerFile[rB] = state->registerFile[rB] % state->registerFile[rA];
      break;
    }
    cc = setCC(state->registerFile[rB]);
    break;
  case I_JXX:
    switch (iFun)
    {
    case C_NC:
      valP = valC;
      break;
    case C_LE:
      if ((cc & 0x3) != 0)
        valP = valC;
      break;
    case C_L:
      if ((cc & 0x2) == 2)
        valP = valC;
      break;
    case C_E:
      if ((cc & 0x1) == 1)
        valP = valC;
      break;
    case C_NE:
      if ((cc & 0x1) == 0)
        valP = valC;
      break;
    case C_GE:
      if ((cc & 0x2) == 0)
        valP = valC;
      break;
    case C_G:
      if ((cc & 0x3) == 0)
        valP = valC;
      break;
    }
    break;
  case I_CALL:
    mem[state->registerFile[R_RSP] - 8] = valP;
    state->registerFile[R_RSP] -= 8;
    valP = valC;
    break;
  case I_RET:
    valP = mem[state->registerFile[R_RSP]];
    state->registerFile[R_RSP] += 8;
    break;
  case I_PUSHQ:
    mem[state->registerFile[R_RSP] - 8] = state->registerFile[rA];
    state->registerFile[R_RSP] -= 8;
    break;
  case I_POPQ:
    state->registerFile[rA] = mem[state->registerFile[R_RSP]];
    state->registerFile[R_RSP] += 8;
    break;
  case I_INVALID:
    return 0;
  case I_TOO_SHORT:
    return 0;
  default:
    return 0;
  }

  // UPDATE MACHINE STATE
  state->programCounter = valP;
  state->conditionCodes = cc;
  return 1;
}
