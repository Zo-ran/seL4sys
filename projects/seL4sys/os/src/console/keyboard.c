#include "keyboard.h"
#include "vga.h"
#include "../ioport.h"
#include "../utils.h"
 
vka_object_t kbd_ntfn;
vka_object_t buf_full_ntfn;
cspacepath_t kbd_irq;

#define KEYBOARD_INPUT_CONTROL      0x64
#define KEYBOARD_OUTPUT_BUFFER      0x60

/* Press Scan Code */
#define KESC_P 0x01 // escape
#define K1_P 0x02
#define K2_P 0x03
#define K3_P 0x04
#define K4_P 0x05
#define K5_P 0x06
#define K6_P 0x07
#define K7_P 0x08
#define K8_P 0x09
#define K9_P 0x0a
#define K0_P 0x0b
#define KMIN_P 0x0c // - minus
#define KEQU_P 0xd  // = equal
#define KBAC_P 0xe  // backspace
#define KTAB_P 0xf  // tabular
#define KQ_P 0x10
#define KW_P 0x11
#define KE_P 0x12
#define KR_P 0x13
#define KT_P 0x14
#define KY_P 0x15
#define KU_P 0x16
#define KI_P 0x17
#define KO_P 0x18
#define KP_P 0x19
#define KLBR_P 0x1a // [ left bracket
#define KRBR_P 0x1b // ] right bracket
#define KENT_P 0x1c
#define KLCO_P 0x1d // left ctrl
#define KA_P 0x1e
#define KS_P 0x1f
#define KD_P 0x20
#define KF_P 0x21
#define KG_P 0x22
#define KH_P 0x23
#define KJ_P 0x24
#define KK_P 0x25
#define KL_P 0x26
#define KSCO_P 0x27 // ; semicolon
#define KSQU_P 0x28 // ' single quote
#define KBTI_P 0x29 // ` back tick
#define KLSH_P 0x2a // left shift
#define KBSL_P 0x2b // \ back slash
#define KZ_P 0x2c
#define KX_P 0x2d
#define KC_P 0x2e
#define KV_P 0x2f
#define KB_P 0x30
#define KN_P 0x31
#define KM_P 0x32
#define KCOM_P 0x33 // , comma
#define KDOT_P 0x34 // . full stop
#define KFSL_P 0x35 // / forward slash
#define KRSH_P 0x36 // right shift
#define KKPS_P 0x37 // keypad *
#define KLAL_P 0x38 // left alt
#define KSPA_P 0x39 // space
#define KCAP_P 0x3a // capslock
#define KF1_P 0x3b
#define KF2_P 0x3c
#define KF3_P 0x3d
#define KF4_P 0x3e
#define KF5_P 0x3f
#define KF6_P 0x40
#define KF7_P 0x41
#define KF8_P 0x42
#define KF9_P 0x43
#define KF10_P 0x44
#define KNLO_P 0x45 // numberlock
#define KSLO_P 0x46 // scrolllock
#define KKP7_P 0x47 // keypad 7
#define KKP8_P 0x48 // keypad 8
#define KKP9_P 0x49 // keypad 9
#define KKPM_P 0x4a // keypad -
#define KKP4_P 0x4b // keypad 4
#define KKP5_P 0x4c // keypad 5
#define KKP6_P 0x4d // keypad 6
#define KKPP_P 0x4e // keypad +
#define KKP1_P 0x4f // keypad 1
#define KKP2_P 0x50 // keypad 2
#define KKP3_P 0x51 // keypad 3
#define KKP0_P 0x52 // keypad 0
#define KKPD_P 0x53 // keypad .
#define KF11_P 0x57
#define KF12_P 0x58

#define KEYTABLE_SIZE (KF12_P + 1)

char key_buffer[MAX_KEYBUFFER_SIZE];
int buffer_head;
int buffer_tail;

#define BUFFER_FULL (((buffer_tail + 1) % MAX_KEYBUFFER_SIZE) == buffer_head)
#define BUFFER_EMPTY (buffer_head == buffer_tail)

