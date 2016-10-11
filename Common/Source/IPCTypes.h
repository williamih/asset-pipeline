#ifndef IPCTYPES_H
#define IPCTYPES_H

enum IPCAppToHelperAction {
    IPCAPPTOHELPER_SHOW_PROJECTS_WINDOW,
    IPCAPPTOHELPER_SHOW_ERRORS_WINDOW,
    IPCAPPTOHELPER_SHOW_ABOUT_WINDOW,

    IPCAPPTOHELPER_REFRESH_ERRORS,

    IPCAPPTOHELPER_QUIT,
};

enum IPCHelperToAppAction {
    IPCHELPERTOAPP_QUIT,
};

#endif // IPCTYPES_H
