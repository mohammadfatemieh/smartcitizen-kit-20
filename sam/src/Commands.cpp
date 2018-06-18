#include "Commands.h"
#include "SckBase.h"


void AllCommands::in(SckBase* base, String strIn)
{

	if (strIn.length() <= 0) return;

	CommandType reqComm = COM_COUNT;

	// Search in the command list
	for (uint8_t i=0; i < COM_COUNT; ++i) {

		CommandType thisType = static_cast<CommandType>(i);

		OneCom *thisCommand = &com_list[thisType];

		if (strIn.startsWith(thisCommand->title)) {
			reqComm = thisType;
			strIn.replace(thisCommand->title, "");
			strIn.trim();
			break;
		}
	}

	if (reqComm < COM_COUNT) com_list[reqComm].function(base, strIn);
	else base->sckOut("Unrecognized command!!");
}

void reset_com(SckBase* base, String parameters)
{

	base->reset();
}
void getVersion_com(SckBase* base, String parameters)
{
	base->sckOut("Hardware Version: ");
	base->sckOut("SAM Version: ");
	base->sckOut("ESP version: ");

	base->getUniqueID();
	sprintf(base->outBuff, "Hardware ID: %lx-%lx-%lx-%lx", base->uniqueID[0], base->uniqueID[1], base->uniqueID[2], base->uniqueID[3]);
	base->sckOut();
}
void resetCause_com(SckBase* base, String parameters)
{

	uint8_t resetCause = PM->RCAUSE.reg;

	switch(resetCause){
		case 1:
			base->sckOut("POR: Power On Reset");
			break;
		case 2:
			base->sckOut("BOD12: Brown Out 12 Detector Reset");
			break;
		case 4:
			base->sckOut("BOD33: Brown Out 33 Detector Reset");
			break;
		case 16:
			base->sckOut("EXT: External Reset");
			break;
		case 32:
			base->sckOut("WDT: Watchdog Reset");
			break;
		case 64:
			base->sckOut("SYST: System Reset Request");
			break;
	}
}
void outlevel_com(SckBase* base, String parameters)
{

	// get
	if (parameters.length() <= 0) {

		sprintf(base->outBuff, "Current output level: %s", base->outLevelTitles[base->outputLevel]);
		base->sckOut();

		// set
	} else {

		uint8_t newLevel = parameters.toInt();

		// Parameter sanity check
		if (newLevel >= 0 && newLevel < OUT_COUNT) {
			OutLevels thisLevel = static_cast<OutLevels>(newLevel);
			base->outputLevel = thisLevel;
			sprintf(base->outBuff, "New output level: %s", base->outLevelTitles[newLevel]);
			base->sckOut();
		} else {
			base->sckOut("unrecognized output level!!");
			return;
		}
	}
}
void help_com(SckBase* base, String parameters)
{

	// TODO manage multiline help. Maybe a simple general help and a per command help: "help config"
	base->sckOut();
	for (uint8_t i=0; i < COM_COUNT; ++i) {

		CommandType thisType = static_cast<CommandType>(i);
		OneCom *thisCommand = &base->commands[thisType];

		String sep = "            ";
		sep.remove(sep.length() -String(thisCommand->title).length());

		sprintf(base->outBuff, "%s:%s%s", thisCommand->title, sep.c_str(), thisCommand->help);
		base->sckOut();
	}
	base->sckOut();
}
void pinmux_com(SckBase* base, String parameters)
{

	for (uint8_t pin=0; pin<PINS_COUNT; pin++) {  // For all defined pins
		pinmux_report(pin, base->outBuff, 0);
		base->sckOut();
	}
}
void sensorConfig_com(SckBase* base, String parameters)
{
	// Get
	if (parameters.length() <= 0) {

		SensorType thisType = SENSOR_COUNT;

		sprintf(base->outBuff, "\r\nDisabled\r\n----------");
		base->sckOut();
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			thisType = static_cast<SensorType>(i);

			if (!base->sensors[thisType].enabled) base->sckOut(base->sensors[thisType].title);
		}

		sprintf(base->outBuff, "\r\nEnabled\r\n----------");
		base->sckOut();
		// Get sensor type
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			thisType = static_cast<SensorType>(i);

			if (base->sensors[thisType].enabled) base->sckOut(String(base->sensors[thisType].title) + " (" + String(base->sensors[thisType].interval) + " sec)");
		}

	} else {
		Configuration thisConfig = base->getConfig();

		uint16_t sensorIndex = parameters.indexOf(" ", parameters.indexOf("-"));
		SensorType sensorToChange = base->sensors.getTypeFromString(parameters.substring(sensorIndex));

		if (parameters.indexOf("-enable") >=0) {
			thisConfig.sensors[sensorToChange].enabled = true;
		} else if (parameters.indexOf("-disable") >=0) {
			thisConfig.sensors[sensorToChange].enabled = false;
		} else if (parameters.indexOf("-interval") >=0) {
			sensorIndex = parameters.indexOf(" ", parameters.indexOf("-interval"));
			uint16_t intervalIndex = parameters.indexOf(" ", sensorIndex+1);
			String strInterval = parameters.substring(intervalIndex);
			uint32_t intervalInt = strInterval.toInt();
			if (intervalInt > minimal_sensor_reading_interval && intervalInt < max_sensor_reading_interval)	thisConfig.sensors[sensorToChange].interval = strInterval.toInt();
		}

		base->saveConfig(thisConfig);
	}
}
void readSensor_com(SckBase* base, String parameters)
{

	SensorType sensorToRead = base->sensors.getTypeFromString(parameters);

	if (base->getReading(sensorToRead, true)) sprintf(base->outBuff, "%s: %s %s", base->sensors[sensorToRead].title, base->sensors[sensorToRead].reading.c_str(), base->sensors[sensorToRead].unit);
	else sprintf(base->outBuff, "ERROR reading %s sensor!!!", base->sensors[sensorToRead].title);
	base->sckOut();
}
void controlSensor_com(SckBase* base, String parameters)
{
  	SensorType wichSensor = base->sensors.getTypeFromString(parameters);
	parameters.remove(0, parameters.indexOf(" ")+1);

	if (parameters.length() < 1) {
		base->sckOut("No command received!! please try again...");
		return;
	} else {
		base->controlSensor(wichSensor, parameters);
	}
}
void monitorSensor_com(SckBase* base, String parameters)
{


	SensorType sensorsToMonitor[SENSOR_COUNT];
	uint8_t index = 0;

	if (parameters.length() > 0) {
		while (parameters.length() > 0) {
			uint8_t sep = parameters.indexOf(",");
			if (sep == 0) sep = parameters.length();
			String thisSensor = parameters.substring(0, sep);
			parameters.remove(0, sep+1);
			SensorType thisSensorType = base->sensors.getTypeFromString(thisSensor);
			if (base->sensors[thisSensorType].enabled) {
				sensorsToMonitor[index] = thisSensorType;
				index ++;
			}
		}
	} else {
		for (uint8_t i=0; i<SENSOR_COUNT; i++) {
			if (base->sensors[static_cast<SensorType>(i)].enabled) {
				sensorsToMonitor[index] = static_cast<SensorType>(i);
				index++;
			}
		}

	}

	// Titles
	sprintf(base->outBuff, "%s", base->sensors[sensorsToMonitor[0]].title);
	for (uint8_t i=1; i<index; i++) {
		sprintf(base->outBuff, "%s, %s", base->outBuff, base->sensors[sensorsToMonitor[i]].title);
	}
	base->sckOut();

	// Readings
	strncpy(base->outBuff, "", 240);
	while (!SerialUSB.available()) {
		if (base->getReading(sensorsToMonitor[0], true)) sprintf(base->outBuff, "%s", base->sensors[sensorsToMonitor[0]].reading.c_str());
		for (uint8_t i=1; i<index; i++) {
			if (base->getReading(sensorsToMonitor[i], true)) sprintf(base->outBuff, "%s, %s", base->outBuff, base->sensors[sensorsToMonitor[i]].reading.c_str());
		}
		base->sckOut();
	}
}
void publish_com(SckBase* base, String parameters)
{
	base->publish();
}
extern "C" char *sbrk(int i);
void freeRAM_com(SckBase* base, String parameters)
{

	char stack_dummy = 0;
	uint32_t free = &stack_dummy - sbrk(0);
	sprintf(base->outBuff, "Free RAM: %lu bytes", free);
	base->sckOut();
}
void battReport_com(SckBase* base, String parameters)
{

	base->batteryReport();
}
void i2cDetect_com(SckBase* base, String parameters)
{

	for (uint8_t wichWire=0; wichWire<2; wichWire++) {

		if (wichWire == 0) base->sckOut("Searching for devices on internal I2C bus...");
		else base->sckOut("\r\nSearching for devices on auxiliary I2C bus...");

		uint8_t nDevices = 0;
		for(uint8_t address = 1; address < 127; address++ ) {

			uint8_t error;
			if (wichWire == 0) {
				Wire.beginTransmission(address);
				error = Wire.endTransmission();
			} else {
				auxWire.beginTransmission(address);
				error = auxWire.endTransmission();
			}

			if (error == 0) {
				sprintf(base->outBuff, "I2C device found at address 0x%02x!!", address);
				base->sckOut();
				nDevices++;
			} else if (error==4) {
				sprintf(base->outBuff, "Unknow error at address 0x%02x!!", address);
				base->sckOut();
			}
		}
		sprintf(base->outBuff, "Found %u devices", nDevices);
		base->sckOut();
	}
}
void getCharger_com(SckBase* base, String parameters)
{

	// sprintf(base->outBuff, "%u", base->charger.chargeTimer(parameters.toInt()));
	// base->sckOut();

	if (parameters.endsWith("otg")) base->charger.OTG(0);

	sprintf(base->outBuff, "Battery: %s", base->charger.chargeStatusTitles[base->charger.getChargeStatus()]);
	base->sckOut();

	sprintf(base->outBuff, "USB: %s", base->charger.VBUSStatusTitles[base->charger.getVBUSstatus()]);
	base->sckOut();

	sprintf(base->outBuff, "OTG: %s", base->charger.enTitles[base->charger.OTG()]);
	base->sckOut();

	sprintf(base->outBuff, "Charge: %s", base->charger.enTitles[base->charger.chargeState()]);
	base->sckOut();

	sprintf(base->outBuff, "Charger current limit: %u mA", base->charger.chargerCurrentLimit());
	base->sckOut();

	sprintf(base->outBuff, "Input current limit: %u mA", base->charger.inputCurrentLimit());
	base->sckOut();

	sprintf(base->outBuff, "I2c watchdog timer: %u sec (0: disabled)", base->charger.I2Cwatchdog());
	base->sckOut();

	sprintf(base->outBuff, "Charging safety timer: %u hours (0: disabled)", base->charger.chargeTimer());
	base->sckOut();
}
void config_com(SckBase* base, String parameters)
{

	// Set
	if (parameters.length() > 0) {

		if (parameters.indexOf("-defaults") >= 0) {
			base->saveConfig(true);
		} else {
			Configuration thisConfig;
			thisConfig = base->getConfig();

			// Shows or sets configuration [-defaults -mode sdcard/network -pubint publish-interval -wifi \"ssid/null\" [\"pass\"] -token token/null]
			uint16_t modeI = parameters.indexOf("-mode");
			if (modeI >= 0) {
				String modeC = parameters.substring(modeI+6);
				modeC.toLowerCase();
				if (modeC.startsWith("sd")) thisConfig.mode = MODE_SD;
				else if (modeC.startsWith("net")) thisConfig.mode = MODE_NET;
			}
			uint16_t pubIntI = parameters.indexOf("-pubint");
			if (pubIntI >= 0) {
				String pubIntC = parameters.substring(pubIntI+8);
				uint32_t pubIntV = pubIntC.toInt();
				if (pubIntV > minimal_publish_interval && pubIntV < max_publish_interval) thisConfig.publishInterval = pubIntV;
			}
			uint16_t credI = parameters.indexOf("-wifi");
			if (credI >= 0) {
				uint8_t first = parameters.indexOf("\"", credI+6);
				uint8_t second = parameters.indexOf("\"", first + 1);
				uint8_t third = parameters.indexOf("\"", second + 1);
				uint8_t fourth = parameters.indexOf("\"", third + 1);
				if (parameters.substring(first + 1, second).length() > 0) {
					parameters.substring(first + 1, second).toCharArray(thisConfig.credentials.ssid, 64);
					thisConfig.credentials.set = true;
				}
				if (parameters.substring(third + 1, fourth).length() > 0) {
					parameters.substring(third + 1, fourth).toCharArray(thisConfig.credentials.pass, 64);
				}
			}
			uint16_t tokenI = parameters.indexOf("-token");
			if (tokenI >= 0) {
				String tokenC = parameters.substring(tokenI+7);
				if (tokenC.length() >= 6) {
					tokenC.toCharArray(thisConfig.token.token, 7);
					thisConfig.token.set = true;
				}
			}
			base->saveConfig(thisConfig);
		}
		sprintf(base->outBuff, "-- New config --\r\n");

		// Get
	} else base->outBuff[0] = 0;

	Configuration currentConfig = base->getConfig();

	sprintf(base->outBuff, "%sMode: %s\r\nPublish interval: %lu\r\n", base->outBuff, modeTitles[currentConfig.mode], currentConfig.publishInterval);

	sprintf(base->outBuff, "%sWifi credentials: ", base->outBuff);
	if (currentConfig.credentials.set) sprintf(base->outBuff, "%s%s - %s\r\n", base->outBuff, currentConfig.credentials.ssid, currentConfig.credentials.pass);
	else sprintf(base->outBuff, "%snot configured\r\n", base->outBuff);

	sprintf(base->outBuff, "%sToken: ", base->outBuff);
	if (currentConfig.token.set) sprintf(base->outBuff, "%s%s", base->outBuff, currentConfig.token.token);
	else sprintf(base->outBuff, "%snot configured", base->outBuff);
	base->sckOut();
}
void esp_com(SckBase* base, String parameters)
{

	if (parameters.length() <= 0) {
		base->sckOut("Parameters: on/off/reboot/flash/debug");
	} else if (parameters.equals("on")) base->ESPcontrol(base->ESP_ON);
	else if (parameters.equals("off")) base->ESPcontrol(base->ESP_OFF);
	else if (parameters.equals("reboot")) base->ESPcontrol(base->ESP_REBOOT);
	else if (parameters.equals("flash")) base->ESPcontrol(base->ESP_FLASH);
	else if (parameters.equals("debug")) {
		// TODO toggle esp debug
	}
}
void netInfo_com(SckBase* base, String parameters)
{

	base->sendMessage(ESPMES_GET_NETINFO);
}
void time_com(SckBase* base, String parameters)
{

	if (parameters.length() <= 0) {

		if (base->ISOtime()) {
			sprintf(base->outBuff, "Time: %s", base->ISOtimeBuff);
			base->sckOut();
		} else {
			base->sckOut("Time is not synced, trying to sync...");
			base->sendMessage(ESPMES_GET_TIME, "");
		}
	} else if (parameters.toInt() > 0) base->setTime(parameters);
}
void state_com(SckBase* base, String parameters)
{

	base->printState();
}
void hello_com(SckBase* base, String parameters)
{

	base->st.helloPending = true;
	base->sckOut("Waiting for MQTT hello response...");
}
void debug_com(SckBase* base, String parameters)
{

	if (parameters.length() > 0) {
		if (parameters.equals("-light")) {
			base->readLight.debugFlag = !base->readLight.debugFlag;
			sprintf(base->outBuff, "ReadLight debugFlag: %s", base->readLight.debugFlag  ? "true" : "false");
			base->sckOut();
		}
	}
}
