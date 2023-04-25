#pragma once
#ifndef OUTPUT_MESSAGES_H
#define OUTPUT_MESSAGES_H

enum OutputMessageType {
    SUCCESS = 0,
    SHUTDOWN = 1,
    DISCONNECT = 2,
    BIND_ERROR = 3,
    CONNECT_ERROR = 4,
    SETUP_ERROR = 5,
    STARTUP_ERROR = 6,
    ADDRESS_ERROR = 7,
    PARAMETER_ERROR = 8,
};

#endif // OUTPUT_MESSAGES_H