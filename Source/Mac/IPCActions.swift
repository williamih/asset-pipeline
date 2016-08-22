import Foundation

enum IPCAppToHelperAction: UInt8 {
    case Quit = 1
    case QuitReceived = 2
    case ShowProjectWindow = 3
}

enum IPCHelperToAppAction: UInt8 {
    case Quit = 1
    case QuitReceived = 2
}