#include <cstdint>
/*
    IO.cpp is part of DualSenseWindows
    https://github.com/Ohjurot/DualSense-Windows

    Contributors of this file:
    11.2020 Ludwig Füchsl

    Licensed under the MIT License (To be found in repository root directory)
*/

#include <hidapi.h>
#include "IO.h"
#include "DS_CRC32.h"
#include "DS5_Input.h"
#include "DS5_Output.h"

#include <cstdlib>
#include <cstring>

// Cross-platform handle for HID device

DS5W_API DS5W_ReturnValue DS5W::enumDevices(void* ptrBuffer, unsigned int inArrLength, unsigned int* requiredLength, bool pointerToArray) {
    struct hid_device_info* devices = hid_enumerate(0x054C, 0x0CE6);  // Sony vendor ID and DualSense product ID

    // Enumerate HID devices and fill the buffer
    unsigned int inputArrIndex = 0;
    for (struct hid_device_info* dev = devices; dev != nullptr; dev = dev->next) {
        if (inputArrIndex >= inArrLength) {
            break;  // Stop if buffer limit is reached
        }

        DS5W::DeviceEnumInfo* ptrInfo = nullptr;
        if (pointerToArray) {
            ptrInfo = &(((DS5W::DeviceEnumInfo*)ptrBuffer)[inputArrIndex]);
        } else {
            ptrInfo = (((DS5W::DeviceEnumInfo**)ptrBuffer)[inputArrIndex]);
        }

        // Copy path
        if (ptrInfo) {
            std::strncpy(ptrInfo->_internal.path, dev->path, 260);
            ptrInfo->_internal.connection = (dev->interface_number == 0) ? DS5W::DeviceConnection::USB : DS5W::DeviceConnection::BT;
        }

        inputArrIndex++;
    }

    // Set required size
    if (requiredLength) {
        *requiredLength = inputArrIndex;
    }

    // Cleanup
    hid_free_enumeration(devices);

    // Return success or insufficient buffer error
    if (inputArrIndex <= inArrLength) {
        return DS5W_OK;
    } else {
        return DS5W_E_INSUFFICIENT_BUFFER;
    }
}

DS5W_API DS5W_ReturnValue DS5W::initDeviceContext(DS5W::DeviceEnumInfo* ptrEnumInfo, DS5W::DeviceContext* ptrContext) {
    if (!ptrEnumInfo || !ptrContext || std::strlen(ptrEnumInfo->_internal.path) == 0) {
        return DS5W_E_INVALID_ARGS;
    }

    // Open HID device
    hid_device* deviceHandle = hid_open_path(ptrEnumInfo->_internal.path);
    if (!deviceHandle) {
        return DS5W_E_DEVICE_REMOVED;
    }

    // Write to context
    ptrContext->_internal.connected = true;
    ptrContext->_internal.connection = ptrEnumInfo->_internal.connection;
    ptrContext->_internal.deviceHandle = deviceHandle;
    std::strncpy(ptrContext->_internal.devicePath, ptrEnumInfo->_internal.path, 260);

    return DS5W_OK;
}

DS5W_API void DS5W::freeDeviceContext(DS5W::DeviceContext* ptrContext) {
    if (ptrContext->_internal.deviceHandle) {
        // Reset the device's state and free the handle
        hid_close((hid_device*)ptrContext->_internal.deviceHandle);
        ptrContext->_internal.deviceHandle = nullptr;
    }

    ptrContext->_internal.connected = false;
    std::memset(ptrContext->_internal.devicePath, 0, sizeof(ptrContext->_internal.devicePath));
}

DS5W_API DS5W_ReturnValue DS5W::reconnectDevice(DS5W::DeviceContext* ptrContext) {
    if (!ptrContext || std::strlen(ptrContext->_internal.devicePath) == 0) {
        return DS5W_E_INVALID_ARGS;
    }

    // Reopen HID device
    hid_device* deviceHandle = hid_open_path(ptrContext->_internal.devicePath);
    if (!deviceHandle) {
        return DS5W_E_DEVICE_REMOVED;
    }

    ptrContext->_internal.deviceHandle = deviceHandle;
    ptrContext->_internal.connected = true;

    return DS5W_OK;
}

DS5W_API DS5W_ReturnValue DS5W::getDeviceInputState(DS5W::DeviceContext* ptrContext, DS5W::DS5InputState* ptrInputState) {
    if (!ptrContext || !ptrInputState || !ptrContext->_internal.connected) {
        return DS5W_E_INVALID_ARGS;
    }

    // Get the input report
    unsigned char inputBuffer[78];
    int bytesRead = hid_read((hid_device*)ptrContext->_internal.deviceHandle, inputBuffer, sizeof(inputBuffer));

    if (bytesRead < 0) {
        hid_close((hid_device*)ptrContext->_internal.deviceHandle);
        ptrContext->_internal.deviceHandle = nullptr;
        ptrContext->_internal.connected = false;
        return DS5W_E_DEVICE_REMOVED;
    }

    // Evaluate input buffer (Bluetooth or USB)
    if (ptrContext->_internal.connection == DS5W::DeviceConnection::BT) {
        __DS5W::Input::evaluateHidInputBuffer(&inputBuffer[2], ptrInputState);
    } else {
        __DS5W::Input::evaluateHidInputBuffer(&inputBuffer[1], ptrInputState);
    }

    return DS5W_OK;
}

DS5W_API DS5W_ReturnValue DS5W::setDeviceOutputState(DS5W::DeviceContext* ptrContext, DS5W::DS5OutputState* ptrOutputState) {
    if (!ptrContext || !ptrOutputState || !ptrContext->_internal.connected) {
        return DS5W_E_INVALID_ARGS;
    }

    unsigned char outputBuffer[547] = {0};

    // Build the output report for BT or USB
    if (ptrContext->_internal.connection == DS5W::DeviceConnection::BT) {
        outputBuffer[0x00] = 0x31;
        outputBuffer[0x01] = 0x02;
        __DS5W::Output::createHidOutputBuffer(&outputBuffer[2], ptrOutputState);

        uint32_t crcChecksum = __DS5W::CRC32::compute(outputBuffer, 74);
        outputBuffer[0x4A] = (unsigned char)((crcChecksum >> 0) & 0xFF);
        outputBuffer[0x4B] = (unsigned char)((crcChecksum >> 8) & 0xFF);
        outputBuffer[0x4C] = (unsigned char)((crcChecksum >> 16) & 0xFF);
        outputBuffer[0x4D] = (unsigned char)((crcChecksum >> 24) & 0xFF);
    } else {
        outputBuffer[0x00] = 0x02;
        __DS5W::Output::createHidOutputBuffer(&outputBuffer[1], ptrOutputState);
    }

    // Write output to the device
    if (hid_write((hid_device*)ptrContext->_internal.deviceHandle, outputBuffer, sizeof(outputBuffer)) < 0) {
        hid_close((hid_device*)ptrContext->_internal.deviceHandle);
        ptrContext->_internal.deviceHandle = nullptr;
        ptrContext->_internal.connected = false;
        return DS5W_E_DEVICE_REMOVED;
    }

    return DS5W_OK;
}
