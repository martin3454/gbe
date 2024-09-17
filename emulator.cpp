
#include "Emulator.h"
#include "JoyPad.h"
#include "gRom.h"
//#include "Common.h"
#include <cstring>

#define VERTICAL_BLANK_SCAN_LINE 0x90
#define VERTICAL_BLANK_SCAN_LINE_MAX 0x99
#define RETRACE_START 456


JoyPad joypad;


Emulator::Emulator(void) :
	m_GameLoaded(false)
	,m_CyclesThisUpdate(0)
	,m_UsingMBC1(false)
	,m_EnableRamBank(false)
	,m_UsingMemoryModel16_8(true)
	,m_EnableInterupts(false)
	,m_PendingInteruptDisabled(false)
	,m_PendingInteruptEnabled(false)
	,m_RetraceLY(RETRACE_START)
	//,m_JoypadState(0)
	,m_Halted(false)
	,m_TimerVariable(0)
	,m_CurrentClockSpeed(1024)
	,m_DividerVariable(0)
	,m_CurrentRamBank(0)
	,m_DebugPause(false)
	,m_DebugPausePending(false)
	//,m_TimeToPause(NULL)
	,m_TotalOpcodes(0)
	,m_DoLogging(false)
{
	//ResetScreen( );
  m_Rom = new BYTE[0x10000];
  joypad.write_reg(0b00100000); // pak smazat
  ResetCPU();
}


BYTE Emulator::dump_rom(uint16_t index){
      return m_Rom[index];
}

bool Emulator::ResetCPU( )
{
	//ResetScreen( ) ;
	m_DoLogging = false ;
	m_CurrentRamBank = 0 ;
	m_TimerVariable = 0 ;
	m_CurrentClockSpeed = 1024 ;
	m_DividerVariable = 0 ;
	m_Halted = false ;
	m_TotalOpcodes = 0 ;
	//m_JoypadState = 0xFF ;
	m_CyclesThisUpdate = 0 ;
	m_ProgramCounter = 0x100 ;
	m_RegisterAF.hi = 0x1;
	m_RegisterAF.lo = 0xB0 ;
	m_RegisterBC.reg = 0x0013 ;
	m_RegisterDE.reg = 0x00D8 ;
	m_RegisterHL.reg = 0x014D ;
	m_StackPointer.reg = 0xFFFE ;
  //zapis do joypad registru
  joypad.write_reg(0xFF);
	m_Rom[0xFF00] = 0xFF   ; //joypad
	m_Rom[0xFF05] = 0x00   ;
	m_Rom[0xFF06] = 0x00   ;
	m_Rom[0xFF07] = 0x00   ;
	m_Rom[0xFF10] = 0x80   ;
	m_Rom[0xFF11] = 0xBF   ;
	m_Rom[0xFF12] = 0xF3   ;
	m_Rom[0xFF14] = 0xBF   ;
	m_Rom[0xFF16] = 0x3F   ;
	m_Rom[0xFF17] = 0x00   ;
	m_Rom[0xFF19] = 0xBF   ;
	m_Rom[0xFF1A] = 0x7F   ;
	m_Rom[0xFF1B] = 0xFF   ;
	m_Rom[0xFF1C] = 0x9F   ;
	m_Rom[0xFF1E] = 0xBF   ;
	m_Rom[0xFF20] = 0xFF   ;
	m_Rom[0xFF21] = 0x00   ;
	m_Rom[0xFF22] = 0x00   ;
	m_Rom[0xFF23] = 0xBF   ;
	m_Rom[0xFF24] = 0x77   ;
	m_Rom[0xFF25] = 0xF3   ;
	m_Rom[0xFF26] = 0xF1	;
	m_Rom[0xFF40] = 0x91   ;
	m_Rom[0xFF42] = 0x00   ;
	m_Rom[0xFF43] = 0x00   ;
	m_Rom[0xFF45] = 0x00   ;
	m_Rom[0xFF47] = 0xFC   ;
	m_Rom[0xFF48] = 0xFF   ;
	m_Rom[0xFF49] = 0xFF   ;
	m_Rom[0xFF4A] = 0x00   ;
	m_Rom[0xFF4B] = 0x00   ;
	m_Rom[0xFFFF] = 0x00   ;
	m_RetraceLY = RETRACE_START ;
	m_DebugValue = m_Rom[0x40] ;
	m_EnableRamBank = false ;
	m_UsingMBC2 = false;

	// what kinda rom switching are we using, if any?
	switch(ReadMemory(0x147))
	{
		case 0: m_UsingMBC1 = false ; break ; // not using any memory swapping
		case 1:
		case 2:
		case 3 : m_UsingMBC1 = true ; break ;
		case 5 : m_UsingMBC2 = true ; break ;
		case 6 : m_UsingMBC2 = true ; break ;
		default: return false ; // unhandled memory swappping, probably MBC2
	}

	// how many ram banks do we neeed, if any?
	uint8_t numRamBanks = 0 ;
	switch (ReadMemory(0x149))
	{
		case 0: numRamBanks = 0 ;break ;
		case 1: numRamBanks = 1 ;break ;
		case 2: numRamBanks = 1 ;break ;
		case 3: numRamBanks = 4 ;break ;
		case 4: numRamBanks = 16 ;break ;
	}
	CreateRamBanks(numRamBanks) ;
	return true ;
}

