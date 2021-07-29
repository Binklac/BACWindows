#include "LpcClient.h"
#include <Windows.h>
#include <stdlib.h>
#include "CallbackLogger.h"
#include <cstdint>

DWORD WINAPI LpcListen(LPVOID lpThreadParameter) {
    PPORT_MESSAGE                  LpcSendMessageBuffer          = nullptr;
    PPORT_MESSAGE                  LpcReceiveMessageBuffer       = nullptr;
    PLPC_MESSAGE_INFORMATION       MessageInformation            = nullptr;
    PLPC_REPLY_MESSAGE_INFORMATION ReplyMessageInformation       = nullptr;
    PUCHAR                         MessageToBeSent               = nullptr;
    PUCHAR                         MessageReceived               = nullptr;
    SIZE_T                         LpcReceiveMessageBufferLength = PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH;
    NTSTATUS                       Status                        = STATUS_SUCCESS;
    SIZE_T                         MessageToBeSentLength         = INT64_MAX;

    if ((LpcSendMessageBuffer = reinterpret_cast<PPORT_MESSAGE>(malloc(PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH))) == nullptr) {
        LOG_ERROR("Unable to allocate memory.");
        goto CleanAndExit;
    }

    if ((LpcReceiveMessageBuffer = reinterpret_cast<PPORT_MESSAGE>(malloc(PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH))) == nullptr) {
        LOG_ERROR("Unable to allocate memory.");
        goto CleanAndExit;
    }

    while (!LpcClient::StopLpcClient) {
        RtlZeroMemory(LpcSendMessageBuffer, PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH);
        RtlZeroMemory(LpcReceiveMessageBuffer, PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH);
        RtlZeroMemory(&MessageToBeSent, sizeof(PUCHAR));
        RtlZeroMemory(&MessageReceived, sizeof(PUCHAR));
        MessageToBeSentLength = INT64_MAX;

        if (NT_SUCCESS(Status = NtAlpcSendWaitReceivePort(lpThreadParameter, 0, NULL, NULL, LpcReceiveMessageBuffer, &LpcReceiveMessageBufferLength, NULL, NULL))) {
            LOG_DEBUG("An ALPC message is received, the category is %X. Message id is %08X", LpcReceiveMessageBuffer->u2.s2.Type, LpcReceiveMessageBuffer->MessageId);

            if (((BYTE)LpcReceiveMessageBuffer->u2.s2.Type) == 0x01) {
                MessageInformation = reinterpret_cast<PLPC_MESSAGE_INFORMATION>((reinterpret_cast<PUCHAR>(LpcReceiveMessageBuffer) + sizeof(PORT_MESSAGE)));
                MessageReceived    = reinterpret_cast<PUCHAR>((reinterpret_cast<PUCHAR>(LpcReceiveMessageBuffer) + sizeof(PORT_MESSAGE) + sizeof(LPC_MESSAGE_INFORMATION)));
                MessageToBeSent    = reinterpret_cast<PUCHAR>((reinterpret_cast<PUCHAR>(LpcSendMessageBuffer) + sizeof(PORT_MESSAGE) + sizeof(LPC_REPLY_MESSAGE_INFORMATION)));

                LpcClient::DispathLpcMessage(MessageInformation->MessageType, MessageReceived, LpcReceiveMessageBuffer->u1.s1.DataLength - sizeof(LPC_REPLY_MESSAGE_INFORMATION), MessageToBeSent, &MessageToBeSentLength);

                if (MessageInformation->MessageControl == LPC_MESSAGE_CONTROL::REPLY_AS_CALLBACK) {
                    LpcSendMessageBuffer->MessageId         = LpcReceiveMessageBuffer->MessageId;
                    LpcSendMessageBuffer->u1.s1.DataLength  = static_cast<CSHORT>(sizeof(LPC_REPLY_MESSAGE_INFORMATION) + MessageToBeSentLength);
                    LpcSendMessageBuffer->u1.s1.TotalLength = static_cast<CSHORT>(sizeof(PORT_MESSAGE) + sizeof(LPC_REPLY_MESSAGE_INFORMATION) + MessageToBeSentLength);
                    ReplyMessageInformation                 = reinterpret_cast<PLPC_REPLY_MESSAGE_INFORMATION>((reinterpret_cast<PUCHAR>(LpcSendMessageBuffer) + sizeof(PORT_MESSAGE)));
                    ReplyMessageInformation->ReplyControl   = MessageToBeSentLength == INT64_MAX ? LPC_REPLY_CONTROL::FAILED : LPC_REPLY_CONTROL::PROCEED;

                    if (NT_SUCCESS(Status = NtAlpcSendWaitReceivePort(lpThreadParameter, ALPC_MSGFLG_REPLY_MESSAGE, LpcSendMessageBuffer, NULL, NULL, NULL, NULL, NULL))) {
                        LOG_DEBUG("The reply to message %08X was successfully sent.", LpcReceiveMessageBuffer->MessageId);
                    } else {
                        LOG_DEBUG("Sending the reply to message %08X failed. The error code is %08X", LpcReceiveMessageBuffer->MessageId, Status);
                    }
                }
            } else {
                LOG_DEBUG("Messages of unknown type, ignored.");
            }
        }
    }

CleanAndExit:
    if (LpcSendMessageBuffer != nullptr) {
        free(LpcSendMessageBuffer);
        LpcSendMessageBuffer = nullptr;
    }

    if (LpcReceiveMessageBuffer != nullptr) {
        free(LpcReceiveMessageBuffer);
        LpcReceiveMessageBuffer = nullptr;
    }

    return 0;
}

