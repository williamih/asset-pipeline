#ifndef IPCTYPES_H
#define IPCTYPES_H

enum IPCAppToHelperAction {
    IPCAPPTOHELPER_SHOW_PROJECTS_WINDOW,
    IPCAPPTOHELPER_QUIT,
};

enum IPCHelperToAppAction {
    IPCHELPERTOAPP_QUIT,
};

#endif // IPCTYPES_H
