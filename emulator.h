#ifndef EMULATOR_H
#define EMULATOR_H



#include <cstdint>
//#include "JoyPad.h"
//#include "common.h"

#define FLAG_MASK_Z 128
#define FLAG_MASK_N 64
#define FLAG_MASK_H 32
#define FLAG_MASK_C 16
#define FLAG_Z 7
#define FLAG_N 6
#define FLAG_H 5
#define FLAG_C 4

typedef bool (*PauseFunc)();
typedef void (*RenderFunc)();

typedef uint8_t BYTE;
typedef int8_t SIGNED_BYTE;
typedef uint16_t WORD;
typedef int16_t SIGNED_WORD;



class Emulator{
public:
	Emulator(); //done
	~Emulator(); //done
	bool LoadRom(); //done
  BYTE dump_rom(uint16_t index);  
  void gb_input();
	//bool InitGame(RenderFunc func);
  void Update();
  //void StopGame();
  WORD GetRegisterAF()  { return m_RegisterAF.reg; }
  WORD GetRegisterBC()  { return m_RegisterBC.reg; }
  WORD GetRegisterDE()  { return m_RegisterDE.reg; }
  WORD GetRegisterHL()  { return m_RegisterHL.reg; }
  int GetProgramCounter()  { return m_ProgramCounter; }
  WORD GetStackPointer() { return m_StackPointer.reg; }
  bool IsGameLoaded()  { return m_GameLoaded; }
  BYTE ExecuteNextOpcode(); //done plus do executeopcode
  int GetZeroFlag();  //done
  int GetSubtractFlag() ; //done 
  int GetCarryFlag() ; //done  
  int GetHalfCarryFlag() ; //done

  

private:
    //JoyPad joypad;
    RenderFunc m_RenderFunc;
    WORD frame_buffer[144][160];
    
    bool TestBit(uint16_t data, uint8_t pos){  return (data & (1 << pos)) ? true : false; }     
    uint16_t BitSet(uint16_t data, uint8_t pos) { return (data | 1 << pos);}
    uint16_t BitReset(uint16_t data, uint8_t pos){ return (data & ~(1 << pos));}
    int8_t BitGetVal(uint16_t data, uint8_t position){ return ( data & (1 << position)) ? 1 : 0 ;}
	        
    enum COLOUR {
        WHITE,
        LIGHT_GRAY,
        DARK_GRAY,
        BLACK
      };

    BYTE GetLCDMode() const;  //done
    void SetLCDStatus();      //done
    COLOUR GetColour(BYTE colourNumber, WORD address);

    BYTE ReadMemory(WORD memory);     //hotovo
    void WriteByte(WORD address, BYTE data);//hotovo
    void CreateRamBanks(uint8_t numBanks); //done

    void ExecuteOpcode(BYTE opcode); //done
    void ExecuteExtendedOpcode();    //done
    bool ResetCPU(); //done
			  
    union Register {
        WORD reg;
        struct {
            BYTE lo;
            BYTE hi;
        };
    };

    PauseFunc m_TimeToPause;
    unsigned long long m_TotalOpcodes;
    bool m_DoLogging;
	  //BYTE m_JoypadState;
	  bool m_GameLoaded;	
    BYTE* m_Rom;   //BYTE m_Rom[0x10000];
    //BYTE* m_GameBank;  //BYTE m_GameBank[0x10000]; asi ani nepotrebuju
	  BYTE* m_RamBank[17];
    bool m_EnableRamBank;
	
    WORD m_ProgramCounter;
	  Register m_RegisterAF;
    Register m_RegisterBC;
    Register m_RegisterDE;
    Register m_RegisterHL;
    Register m_StackPointer;

    int m_CyclesThisUpdate;
    int m_CurrentRomBank;
    bool m_UsingMemoryModel16_8;
    bool m_EnableInterupts;
    bool m_PendingInteruptDisabled;
    bool m_PendingInteruptEnabled;
    int m_CurrentRamBank;
    int m_RetraceLY;
    bool m_DebugPause;
    bool m_DebugPausePending;