VOID LpcClient::DispathLpcMessage(LPC_MESSAGE_TYPE MessageType, PBYTE MessageBuffer, SIZE_T MessageLength, PBYTE ResponseBuffer, PSIZE_T ResponseLength) {
    LOG_DEBUG("An ALPC message procees");
}

BOOL LpcClient::ConnectToLpcControlPort(LPC_CLIENT_TYPE ClientType) {
    UNICODE_STRING                  ServerPort               = {};
    PPORT_MESSAGE                   ConnectMessage           = nullptr;
    HANDLE                          ServerPortHandle         = {};
    DWORD                           CreatedThreadId          = {};
    ALPC_PORT_ATTRIBUTES            PortAttributes           = {};
    PLPC_CLIENT_CONNECT_INFORMATION ClientConnectInformation = nullptr;

    RtlInitUnicodeString(&ServerPort, BAC_LPC_PORT_NAME);
    RtlSecureZeroMemory(&PortAttributes, sizeof(ALPC_PORT_ATTRIBUTES));

    if ((ConnectMessage = reinterpret_cast<PPORT_MESSAGE>(malloc(sizeof(PORT_MESSAGE) + sizeof(LPC_CLIENT_CONNECT_INFORMATION)))) == nullptr) {
        LOG_ERROR("Unable to allocate memory.");
        return FALSE;
    }

    ConnectMessage->u2.s2.Type           = LPC_CONNECTION_REQUEST;
    ConnectMessage->u1.s1.DataLength     = sizeof(LPC_CLIENT_CONNECT_INFORMATION);
    ConnectMessage->u1.s1.TotalLength    = sizeof(PORT_MESSAGE) + sizeof(LPC_CLIENT_CONNECT_INFORMATION);
    ClientConnectInformation             = reinterpret_cast<PLPC_CLIENT_CONNECT_INFORMATION>(reinterpret_cast<PUCHAR>(ConnectMessage) + sizeof(PORT_MESSAGE));
    ClientConnectInformation->ClientType = ClientType;
    PortAttributes.MaxMessageLength      = PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH;
    PortAttributes.DupObjectTypes        = 0xFFFFFFFF;
    PortAttributes.Flags                 = 0x80000 | 0x20000 | 0x40000;

    if (NT_SUCCESS(NtAlpcConnectPort(&ServerPortHandle, &ServerPort, NULL, &PortAttributes, 0, 0, ConnectMessage, NULL, 0, 0, 0))) {
        LOG_DEBUG("Successfully connected to the kernel driver and started to monitor the data sent from the kernel.");
        CreateThread(NULL, NULL, LpcListen, ServerPortHandle, NULL, &CreatedThreadId);
    } else {
        LOG_ERROR("Cannot connect to the kernel driver, is there any problem?");
    }

    return 0;
}

BOOL LpcClient::StopLpcClient = FALSE;
