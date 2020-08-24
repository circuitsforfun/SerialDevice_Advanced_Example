/*
	Multiple IO Device (Using an ESP32)
	Shows how to work with multiple inputs and outputs
	Required Components: Potentiometer, Push Button, Encoder, Photoresitor, LED
*/

#include "WiFi.h"
#include "driver/pcnt.h"
#include <SerialDevicePeripheral.hpp>

#ifndef LED_BUILTIN
#define LED_BUILTIN 22 // If no defined Builtin LED then change this to your LED pin
#endif

#define PHOTORES_PIN	32
#define POT_PIN			34
#define BUTTON1_PIN		35
#define LED_PIN			25

#define PULSES_PER_REVOLUTION 60

#define PULSE_INPUT_PIN_A 27 // Input pin for pulse This time connect phase A of encoder
#define PULSE_INPUT_PIN_B 33 // Control pin This time connect the encoder phase B
#define PCNT_H_LIM_VAL PULSES_PER_REVOLUTION // Counter upper limit Not used this time
#define PCNT_L_LIM_VAL -PULSES_PER_REVOLUTION // lower limit of counter I do not use this time

int16_t count = 0; // number of counts
int16_t revs = 0; // number of revolutions

bool buttonPressed = false;
bool lastButtonState = false;
long lastPressedEvent;

// Pointer for our SerialDevicePeripheral that we will instantiate / initialize later
rw::serial_device::SerialDevicePeripheral* myDevice;

typedef struct {
	int unit;  // the PCNT unit that originated an interrupt
	uint32_t status; // information on the event type that caused the interrupt
} pcnt_evt_t;

static void IRAM_ATTR pcnt_intr_handler(void* arg)
{
	uint32_t intr_status = PCNT.int_st.val;
	int i;
	pcnt_evt_t evt;
	portBASE_TYPE HPTaskAwoken = pdFALSE;

	for (i = 0; i < PCNT_UNIT_MAX; i++) {
		if (intr_status & (BIT(i))) {
			evt.unit = i;
			evt.status = PCNT.status_unit[i].val;
			if (evt.status & PCNT_STATUS_L_LIM_M)
				revs--;
			else if (evt.status & PCNT_STATUS_H_LIM_M)
				revs++;
			PCNT.int_clr.val = BIT(i);
			if (HPTaskAwoken == pdTRUE) {
				portYIELD_FROM_ISR();
			}
		}
	}
}

// Interrupt routine for button press
void buttonChangeState()
{
	buttonPressed = true;
	lastPressedEvent = millis();
}

// the setup function runs once when you press reset or power the board
void setup() {

	// Need to enable WiFi for RNG but we don't need to be connected to anything
	WiFi.mode(WIFI_STA);
	//WiFi.mode(WIFI_OFF);
	WiFi.disconnect();

	// Initialize Serial
	Serial.begin(115200);

	// initialize digital pin LED_BUILTIN as an output.
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(PHOTORES_PIN, INPUT);
	pinMode(POT_PIN, INPUT);
	pinMode(BUTTON1_PIN, INPUT);
	pinMode(LED_PIN, OUTPUT);

	attachInterrupt(digitalPinToInterrupt(BUTTON1_PIN), buttonChangeState, RISING);
	ledcAttachPin(LED_PIN, 1);
	ledcSetup(1, 20000, 8);

	// SerialDevicePeripheral Device Description that we need to fill in for intializing our device
	rw::serial_device::deviceDescriptor desc;

	desc.class_id = 0x0A0A;     // Class ID for our device (can be any 16-bit unsigned interger value that you want)
	desc.type_id = 0x0D0E;      // Type ID for our device (can be any 16-bit unsigned interger value that you want)
	desc.serial = 1010101;      // Unique Serial Number for our device
	desc.version_major = 0;     // Device Version Major
	desc.version_minor = 1;     // Device Version Minor
	desc.version_revision = 1;  // Device Version Revision
	desc.name = "Multiple IO Device";  // Name for our device
	desc.info = "Device For Testing Multiple IO Data";  // Description

	// Instantiate / Initialize our device with the serial device and description we want to use
	myDevice = new rw::serial_device::SerialDevicePeripheral((HardwareSerial*)&Serial, desc);

	// Setup HW PCNT
	pcnt_config_t pcnt_config_A; // Structure declaration for setting (phase A)   
	pcnt_config_A.pulse_gpio_num = PULSE_INPUT_PIN_A;
	pcnt_config_A.ctrl_gpio_num = PULSE_INPUT_PIN_B;
	pcnt_config_A.lctrl_mode = PCNT_MODE_REVERSE;
	pcnt_config_A.hctrl_mode = PCNT_MODE_KEEP;
	pcnt_config_A.channel = PCNT_CHANNEL_0;
	pcnt_config_A.unit = PCNT_UNIT_0;
	pcnt_config_A.pos_mode = PCNT_COUNT_INC;
	pcnt_config_A.neg_mode = PCNT_COUNT_DEC;
	pcnt_config_A.counter_h_lim = PCNT_H_LIM_VAL;
	pcnt_config_A.counter_l_lim = PCNT_L_LIM_VAL;

	pcnt_config_t pcnt_config_B; // Declaration of structure for configuration (phase B)   
	pcnt_config_B.pulse_gpio_num = PULSE_INPUT_PIN_B; // Replace with the one set in config for phase A
	pcnt_config_B.ctrl_gpio_num = PULSE_INPUT_PIN_A;
	pcnt_config_B.lctrl_mode = PCNT_MODE_KEEP; // Replace with the one set in config for phase A
	pcnt_config_B.hctrl_mode = PCNT_MODE_REVERSE;
	pcnt_config_B.channel = PCNT_CHANNEL_1; // Set channel to 1
	pcnt_config_B.unit = PCNT_UNIT_0; // UNIT is the same as phase A
	pcnt_config_B.pos_mode = PCNT_COUNT_INC;
	pcnt_config_B.neg_mode = PCNT_COUNT_DEC;
	pcnt_config_B.counter_h_lim = PCNT_H_LIM_VAL;
	pcnt_config_B.counter_l_lim = PCNT_L_LIM_VAL;

	pcnt_unit_config(&pcnt_config_A); // unit initialization phase A
	pcnt_unit_config(&pcnt_config_B); // unit initialization phase B
	pcnt_counter_pause(PCNT_UNIT_0); // counter pause
	pcnt_counter_clear(PCNT_UNIT_0); // Counter initialization
	// Setup event handlers
	pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_H_LIM);
	pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_L_LIM);
	pcnt_isr_register(pcnt_intr_handler, NULL, 0, NULL);
	pcnt_intr_enable(PCNT_UNIT_0);

	pcnt_counter_resume(PCNT_UNIT_0); // start counting
	pcnt_filter_enable(PCNT_UNIT_0); // Enable Filter
	pcnt_set_filter_value(PCNT_UNIT_0, 50); // Set filter value

}