void Emulator::gb_input(){
   char znak = keypad.getKey();
    if(znak){
      uint8_t pad;  
        switch(znak){
          case 'B':
          case '4': pad = 0b00000001;
                    break;
          case 'A':
          case '6': pad = 0b00000010;
                    break;      
          case '*':
          case '2': pad = 0b00000100;
                    break;
          case '#':
          case '8': pad = 0b00001000;
                    break;
          default:{
            joypad.button_res();
            return;
          }    
      }
      joypad.button_set(pad);
      Serial.println(joypad.read_reg(), BIN);
    }else joypad.button_res();
}



BYTE Emulator::ExecuteNextOpcode( ){

  uint16_t par;
	BYTE opcode = m_Rom[m_ProgramCounter];
	if ((m_ProgramCounter >= 0x4000 && m_ProgramCounter <= 0x7FFF) || (m_ProgramCounter >= 0xA000 && m_ProgramCounter <= 0xBFFF))
		opcode = ReadMemory(m_ProgramCounter) ;

	if (!m_Halted)
	{
		m_ProgramCounter++;
		m_TotalOpcodes++;
		ExecuteOpcode(opcode); //vratit parametry instrukce...
	}
	else 	m_CyclesThisUpdate += 4;
	// we are trying to disable interupts, however interupts get disabled after the next instruction
	// 0xF3 is the opcode for disabling interupt
	if (m_PendingInteruptDisabled)
	{
		if (ReadMemory(m_ProgramCounter - 1) != 0xF3)
		{
			m_PendingInteruptDisabled = false ;
			m_EnableInterupts = false ;
		}
	}

	if (m_PendingInteruptEnabled)
	{
		if (ReadMemory(m_ProgramCounter - 1) != 0xFB)
		{
			m_PendingInteruptEnabled = false ;
			m_EnableInterupts = true ;
		}
	}
	return opcode ;
}



bool Emulator::LoadRom(){
  m_GameLoaded = true;
  memset(m_Rom, 0x00, sizeof(m_Rom));
  memcpy(m_Rom, m_GameBank, 0x8000);
  m_CurrentRomBank = 1;

	return true ;
}