    uint8_t m_DebugValue;
	  bool m_UsingMBC1;
    bool m_UsingMBC2;
    bool m_Halted;
    int m_TimerVariable;
    int m_DividerVariable;
    int m_CurrentClockSpeed;
	
	  void RequestInterupt(uint8_t bit);  //done
    void CheckInterupts(); //rename DoInterrupts
    void ServiceInterrupt(int num); //done
    void DoTimers(int cycles);
	  WORD ReadWord();     //done
    WORD ReadLSWord();  //done
    void PushWordOntoStack(WORD word); //done
    WORD PopWordOffStack();             //done
	

    void CPU_8BIT_LOAD(BYTE& reg);
    void CPU_16BIT_LOAD(WORD& reg);
    void CPU_REG_LOAD(BYTE& reg, BYTE load, int cycles);
    void CPU_REG_LOAD_ROM(BYTE& reg, WORD address);
    void CPU_8BIT_ADD(BYTE& reg, BYTE toAdd, int cycles, bool useImmediate, bool addCarry);
    void CPU_8BIT_SUB(BYTE& reg, BYTE toSubtract, int cycles, bool useImmediate, bool subCarry);
    void CPU_8BIT_AND(BYTE& reg, BYTE toAnd, int cycles, bool useImmediate);
    void CPU_8BIT_OR(BYTE& reg, BYTE toOr, int cycles, bool useImmediate);
    void CPU_8BIT_XOR(BYTE& reg, BYTE toXOr, int cycles, bool useImmediate);
    void CPU_8BIT_COMPARE(BYTE reg, BYTE toSubtract, int cycles, bool useImmediate);
    void CPU_8BIT_INC(BYTE& reg, int cycles);
    void CPU_8BIT_DEC(BYTE& reg, int cycles);
    void CPU_8BIT_MEMORY_INC(WORD address, int cycles);
    void CPU_8BIT_MEMORY_DEC(WORD address, int cycles);
    void CPU_RESTARTS(BYTE n);

    void CPU_16BIT_DEC(WORD& word, int cycles);
    void CPU_16BIT_INC(WORD& word, int cycles);
    void CPU_16BIT_ADD(WORD& reg, WORD toAdd, int cycles);

    void CPU_JUMP(bool useCondition, int flag, bool condition);
    void CPU_JUMP_IMMEDIATE(bool useCondition, int flag, bool condition);
    void CPU_CALL(bool useCondition, int flag, bool condition);
    void CPU_RETURN(bool useCondition, int flag, bool condition);

    void CPU_SWAP_NIBBLES(BYTE& reg);
    void CPU_SWAP_NIB_MEM(WORD address);
    void CPU_SHIFT_LEFT_CARRY(BYTE& reg);
    void CPU_SHIFT_LEFT_CARRY_MEMORY(WORD address);
    void CPU_SHIFT_RIGHT_CARRY(BYTE& reg, bool resetMSB);
    void CPU_SHIFT_RIGHT_CARRY_MEMORY(WORD address, bool resetMSB);

    void CPU_RESET_BIT(BYTE& reg, int bit);
    void CPU_RESET_BIT_MEMORY(WORD address, int bit);
    void CPU_TEST_BIT(BYTE reg, int bit, int cycles);
    void CPU_SET_BIT(BYTE& reg, int bit);
    void CPU_SET_BIT_MEMORY(WORD address, int bit);

    void CPU_DAA();

    void CPU_RLC(BYTE& reg);
    void CPU_RLC_MEMORY(WORD address);
    void CPU_RRC(BYTE& reg);
    void CPU_RRC_MEMORY(WORD address);
    void CPU_RL(BYTE& reg);
    void CPU_RL_MEMORY(WORD address);
    void CPU_RR(BYTE& reg);
    void CPU_RR_MEMORY(WORD address);

    void CPU_SLA(BYTE& reg);
    void CPU_SLA_MEMORY(WORD address);
    void CPU_SRA(BYTE& reg);
    void CPU_SRA_MEMORY(WORD address);
    void CPU_SRL(BYTE& reg);
    void CPU_SRL_MEMORY(WORD address);	
};

#endif




