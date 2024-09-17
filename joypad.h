#include <Keypad.h>

#define ROWS 4
#define COLS 4

const char kp4x4Keys[ROWS][COLS] 	= {{'1', '2', '3', 'A'}, {'4', '5', '6', 'B'}, {'7', '8', '9', 'C'}, {'*', '0', '#', 'D'}};
byte rowKp4x4Pin [4] = {27, 14, 12, 13};
byte colKp4x4Pin [4] = {32, 33, 25, 26};
Keypad keypad = Keypad(makeKeymap(kp4x4Keys), rowKp4x4Pin, colKp4x4Pin, ROWS, COLS);



class JoyPad{
  private:
    uint8_t but_state;
    uint8_t reg;
    
  public:
    void write_reg(uint8_t data);
    uint8_t read_reg();
    void button_set(uint8_t but);
    void button_res();
    void get_input();
};

void JoyPad::button_set(uint8_t but){
  but_state |= but;
}
void JoyPad::button_res(){
  but_state = 0x00;
}

void JoyPad::write_reg(uint8_t data){
  reg = data & 0x30;
}

uint8_t JoyPad::read_reg(){
  uint8_t pad = but_state ^ 0xff;
  return reg | (pad & 0x0f);
}

/*void JoyPad::get_input(){
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
      //Serial.println(joypad.Read(), BIN);
    }else joypad.button_res();
}*/