// the loop function runs over and over again until power down or reset
void loop() {
	static bool sendRng = false;
	static long lastSend = 0;
	int32_t totalCount;

	// Call update to check for new data
	myDevice->update();

	// Process new data if available
	if (myDevice->available()) // Check if there is data waiting in the buffer
	{
		// Check if PWM set command was sent
		// We could add a get command later if we wanted to sync the GUI
		//   with the current settings instead of always overriding them
		if (myDevice->get<std::string>("pwm") == "set")
		{
			uint8_t pwmVal = myDevice->get<uint8_t>("lvl");
			ledcWrite(1, pwmVal);
		}

		// Check if LED set command
		// We could add a get command later if we want to sync the GUI
		if (myDevice->get<std::string>("led") == "set")
		{
			uint8_t ledSta = myDevice->get<uint8_t>("sta");
			if (ledSta == 0)
				digitalWrite(LED_BUILTIN, LOW);
			else if (ledSta == 1)
				digitalWrite(LED_BUILTIN, HIGH);
		}

		if (myDevice->get<std::string>("rnd") == "get")
		{
			sendRng = true;
		}

	}

	// Throttle output for application timing so it's not flooding serial
	if (millis() - lastSend > 33) // 33ms = 30 Hz (Half GUI App 60Hz Refresh) 
	{
		// Clear Data Buffer
		myDevice->clearData();

		// Get Component Values
		uint16_t photoResVal = analogRead(PHOTORES_PIN);
		myDevice->add("pho", photoResVal);

		pcnt_get_counter_value(PCNT_UNIT_0, &count);
		totalCount = ((revs * PULSES_PER_REVOLUTION) + count);
		myDevice->add("enc", totalCount);

		uint16_t potVal = analogRead(POT_PIN);
		myDevice->add("pot", potVal);

		// Interrupt will catch button press
		if (buttonPressed)
		{
			myDevice->add("but", (uint8_t)1);
		}

		// Quick and Dirty debouce trick to catch button release / not pressed
		if (digitalRead(BUTTON1_PIN) == 0)
		{
			int debounce = 0;
			for (int i = 0; i < 7; i++)
				debounce += digitalRead(BUTTON1_PIN);

			if (debounce == 0)
			{
				myDevice->add("but", (uint8_t)0);
				buttonPressed = false;
			}
		}

		// Add RNG Response if user preseed the button in the GUI
		if (sendRng)
		{
			uint32_t rNum = esp_random();
			myDevice->add("rng", rNum);
			sendRng = false;
		}

		// Send Packet
		myDevice->sendPacket();
		lastSend = millis();
	}
}