BYTE Emulator::ReadMemory(WORD memory){
	// reading from rom bank
	if (memory >= 0x4000 && memory <= 0x7FFF){
		uint16_t newAddress = memory ;
		newAddress += ((m_CurrentRomBank - 1) * 0x4000) ;
		return m_GameBank[newAddress] ;
	}
	// reading from RAM Bank
	else if (memory >= 0xA000 && memory <= 0xBFFF){
		WORD newAddress = memory - 0xA000 ;
		return m_RamBank[m_CurrentRamBank][newAddress];
	}
	// trying to read joypad state  DODELAT!!!!!!!!!!!!!!!
	else if (memory == 0xFF00){
    m_Rom[memory] = joypad.read_reg();
    //WriteByte(memory, reg);
  }
	return m_Rom[memory] ;
}

void Emulator::WriteByte(WORD address, BYTE data){
	// writing to memory address 0x0 to 0x1FFF this disables writing to the ram bank. 0 disables, 0xA enables
	if (address <= 0x1FFF){
	    if (m_UsingMBC1){
          if ((data & 0xF) == 0xA)
              m_EnableRamBank = true ;
          else if (data == 0x0)
              m_EnableRamBank = false ;
	    }
	    else if (m_UsingMBC2){                     
        //bit 0 of upper byte must be 0
	     if (false == TestBit(address,8)){
	         if ((data & 0xF) == 0xA)
                m_EnableRamBank = true ;
            else if (data == 0x0)
                m_EnableRamBank = false ;
	     }
	    }
	}
	// if writing to a memory address between 2000 and 3FFF then we need to change rom bank
	else if ( (address >= 0x2000) && (address <= 0x3FFF) ){
		if (m_UsingMBC1){
			if (data == 0x00) data++;

			data &= 31;
			// Turn off the lower 5-bits.
			m_CurrentRomBank &= 224;
			// Combine the written data with the register.
			m_CurrentRomBank |= data;
      //LOG CODE
			/*char buffer[256];
			sprintf(buffer, "Chaning Rom Bank to %d", m_CurrentRomBank) ;
			LogMessage::GetSingleton()->DoLogMessage(buffer, false) ;*/
		}
		else if (m_UsingMBC2){
            data &= 0xF ;
            m_CurrentRomBank = data ;
		}
	}
	// writing to address 0x4000 to 0x5FFF switches ram banks (if enabled of course)
	else if ( (address >= 0x4000) && (address <= 0x5FFF)){
		if (m_UsingMBC1){
			// are we using memory model 16/8
			if (m_UsingMemoryModel16_8){
				// in this mode we can only use Ram Bank 0
				m_CurrentRamBank = 0 ;

				data &= 3;
				data <<= 5;

				if ((m_CurrentRomBank & 31) == 0) data++;
				// Turn off bits 5 and 6, and 7 if it somehow got turned on.
				m_CurrentRomBank &= 31;
				// Combine the written data with the register.
				m_CurrentRomBank |= data;
        //LOG CODE
				/*char buffer[256] ;
				sprintf(buffer, "Chaning Rom Bank to %d", m_CurrentRomBank) ;
				LogMessage::GetSingleton()->DoLogMessage(buffer, false) ;*/

			}
			else{
				m_CurrentRamBank = data & 0x3 ;
        //LOG CODE
				/*char buffer[256] ;
				sprintf(buffer, "=====Chaning Ram Bank to %d=====", m_CurrentRamBank) ;
				LogMessage::GetSingleton()->DoLogMessage(buffer, false) ;*/
			}
		}
	}

	// writing to address 0x6000 to 0x7FFF switches memory model
	else if ( (address >= 0x6000) && (address <= 0x7FFF)){
		if (m_UsingMBC1){
			// we're only interested in the first bit
			data &= 1 ;
			if (data == 1){
				m_CurrentRamBank = 0 ;
				m_UsingMemoryModel16_8 = false ;
			}
			else
				m_UsingMemoryModel16_8 = true ;
		}
	}
	// from now on we're writing to RAM
 	else if ((address >= 0xA000) && (address <= 0xBFFF)){
 		if (m_EnableRamBank){
 		    if (m_UsingMBC1){
                WORD newAddress = address - 0xA000 ;
                //m_RamBank.at(m_CurrentRamBank)[newAddress] = data;
                m_RamBank[m_CurrentRamBank][newAddress] = data;
 		    }
 		}
 		else if (m_UsingMBC2 && (address < 0xA200)){
 		    WORD newAddress = address - 0xA000 ;
            //m_RamBank.at(m_CurrentRamBank)[newAddress] = data;
            m_RamBank[m_CurrentRamBank][newAddress] = data;
 		}

 	}
	// we're right to internal RAM, remember that it needs to echo it
	else if ( (address >= 0xC000) && (address <= 0xDFFF) ){
		m_Rom[address] = data ;
	}
	// echo memory. Writes here and into the internal ram. Same as above
	else if ( (address >= 0xE000) && (address <= 0xFDFF) ){
		m_Rom[address] = data ;
		m_Rom[address - 0x2000] = data ; // echo data into ram address
	}
	// This area is restricted.
 	else if ((address >= 0xFEA0) && (address <= 0xFEFF)){}
  //tady napsat zapis do joypad registru
  else if(address == 0xFF00){
    joypad.write_reg(data);
    m_Rom[0xFF00] = data;
  }
	// reset the divider register
	else if (address == 0xFF04){
		m_Rom[0xFF04] = 0 ;
		m_DividerVariable = 0 ;
	}

	// not sure if this is correct
	else if (address == 0xFF07){
		m_Rom[address] = data;
		uint8_t timerVal = data & 0x03;
		int clockSpeed = 0;
		switch(timerVal){
			case 0: clockSpeed = 1024; break;
			case 1: clockSpeed = 16; break ;
			case 2: clockSpeed = 64 ;break ;
			case 3: clockSpeed = 256 ;break ; // 256
			default: {} break;
		}

		if (clockSpeed != m_CurrentClockSpeed){
			m_TimerVariable = 0 ;
			m_CurrentClockSpeed = clockSpeed ;
		}
	}

	// FF44 shows which horizontal scanline is currently being draw. Writing here resets it
	else if (address == 0xFF44){
		m_Rom[0xFF44] = 0 ;
	}

	else if (address == 0xFF45){
		m_Rom[address] = data ;
	}
	// DMA transfer
	else if (address == 0xFF46)
  {
	    WORD newAddress = (data << 8) ;
		for (int8_t i = 0; i < 0xA0; i++){
			m_Rom[0xFE00 + i] = ReadMemory(newAddress + i);
		}
	}

	// This area is restricted.
 	else if ((address >= 0xFF4C) && (address <= 0xFF7F))
 	{
 	}
	// I guess we're ok to write to memory... gulp
	else
	{
		m_Rom[address] = data ;
	}
}