static char keyTableL[KEYTABLE_SIZE];
static char keyTableU[KEYTABLE_SIZE];
static char keyTable[4][KEYTABLE_SIZE];
static int keyboard_state;

uint8_t lshift;
uint8_t rshift;
uint8_t caps;

#define NCAPS_NSH 0b00
#define NCAPS_SH 0b01
#define CAPS_NSH 0b10
#define CAPS_SH 0b11

static inline char code_to_char(uint8_t code) {
	if (code >= KF12_P)
		return 0;
	else
		return keyTable[(caps & 0b10) + (lshift | rshift)][code];
}

static inline void update_kbd_state(uint8_t code) {
    if (code <= (KF12_P + 0x80)) {
		switch(code) {
			case KLSH_P: lshift = 1;			break;
			case (KLSH_P + 0x80): lshift = 0; 	break;
			case KRSH_P: rshift = 1; 			break;
			case (KRSH_P + 0x80): rshift = 0; 	break;
			case KCAP_P: // press capslock
				switch (caps) {
					case 0: caps = 2; break;
					case 3: caps = 1; break;
				}
				break;
			case (KCAP_P + 0x80): // release capslock
				switch (caps) {
					case 2: caps = 3; break;
					case 1: caps = 0; break;
				}
			default:
				break;
		}
	}
}

void ps2_init() {
    // get old internel config
    uint8_t config;
    outByte(KEYBOARD_INPUT_CONTROL, 0x20);
    while ((inByte(KEYBOARD_INPUT_CONTROL) & 0x1) == 0);
    config = inByte(KEYBOARD_OUTPUT_BUFFER);
    
    // generate interrupts (irq number 1)
    config &= 0xBC;
    config |= 0x1;
    outByte(KEYBOARD_INPUT_CONTROL, 0x60);
    outByte(KEYBOARD_OUTPUT_BUFFER, config);

    // enable port
    outByte(KEYBOARD_INPUT_CONTROL, 0xAE);

    // switch to mode 1 scancode
    outByte(KEYBOARD_OUTPUT_BUFFER, 0xF0); 
    outByte(KEYBOARD_OUTPUT_BUFFER, 1); 
}

