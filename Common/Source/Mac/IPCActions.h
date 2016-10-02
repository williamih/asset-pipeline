#ifndef MAC_IPCACTIONS_H
#define MAC_IPCACTIONS_H

#include <stdint.h>

typedef NS_ENUM(uint8_t, IPCAppToHelperAction) {
    IPCAppToHelperActionQuit,
    IPCAppToHelperActionQuitReceived,
    IPCAppToHelperActionShowProjectWindow
};

typedef NS_ENUM(uint8_t, IPCHelperToAppAction) {
    IPCHelperToAppActionQuit,
    IPCHelperToAppActionQuitReceived
};

#endif // MAC_IPCACTIONS_H