void Emulator::PushWordOntoStack(WORD word){
  
  BYTE hi = word >> 8 ;
	BYTE lo = word & 0xFF;
	m_StackPointer.reg-- ;
	WriteByte(m_StackPointer.reg, hi) ;
	m_StackPointer.reg-- ;
	WriteByte(m_StackPointer.reg, lo) ;
} 
WORD Emulator::PopWordOffStack(){
  
  WORD word = ReadMemory(m_StackPointer.reg+1) << 8 ;
	word |= ReadMemory(m_StackPointer.reg) ;
	m_StackPointer.reg+=2 ;

	return word ;
}


int Emulator::GetCarryFlag( )
{
	if (TestBit(m_RegisterAF.lo, FLAG_C))
		return 1 ;

	return 0 ;
}

int Emulator::GetZeroFlag( ) 
{
	if (TestBit(m_RegisterAF.lo, FLAG_Z))
		return 1 ;

	return 0 ;
}

int Emulator::GetHalfCarryFlag( ) 
{
	if (TestBit(m_RegisterAF.lo, FLAG_H))
		return 1 ;

	return 0 ;
}

int Emulator::GetSubtractFlag( ) 
{
	if (TestBit(m_RegisterAF.lo, FLAG_N))
		return 1 ;

	return 0 ;
}

