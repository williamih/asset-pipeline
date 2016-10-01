import Foundation

enum IPCAppToHelperAction: UInt8 {
    case quit = 1
    case quitReceived = 2
    case showProjectWindow = 3
}

enum IPCHelperToAppAction: UInt8 {
    case quit = 1
    case quitReceived = 2
}