void keytable_init() {
	buffer_head = 0;
	buffer_tail = 0;
	memset(key_buffer, 0, sizeof(key_buffer));
	lshift = 0;
	rshift = 0;
	caps = 0;

	keyTable[NCAPS_NSH][K1_P]='1';    keyTable[NCAPS_SH][K1_P]='!';		keyTable[CAPS_NSH][K1_P]='1';	  keyTable[CAPS_SH][K1_P]='!';		
	keyTable[NCAPS_NSH][K2_P]='2';    keyTable[NCAPS_SH][K2_P]='@';		keyTable[CAPS_NSH][K2_P]='2';	  keyTable[CAPS_SH][K2_P]='@';		
	keyTable[NCAPS_NSH][K3_P]='3';    keyTable[NCAPS_SH][K3_P]='#';		keyTable[CAPS_NSH][K3_P]='3';	  keyTable[CAPS_SH][K3_P]='#';		
	keyTable[NCAPS_NSH][K4_P]='4';    keyTable[NCAPS_SH][K4_P]='$';		keyTable[CAPS_NSH][K4_P]='4';	  keyTable[CAPS_SH][K4_P]='$';		
	keyTable[NCAPS_NSH][K5_P]='5';    keyTable[NCAPS_SH][K5_P]='%';		keyTable[CAPS_NSH][K5_P]='5';	  keyTable[CAPS_SH][K5_P]='%';		
	keyTable[NCAPS_NSH][K6_P]='6';    keyTable[NCAPS_SH][K6_P]='^';		keyTable[CAPS_NSH][K6_P]='6';	  keyTable[CAPS_SH][K6_P]='^';		
	keyTable[NCAPS_NSH][K7_P]='7';    keyTable[NCAPS_SH][K7_P]='&';		keyTable[CAPS_NSH][K7_P]='7';	  keyTable[CAPS_SH][K7_P]='&';		
	keyTable[NCAPS_NSH][K8_P]='8';    keyTable[NCAPS_SH][K8_P]='*';		keyTable[CAPS_NSH][K8_P]='8';	  keyTable[CAPS_SH][K8_P]='*';		
	keyTable[NCAPS_NSH][K9_P]='9';    keyTable[NCAPS_SH][K9_P]='(';		keyTable[CAPS_NSH][K9_P]='9';	  keyTable[CAPS_SH][K9_P]='(';		
	keyTable[NCAPS_NSH][K0_P]='0';    keyTable[NCAPS_SH][K0_P]=')';		keyTable[CAPS_NSH][K0_P]='0';	  keyTable[CAPS_SH][K0_P]=')';		
	keyTable[NCAPS_NSH][KA_P]='a';    keyTable[NCAPS_SH][KA_P]='A';		keyTable[CAPS_NSH][KA_P]='A';	  keyTable[CAPS_SH][KA_P]='a';		
	keyTable[NCAPS_NSH][KB_P]='b';    keyTable[NCAPS_SH][KB_P]='B';		keyTable[CAPS_NSH][KB_P]='B';	  keyTable[CAPS_SH][KB_P]='b';		
	keyTable[NCAPS_NSH][KC_P]='c';    keyTable[NCAPS_SH][KC_P]='C';		keyTable[CAPS_NSH][KC_P]='C';	  keyTable[CAPS_SH][KC_P]='c';		
	keyTable[NCAPS_NSH][KD_P]='d';    keyTable[NCAPS_SH][KD_P]='D';		keyTable[CAPS_NSH][KD_P]='D';	  keyTable[CAPS_SH][KD_P]='d';		
	keyTable[NCAPS_NSH][KE_P]='e';    keyTable[NCAPS_SH][KE_P]='E';		keyTable[CAPS_NSH][KE_P]='E';	  keyTable[CAPS_SH][KE_P]='e';		
	keyTable[NCAPS_NSH][KF_P]='f';    keyTable[NCAPS_SH][KF_P]='F';		keyTable[CAPS_NSH][KF_P]='F';	  keyTable[CAPS_SH][KF_P]='f';		
	keyTable[NCAPS_NSH][KG_P]='g';    keyTable[NCAPS_SH][KG_P]='G';		keyTable[CAPS_NSH][KG_P]='G';	  keyTable[CAPS_SH][KG_P]='g';		
	keyTable[NCAPS_NSH][KH_P]='h';    keyTable[NCAPS_SH][KH_P]='H';		keyTable[CAPS_NSH][KH_P]='H';	  keyTable[CAPS_SH][KH_P]='h';		
	keyTable[NCAPS_NSH][KI_P]='i';    keyTable[NCAPS_SH][KI_P]='I';		keyTable[CAPS_NSH][KI_P]='I';	  keyTable[CAPS_SH][KI_P]='i';		
	keyTable[NCAPS_NSH][KJ_P]='j';    keyTable[NCAPS_SH][KJ_P]='J';		keyTable[CAPS_NSH][KJ_P]='J';	  keyTable[CAPS_SH][KJ_P]='j';		
	keyTable[NCAPS_NSH][KK_P]='k';    keyTable[NCAPS_SH][KK_P]='K';		keyTable[CAPS_NSH][KK_P]='K';	  keyTable[CAPS_SH][KK_P]='k';		
	keyTable[NCAPS_NSH][KL_P]='l';    keyTable[NCAPS_SH][KL_P]='L';		keyTable[CAPS_NSH][KL_P]='L';	  keyTable[CAPS_SH][KL_P]='l';		
	keyTable[NCAPS_NSH][KM_P]='m';    keyTable[NCAPS_SH][KM_P]='M';		keyTable[CAPS_NSH][KM_P]='M';	  keyTable[CAPS_SH][KM_P]='m';		
	keyTable[NCAPS_NSH][KN_P]='n';    keyTable[NCAPS_SH][KN_P]='N';		keyTable[CAPS_NSH][KN_P]='N';	  keyTable[CAPS_SH][KN_P]='n';		
	keyTable[NCAPS_NSH][KO_P]='o';    keyTable[NCAPS_SH][KO_P]='O';		keyTable[CAPS_NSH][KO_P]='O';	  keyTable[CAPS_SH][KO_P]='o';		
	keyTable[NCAPS_NSH][KP_P]='p';    keyTable[NCAPS_SH][KP_P]='P';		keyTable[CAPS_NSH][KP_P]='P';	  keyTable[CAPS_SH][KP_P]='p';		
	keyTable[NCAPS_NSH][KQ_P]='q';    keyTable[NCAPS_SH][KQ_P]='Q';		keyTable[CAPS_NSH][KQ_P]='Q';	  keyTable[CAPS_SH][KQ_P]='q';		
	keyTable[NCAPS_NSH][KR_P]='r';    keyTable[NCAPS_SH][KR_P]='R';		keyTable[CAPS_NSH][KR_P]='R';	  keyTable[CAPS_SH][KR_P]='r';		
	keyTable[NCAPS_NSH][KS_P]='s';    keyTable[NCAPS_SH][KS_P]='S';		keyTable[CAPS_NSH][KS_P]='S';	  keyTable[CAPS_SH][KS_P]='s';		
	keyTable[NCAPS_NSH][KT_P]='t';    keyTable[NCAPS_SH][KT_P]='T';		keyTable[CAPS_NSH][KT_P]='T';	  keyTable[CAPS_SH][KT_P]='t';		
	keyTable[NCAPS_NSH][KU_P]='u';    keyTable[NCAPS_SH][KU_P]='U';		keyTable[CAPS_NSH][KU_P]='U';	  keyTable[CAPS_SH][KU_P]='u';		
	keyTable[NCAPS_NSH][KV_P]='v';    keyTable[NCAPS_SH][KV_P]='V';		keyTable[CAPS_NSH][KV_P]='V';	  keyTable[CAPS_SH][KV_P]='v';		
	keyTable[NCAPS_NSH][KW_P]='w';    keyTable[NCAPS_SH][KW_P]='W';		keyTable[CAPS_NSH][KW_P]='W';	  keyTable[CAPS_SH][KW_P]='w';		
	keyTable[NCAPS_NSH][KX_P]='x';    keyTable[NCAPS_SH][KX_P]='X';		keyTable[CAPS_NSH][KX_P]='X';	  keyTable[CAPS_SH][KX_P]='x';		
	keyTable[NCAPS_NSH][KY_P]='y';    keyTable[NCAPS_SH][KY_P]='Y';		keyTable[CAPS_NSH][KY_P]='Y';	  keyTable[CAPS_SH][KY_P]='y';		
	keyTable[NCAPS_NSH][KZ_P]='z';    keyTable[NCAPS_SH][KZ_P]='Z';		keyTable[CAPS_NSH][KZ_P]='Z';	  keyTable[CAPS_SH][KZ_P]='z';		
	keyTable[NCAPS_NSH][KMIN_P]='-';  keyTable[NCAPS_SH][KMIN_P]='_';	keyTable[CAPS_NSH][KMIN_P]='-';   keyTable[CAPS_SH][KMIN_P]='_';
	keyTable[NCAPS_NSH][KEQU_P]='=';  keyTable[NCAPS_SH][KEQU_P]='+';	keyTable[CAPS_NSH][KEQU_P]='=';   keyTable[CAPS_SH][KEQU_P]='+';
	keyTable[NCAPS_NSH][KLBR_P]='[';  keyTable[NCAPS_SH][KLBR_P]='{';   keyTable[CAPS_NSH][KLBR_P]='[';   keyTable[CAPS_SH][KLBR_P]='{';
	keyTable[NCAPS_NSH][KRBR_P]=']';  keyTable[NCAPS_SH][KRBR_P]='}';   keyTable[CAPS_NSH][KRBR_P]=']';   keyTable[CAPS_SH][KRBR_P]='}';
	keyTable[NCAPS_NSH][KENT_P]='\n'; keyTable[NCAPS_SH][KENT_P]='\n';  keyTable[CAPS_NSH][KENT_P]='\n';  keyTable[CAPS_SH][KENT_P]='\n';
	keyTable[NCAPS_NSH][KSCO_P]=';';  keyTable[NCAPS_SH][KSCO_P]=':';   keyTable[CAPS_NSH][KSCO_P]=';';   keyTable[CAPS_SH][KSCO_P]=':';
	keyTable[NCAPS_NSH][KSQU_P]='\''; keyTable[NCAPS_SH][KSQU_P]='\"';  keyTable[CAPS_NSH][KSQU_P]='\'';  keyTable[CAPS_SH][KSQU_P]='\"';
	keyTable[NCAPS_NSH][KBTI_P]='`';  keyTable[NCAPS_SH][KBTI_P]='~';   keyTable[CAPS_NSH][KBTI_P]='`';   keyTable[CAPS_SH][KBTI_P]='~';
	keyTable[NCAPS_NSH][KBSL_P]='\\'; keyTable[NCAPS_SH][KBSL_P]='|';   keyTable[CAPS_NSH][KBSL_P]='\\';  keyTable[CAPS_SH][KBSL_P]='|';
	keyTable[NCAPS_NSH][KCOM_P]=',';  keyTable[NCAPS_SH][KCOM_P]='<';   keyTable[CAPS_NSH][KCOM_P]=',';   keyTable[CAPS_SH][KCOM_P]='<';
	keyTable[NCAPS_NSH][KDOT_P]='.';  keyTable[NCAPS_SH][KDOT_P]='>';   keyTable[CAPS_NSH][KDOT_P]='.';   keyTable[CAPS_SH][KDOT_P]='>';
	keyTable[NCAPS_NSH][KFSL_P]='/';  keyTable[NCAPS_SH][KFSL_P]='?';   keyTable[CAPS_NSH][KFSL_P]='/';   keyTable[CAPS_SH][KFSL_P]='?';
	keyTable[NCAPS_NSH][KSPA_P]=' ';  keyTable[NCAPS_SH][KSPA_P]=' ';   keyTable[CAPS_NSH][KSPA_P]=' ';   keyTable[CAPS_SH][KSPA_P]=' ';
}