static int timerhack = 0;
void Emulator::DoTimers( int cycles ){
	BYTE timerAtts = m_Rom[0xFF07];
	m_DividerVariable += cycles ;

	if (TestBit(timerAtts, 2))
	{
		m_TimerVariable += cycles ;
		// time to increment the timer
		if (m_TimerVariable >= m_CurrentClockSpeed)
		{
			m_TimerVariable = 0 ;
			bool overflow = false ;
			if (m_Rom[0xFF05] == 0xFF)
			{
				overflow = true ;
			}
			m_Rom[0xFF05]++ ;

			if (overflow)
			{
				timerhack++;
				m_Rom[0xFF05] = m_Rom[0xFF06];
				// request the interupt
				RequestInterupt(2) ;
			}
		}
	}

	// do divider register
	if (m_DividerVariable >= 256)
	{
		m_DividerVariable = 0;
		m_Rom[0xFF04]++ ;
	}
}

void Emulator::ServiceInterrupt( int num ){
	PushWordOntoStack(m_ProgramCounter) ;
	m_Halted = false ;

	/*char buffer[200] ;
	sprintf(buffer, "servicing interupt %d", num) ;
	LogMessage::GetSingleton()->DoLogMessage(buffer, false) ;*/

//	unsigned long long limit =(8000000);
//	if (m_TotalOpcodes > limit)
//		LOGMESSAGE(Logging::MSG_INFO, STR::Format("Servicing interrupt %d", num).ConstCharPtr() ) ;
	switch(num)
	{
		case 0: m_ProgramCounter = 0x40 ; break ;// V-Blank
		case 1: m_ProgramCounter = 0x48 ; break ;// LCD-STATE
		case 2: m_ProgramCounter = 0x50 ; break ;// Timer
		case 4: m_ProgramCounter = 0x60 ; break ;// JoyPad
		default: {} break;
	}

	m_EnableInterupts = false ;
	m_Rom[0xFF0F] = BitReset(m_Rom[0xFF0F], num) ;
}

void Emulator::CheckInterupts( ){
	// are interrupts enabled
	if (m_EnableInterupts){
		// has anything requested an interrupt?
		BYTE requestFlag = ReadMemory(0xFF0F);
		if (requestFlag > 0){
			// which requested interrupt has the lowest priority?
			for (int bit = 0; bit < 8; bit++){
				if (TestBit(requestFlag, bit)){					
					BYTE enabledReg = ReadMemory(0xFFFF);
					if (TestBit(enabledReg,bit)){					
						ServiceInterrupt(bit) ;
					}
				}
			}
		}
	}
}

void Emulator::RequestInterupt(uint8_t bit){
	BYTE requestFlag = ReadMemory(0xFF0F) ;
	requestFlag = BitSet(requestFlag,bit) ;
	WriteByte(0xFF0F, requestFlag) ;
}

WORD Emulator::ReadWord(){
  WORD res = ReadMemory(m_ProgramCounter + 1) ;
	res = res << 8 ;
	res |= ReadMemory(m_ProgramCounter) ;
	return res ;
}

WORD Emulator::ReadLSWord( ){
	WORD res = ReadMemory(m_ProgramCounter + 1);
	res = res << 8 ;
	res |= ReadMemory(m_ProgramCounter);
	return res ;
}

void Emulator::CreateRamBanks(uint8_t numBanks){
	// DOES THE FIRST RAM BANK NEED TO BE SET TO THE CONTENTS of m_Rom[0xA000] - m_Rom[0xC000]?
	for (uint8_t i = 0; i < 17; i++){
		BYTE* ram = new BYTE[0x2000];
		memset(ram, 0, sizeof(ram));
		m_RamBank[i] = ram;
	}
  memcpy(m_RamBank[0], &m_GameBank[0xA000], 0x2000);	
}

