#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>  // For std::system
#include <thread>
#include <chrono>
#include "IO.h"
#include "Device.h"
#include "Helpers.h"
#include "hidapi/hidapi.h"

typedef std::wstringstream wstrBuilder;

class Console {
public:
    void writeLine(const std::wstring& text) {
        write(text);
        write(L"\n");
    }

    void writeLine(wstrBuilder& builder) {
        writeLine(builder.str());
        builder.str(L"");
    }

    void write(const std::wstring& text) {
        std::wcout << text;  // Use standard output for Unix-like systems
    }

    void write(wstrBuilder& builder) {
        write(builder.str());
        builder.str(L"");
    }
};

int main() {
    // Console
    Console console;
    wstrBuilder builder;
    console.writeLine(L"DualSense Controller Test\n========================\n");

    // Enum all controllers present
    DS5W::DeviceEnumInfo infos[16];
    unsigned int controllersCount = 0;
    DS5W_ReturnValue rv = DS5W::enumDevices(infos, 16, &controllersCount);

    // Check size
    if (controllersCount == 0) {
        console.writeLine(L"No DualSense controller found!");
        std::wcout << L"Press Enter to exit." << std::endl;
        std::cin.get();  // Wait for user input
        return -1;
    }

    // Print all controllers
    builder << L"Found " << controllersCount << L" DualSense Controller(s):";
    console.writeLine(builder);

    // Iterate controllers
    for (unsigned int i = 0; i < controllersCount; i++) {
        if (infos[i]._internal.connection == DS5W::DeviceConnection::BT) {
            builder << L"Wireless (Bluetooth) controller (";
        } else {
            builder << L"Wired (USB) controller (";
        }

        builder << infos[i]._internal.path << L")";
        console.writeLine(builder);
    }

    // Create first controller
    DS5W::DeviceContext con;
    if (DS5W_SUCCESS(DS5W::initDeviceContext(&infos[0], &con))) {
        console.writeLine(L"DualSense controller connected");

        // Title
        builder << L"DS5 (";
        if (con._internal.connection == DS5W::DeviceConnection::BT) {
            builder << L"BT";
        } else {
            builder << L"USB";
        }
        builder << L") Press L1 and R1 to exit";
        builder.str(L""); // Clear the builder

        // State object
        DS5W::DS5InputState inState;
        DS5W::DS5OutputState outState;
        std::memset(&inState, 0, sizeof(DS5W::DS5InputState));
        std::memset(&outState, 0, sizeof(DS5W::DS5OutputState));

        // Color intensity
        float intensity = 1.0f;
        uint16_t lrmbl = 0;
        uint16_t rrmbl = 0;

        // Force
        DS5W::TriggerEffectType rType = DS5W::TriggerEffectType::NoResitance;

        int btMul = con._internal.connection == DS5W::DeviceConnection::BT ? 10 : 1;

        // Application infinity loop
        while (!(inState.buttonsA & DS5W_ISTATE_BTN_A_LEFT_BUMPER && inState.buttonsA & DS5W_ISTATE_BTN_A_RIGHT_BUMPER)) {
            // Get input state
            if (DS5W_SUCCESS(DS5W::getDeviceInputState(&con, &inState))) {
                // === Read Input ===
                // Build all universal buttons (USB and BT) as text
                builder << L"\n === Universal input ===\n";

                builder << L"Left Stick\tX: " << (int)inState.leftStick.x << L"\tY: " << (int)inState.leftStick.y << (inState.buttonsA & DS5W_ISTATE_BTN_A_LEFT_STICK ? L"\tPUSH" : L"") << L"\n";
                builder << L"Right Stick\tX: " << (int)inState.rightStick.x << L"\tY: " << (int)inState.rightStick.y << (inState.buttonsA & DS5W_ISTATE_BTN_A_RIGHT_STICK ? L"\tPUSH" : L"") << L"\n\n";

                builder << L"Left Trigger:  " << (int)inState.leftTrigger << L"\tBinary active: " << (inState.buttonsA & DS5W_ISTATE_BTN_A_LEFT_TRIGGER ? L"Yes" : L"No") << (inState.buttonsA & DS5W_ISTATE_BTN_A_LEFT_BUMPER ? L"\tBUMPER" : L"") << L"\n";
                builder << L"Right Trigger: " << (int)inState.rightTrigger << L"\tBinary active: " << (inState.buttonsA & DS5W_ISTATE_BTN_A_RIGHT_TRIGGER ? L"Yes" : L"No") << (inState.buttonsA & DS5W_ISTATE_BTN_A_RIGHT_BUMPER ? L"\tBUMPER" : L"") << L"\n\n";

                builder << L"DPAD: " << (inState.buttonsAndDpad & DS5W_ISTATE_DPAD_LEFT ? L"L " : L"  ") << (inState.buttonsAndDpad & DS5W_ISTATE_DPAD_UP ? L"U " : L"  ") <<
                    (inState.buttonsAndDpad & DS5W_ISTATE_DPAD_DOWN ? L"D " : L"  ") << (inState.buttonsAndDpad & DS5W_ISTATE_DPAD_RIGHT ? L"R " : L"  ");
                builder << L"\tButtons: " << (inState.buttonsAndDpad & DS5W_ISTATE_BTX_SQUARE ? L"S " : L"  ") << (inState.buttonsAndDpad & DS5W_ISTATE_BTX_CROSS ? L"X " : L"  ") <<
                    (inState.buttonsAndDpad & DS5W_ISTATE_BTX_CIRCLE ? L"O " : L"  ") << (inState.buttonsAndDpad & DS5W_ISTATE_BTX_TRIANGLE ? L"T " : L"  ") << L"\n";
                builder << (inState.buttonsA & DS5W_ISTATE_BTN_A_MENU ? L"MENU" : L"") << (inState.buttonsA & DS5W_ISTATE_BTN_A_SELECT ? L"\tSELECT" : L"") << L"\n";

                builder << L"Trigger Feedback:\tLeft: " << (int)inState.leftTriggerFeedback << L"\tRight: " << (int)inState.rightTriggerFeedback << L"\n\n";

                builder << L"Touchpad" << (inState.buttonsB & DS5W_ISTATE_BTN_B_PAD_BUTTON ? L" (pushed):" : L":") << L"\n";

                builder << L"Finger 1\tX: " << inState.touchPoint1.x << L"\t Y: " << inState.touchPoint1.y << L"\n";
                builder << L"Finger 2\tX: " << inState.touchPoint2.x << L"\t Y: " << inState.touchPoint2.y << L"\n\n";

                builder << L"Battery: " << inState.battery.level << (inState.battery.chargin ? L" Charging" : L"") << (inState.battery.fullyCharged ? L"  Fully charged" : L"") << L"\n\n";

                builder << (inState.buttonsB & DS5W_ISTATE_BTN_B_PLAYSTATION_LOGO ? L"PLAYSTATION" : L"") << (inState.buttonsB & DS5W_ISTATE_BTN_B_MIC_BUTTON ? L"\tMIC" : L"") << L"\n";

                // Omitted accel and gyro

                // Print to console
                console.writeLine(builder);

                // === Write Output ===
                // Rumble
                lrmbl = std::max(lrmbl - 0x200 / btMul, 0);
                rrmbl = std::max(rrmbl - 0x100 / btMul, 0);

                outState.leftRumble = (lrmbl & 0xFF00) >> 8UL;
                outState.rightRumble = (rrmbl & 0xFF00) >> 8UL;

                // Set rumble
                DS5W::setDeviceOutputState(&con, &outState);

                // Delay loop
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        // Disconnect
        //DS5W::disconnectDevice(&con);
        //console.writeLine(L"DualSense controller disconnected");
    } else {
        console.writeLine(L"Failed to connect to DualSense controller");
    }

    std::wcout << L"Press Enter to exit." << std::endl;
    std::cin.get();  // Wait for user input
    return 0;
}