void kbd_irq_handle_mainloop() {
    while (1) {
        seL4_Wait(kbd_ntfn.cptr, NULL);
        seL4_IRQHandler_Ack(kbd_irq.capPtr);
        
        uint8_t code = inByte(0x60);
        update_kbd_state(code);
		if (code == 0xe) {
			vga_delchar();
			if (!BUFFER_EMPTY) {
				buffer_tail -= 1;
				if (buffer_tail < 0)
					buffer_tail += MAX_KEYBUFFER_SIZE;
			}
		} else {
			char ch = code_to_char(code);
			if (ch != 0) {
				if (!BUFFER_FULL) {
					key_buffer[buffer_tail++] = ch;
					buffer_tail %= MAX_KEYBUFFER_SIZE;
					if (ch == '\n')
						seL4_Signal(buf_full_ntfn.cptr);
				} else {
					seL4_Signal(buf_full_ntfn.cptr);
				}
				vga_putchar(ch, false);
			}
		}
		update_cursor();
    }
}

void kbd_init(vka_t *vka, simple_t *simple) {
    seL4_Error error = vka_cspace_alloc_path(vka, &kbd_irq);
    error = vka_alloc_notification(vka, &kbd_ntfn);
	error = vka_alloc_notification(vka, &buf_full_ntfn);

    error = simple_get_IRQ_handler(simple, 0x1, kbd_irq);
    error = seL4_IRQHandler_SetNotification(kbd_irq.capPtr, kbd_ntfn.cptr);

    ZF_LOGF_IFERR(error != seL4_NoError, "Failed to init keyboard!");
 
    ps2_init();
    keytable_init();
}