Emulator::~Emulator(void){
  delete [] m_Rom;
	for(auto& ptr : m_RamBank){
		delete [] ptr;
	}
}

BYTE Emulator::GetLCDMode() const{
	BYTE lcdStatus = m_Rom[0xFF41] ;
	return lcdStatus & 0x3 ;
}

void Emulator::SetLCDStatus( )
{
	BYTE lcdStatus = m_Rom[0xFF41] ;
	if (TestBit(ReadMemory(0xFF40), 7)== false)
	{
		m_RetraceLY = RETRACE_START ;
		m_Rom[0xFF44] = 0 ;

		// mode gets set to 1 when disabled screen.
		lcdStatus &= 252 ;
		lcdStatus = BitSet(lcdStatus,0) ;
		WriteByte(0xFF41,lcdStatus) ;
		return ;
	}

	BYTE lY = ReadMemory(0xFF44) ;
	BYTE currentMode = GetLCDMode( ) ;
	int mode = 0 ;
	bool reqInt = false ;
	// set mode as vertical blank
	if (lY >= VERTICAL_BLANK_SCAN_LINE) //144
	{
		// mode 1
		mode = 1 ;
		lcdStatus = BitSet(lcdStatus,0) ;
		lcdStatus = BitReset(lcdStatus,1) ;
		reqInt = TestBit(lcdStatus, 4) ;
	}
	else
	{
		int mode2Bounds = (RETRACE_START - 80) ;
		int mode3Bounds = (mode2Bounds - 172) ;
		// mode 2
		if (m_RetraceLY >= mode2Bounds)
		{
			mode = 2 ;
			lcdStatus = BitSet(lcdStatus,1) ;
			lcdStatus = BitReset(lcdStatus,0) ;
			reqInt = TestBit(lcdStatus,5) ;
		}
		// mode 3
		else if (m_RetraceLY >= mode3Bounds)
		{
			mode = 3 ;
			lcdStatus = BitSet(lcdStatus,1) ;
			lcdStatus = BitSet(lcdStatus,0) ;
		}
		// mode 3
		else
		{
			mode = 0 ;
			lcdStatus = BitReset(lcdStatus,1) ;
			lcdStatus = BitReset(lcdStatus,0) ;
			reqInt = TestBit(lcdStatus,3) ;
		}
	}

	// just entered a new mode. Request interupt
	if (reqInt && (currentMode != mode))
		RequestInterupt(1) ;

	// check for coincidence flag
	if ( lY == ReadMemory(0xFF45))
	{
		lcdStatus = BitSet(lcdStatus,2) ;

		if (TestBit(lcdStatus,6))
		{
			RequestInterupt(1) ;
		}
	}
	else
	{
		lcdStatus = BitReset(lcdStatus,2) ;
	}

	WriteByte(0xFF41, lcdStatus) ;
}


Emulator::COLOUR Emulator::GetColour(BYTE colourNum, WORD address)
{
	COLOUR res = WHITE ;
	BYTE palette = ReadMemory(address) ;
	int hi = 0 ;;
	int lo = 0 ;

	switch (colourNum)
	{
	case 0: hi = 1 ; lo = 0 ;break ;
	case 1: hi = 3 ; lo = 2 ;break ;
	case 2: hi = 5 ; lo = 4 ;break ;
	case 3: hi = 7 ; lo = 6 ;break ;
	default: break ;
	}

	int colour = 0;
	colour = BitGetVal(palette, hi) << 1;
	colour |= BitGetVal(palette, lo) ;

	switch (colour)
	{
	case 0: res = WHITE ;break ;
	case 1: res = LIGHT_GRAY ;break ;
	case 2: res = DARK_GRAY ;break ;
	case 3: res = BLACK ;break ;
	default:  break ;
	}

	return res ;
}
