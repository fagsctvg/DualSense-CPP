#include <DualSenseWindows/IO.h>
#include <DualSenseWindows/DS_CRC32.h>
#include <DualSenseWindows/DS5_Input.h>
#include <DualSenseWindows/DS5_Output.h>

#include <hidapi.h>  // Include hidapi for cross-platform support

#ifdef _WIN32
    #define NOMINMAX
    #include <Windows.h>
#else
    #include <wchar.h>
#endif

DS5W_API DS5W_ReturnValue DS5W::enumDevices(void* ptrBuffer, unsigned int inArrLength, unsigned int* requiredLength, bool pointerToArray) {
    // HID enumeration using hidapi
    struct hid_device_info* devs = hid_enumerate(0x054C, 0x0CE6);  // Use Sony's vendor/product IDs for DualSense
    struct hid_device_info* currentDev = devs;

    unsigned int inputArrIndex = 0;
    bool inputArrOverflow = false;

    while (currentDev) {
        // Copy device information to output buffer if space is available
        DS5W::DeviceEnumInfo* ptrInfo = nullptr;
        if (inputArrIndex < inArrLength) {
            if (pointerToArray) {
                ptrInfo = &(((DS5W::DeviceEnumInfo*)ptrBuffer)[inputArrIndex]);
            }
            else {
                ptrInfo = (((DS5W::DeviceEnumInfo**)ptrBuffer)[inputArrIndex]);
            }

            // Copy device path
            mbstowcs(ptrInfo->_internal.path, currentDev->path, 260);

            // Set connection type (for simplicity, assuming USB here)
            ptrInfo->_internal.connection = DS5W::DeviceConnection::USB;

            // Increment index
            inputArrIndex++;
        }

        currentDev = currentDev->next;
    }

    hid_free_enumeration(devs);

    // Set required size if exists
    if (requiredLength) {
        *requiredLength = inputArrIndex;
    }

    return inputArrIndex <= inArrLength ? DS5W_OK : DS5W_E_INSUFFICIENT_BUFFER;
}

DS5W_API DS5W_ReturnValue DS5W::initDeviceContext(DS5W::DeviceEnumInfo* ptrEnumInfo, DS5W::DeviceContext* ptrContext) {
    // Open HID device using hidapi
    hid_device* deviceHandle = hid_open_path(ptrEnumInfo->_internal.path);
    if (!deviceHandle) {
        return DS5W_E_DEVICE_REMOVED;
    }

    // Write to context
    ptrContext->_internal.connected = true;
    ptrContext->_internal.connection = ptrEnumInfo->_internal.connection;
    ptrContext->_internal.deviceHandle = deviceHandle;

    return DS5W_OK;
}

DS5W_API void DS5W::freeDeviceContext(DS5W::DeviceContext* ptrContext) {
    if (ptrContext->_internal.deviceHandle) {
        DS5W::DS5OutputState os = {};  // Zeroing output state
        DS5W::setDeviceOutputState(ptrContext, &os);

        // Close the HID device
        hid_close(ptrContext->_internal.deviceHandle);
        ptrContext->_internal.deviceHandle = nullptr;
    }

    ptrContext->_internal.connected = false;
}

DS5W_API DS5W_ReturnValue DS5W::getDeviceInputState(DS5W::DeviceContext* ptrContext, DS5W::DS5InputState* ptrInputState) {
    if (!ptrContext || !ptrInputState) {
        return DS5W_E_INVALID_ARGS;
    }

    if (!ptrContext->_internal.connected) {
        return DS5W_E_DEVICE_REMOVED;
    }

    // Read input report using hidapi
    unsigned char buf[78];
    int res = hid_read(ptrContext->_internal.deviceHandle, buf, sizeof(buf));
    if (res < 0) {
        return DS5W_E_DEVICE_REMOVED;
    }

    // Process input buffer (for Bluetooth in this case)
    if (ptrContext->_internal.connection == DS5W::DeviceConnection::BT) {
        __DS5W::Input::evaluateHidInputBuffer(&buf[2], ptrInputState);
    }
    else {
        __DS5W::Input::evaluateHidInputBuffer(&buf[1], ptrInputState);
    }

    return DS5W_OK;
}

DS5W_API DS5W_ReturnValue DS5W::setDeviceOutputState(DS5W::DeviceContext* ptrContext, DS5W::DS5OutputState* ptrOutputState) {
    if (!ptrContext || !ptrOutputState) {
        return DS5W_E_INVALID_ARGS;
    }

    if (!ptrContext->_internal.connected) {
        return DS5W_E_DEVICE_REMOVED;
    }

    unsigned char buf[547];
    ZeroMemory(buf, sizeof(buf));

    if (ptrContext->_internal.connection == DS5W::DeviceConnection::BT) {
        buf[0x00] = 0x31;
        buf[0x01] = 0x02;
        __DS5W::Output::createHidOutputBuffer(&buf[2], ptrOutputState);
    }
    else {
        buf[0x00] = 0x02;
        __DS5W::Output::createHidOutputBuffer(&buf[1], ptrOutputState);
    }

    int res = hid_write(ptrContext->_internal.deviceHandle, buf, sizeof(buf));
    if (res < 0) {
        return DS5W_E_DEVICE_REMOVED;
    }

    return DS5W_OK;
}
