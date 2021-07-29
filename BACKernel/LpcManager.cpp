#include "GlobalDeclaration.h"
#include "LpcManager.h"
#include "NtLpcApi.h"
#include "LpcMessageDefine.h"
#include "LpcDispatch.h"

namespace Binklac::Kernel::Lpc {
    BOOLEAN LpcManager::Initialization() {
        UNICODE_STRING NtAlpcCreatePortName              = {};
        UNICODE_STRING NtAlpcSendWaitReceivePortName     = {};
        UNICODE_STRING NtAlpcAcceptConnectPortName       = {};
        UNICODE_STRING NtAlpcDisconnectPortName          = {};
        UNICODE_STRING NtAlpcQueryInformationMessageName = {};

        RtlInitUnicodeString(&NtAlpcCreatePortName, L"ZwAlpcCreatePort");
        RtlInitUnicodeString(&NtAlpcSendWaitReceivePortName, L"ZwAlpcSendWaitReceivePort");
        RtlInitUnicodeString(&NtAlpcAcceptConnectPortName, L"ZwAlpcAcceptConnectPort");
        RtlInitUnicodeString(&NtAlpcDisconnectPortName, L"ZwAlpcDisconnectPort");
        RtlInitUnicodeString(&NtAlpcQueryInformationMessageName, L"ZwAlpcQueryInformationMessage");

        if ((LpcManager::_NtAlpcCreatePort = reinterpret_cast<LpcManager::NtAlpcCreatePortType>(MmGetSystemRoutineAddress(&NtAlpcCreatePortName))) == nullptr) {
            LOG_ERROR("Unable to find the address of function NtAlpcCreatePort, unsupported system version?");
            return FALSE;
        } else {
            LOG_DEBUG("Find the address of function NtAlpcCreatePort :                  %p", LpcManager::_NtAlpcCreatePort);
        }

        if ((LpcManager::_NtAlpcSendWaitReceivePort = reinterpret_cast<LpcManager::NtAlpcSendWaitReceivePortType>(MmGetSystemRoutineAddress(&NtAlpcSendWaitReceivePortName))) == nullptr) {
            LOG_ERROR("Unable to find the address of function NtAlpcSendWaitReceivePort, unsupported system version?");
            return FALSE;
        } else {
            LOG_DEBUG("Find the address of function NtAlpcSendWaitReceivePort :         %p", LpcManager::_NtAlpcSendWaitReceivePort);
        }

        if ((LpcManager::_NtAlpcAcceptConnectPort = reinterpret_cast<LpcManager::NtAlpcAcceptConnectPortType>(MmGetSystemRoutineAddress(&NtAlpcAcceptConnectPortName))) == nullptr) {
            LOG_ERROR("Unable to find the address of function NtAlpcAcceptConnectPort, unsupported system version?");
            return FALSE;
        } else {
            LOG_DEBUG("Find the address of function NtAlpcAcceptConnectPort :           %p", LpcManager::_NtAlpcAcceptConnectPort);
        }

        if ((LpcManager::_NtAlpcDisconnectPort = reinterpret_cast<LpcManager::NtAlpcDisconnectPortType>(MmGetSystemRoutineAddress(&NtAlpcDisconnectPortName))) == nullptr) {
            LOG_ERROR("Unable to find the address of function NtAlpcDisconnectPort, unsupported system version?");
            return FALSE;
        } else {
            LOG_DEBUG("Find the address of function NtAlpcDisconnectPort :              %p", LpcManager::_NtAlpcDisconnectPort);
        }

        if ((LpcManager::_NtAlpcQueryInformationMessage = reinterpret_cast<LpcManager::NtAlpcQueryInformationMessageType>(MmGetSystemRoutineAddress(&NtAlpcQueryInformationMessageName))) == nullptr) {
            LOG_ERROR("Unable to find the address of function NtAlpcDisconnectPort, unsupported system version?");
            return FALSE;
        } else {
            LOG_DEBUG("Find the address of function NtAlpcQueryInformationMessage :     %p", LpcManager::_NtAlpcQueryInformationMessage);
        }

        LpcManager::ClientRecord.KQueueInit();

        return TRUE;
    }

    BOOLEAN LpcManager::CreateLpcPort() {
        UNICODE_STRING       BACControlPortName             = {};
        OBJECT_ATTRIBUTES    BACControlPortObjectAttributes = {};
        ALPC_PORT_ATTRIBUTES BACControlPortAttributes       = {};
        NTSTATUS             Status                         = STATUS_SUCCESS;

        RtlInitUnicodeString(&BACControlPortName, BAC_LPC_PORT_NAME);
        InitializeObjectAttributes(&BACControlPortObjectAttributes, &BACControlPortName, OBJ_CASE_INSENSITIVE, 0, 0);
        BACControlPortAttributes.MaxMessageLength = PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH;

        if (!NT_SUCCESS(Status = LpcManager::_NtAlpcCreatePort(&LpcManager::LpcPortHandle, &BACControlPortObjectAttributes, &BACControlPortAttributes))) {
            LOG_ERROR("Unable to create the ALPC control port, the error code returned by the system is %08X", Status);
            return FALSE;
        } else {
            LOG_DEBUG("The ALPC control port was successfully created.");
            return TRUE;
        }
    }

    VOID LpcManager::LpcServerThreadRoutine(PVOID StartContext) {
        UNREFERENCED_PARAMETER(StartContext);

        PPORT_MESSAGE LpcMessageBuffer       = nullptr;
        SIZE_T        LpcMessageBufferLength = 0;
        NTSTATUS      Status                 = STATUS_SUCCESS;

        if ((LpcMessageBuffer = reinterpret_cast<PPORT_MESSAGE>(ExAllocatePoolZero(PagedPool, PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH, 'CABL'))) != nullptr) {
            while (!LpcManager::StopLpcServer) {
                RtlZeroMemory(LpcMessageBuffer, PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH);
                LpcMessageBufferLength = PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH;

                if (NT_SUCCESS(Status = LpcManager::_NtAlpcSendWaitReceivePort(LpcManager::LpcPortHandle, 0, NULL, NULL, LpcMessageBuffer, &LpcMessageBufferLength, NULL, NULL))) {
                    LOG_DEBUG("An ALPC message is received, the category is %X. Client process id is %d, Message id is %08X.", LpcMessageBuffer->u2.s2.Type, LpcMessageBuffer->ClientId.UniqueThread, LpcMessageBuffer->MessageId);

                    switch ((BYTE)LpcMessageBuffer->u2.s2.Type) {
                    case LPC_CONNECTION_REQUEST:
                        HandleConnectRequest(LpcMessageBuffer);
                        break;
                    case LPC_REQUEST:
                        LpcDispatch::DispathLpcMessage(LpcMessageBuffer);
                        break;
                    default:
                        LOG_DEBUG("Messages of unknown type, ignored.");
                    }
                } else {
                    LOG_ERROR("Unable to receive messages from the client. The error code returned by the system is %08X.", Status);
                }
            }
        }

        if (LpcMessageBuffer != nullptr) {
            ExFreePoolWithTag(LpcMessageBuffer, 'CABL');
        }

        LOG_ERROR("The LPC service thread has exited.");
        PsTerminateSystemThread(STATUS_SUCCESS);
    }

    BOOLEAN LpcManager::StartLpcServerThread() {
        NTSTATUS Status = STATUS_SUCCESS;

        if (NT_SUCCESS(Status = PsCreateSystemThread(&LpcManager::LpcServerThreadHandle, 0, NULL, NULL, NULL, reinterpret_cast<PKSTART_ROUTINE>(&LpcServerThreadRoutine), NULL))) {
            return TRUE;
        } else {
            LOG_ERROR("The thread cannot be created, and the error code returned by the system is %08X.", Status);
            return FALSE;
        }
    }

    VOID LpcManager::CloseLpcPort() {
        LpcManager::StopLpcServer = TRUE;

        LpcManager::ClientRecord.VisitEntity([&](LpcClientRecord *Record) -> BOOLEAN {
            if (Record->ClientPortHandle != nullptr) {
                LpcManager::_NtAlpcDisconnectPort(Record->ClientPortHandle, 0);
                NtClose(Record->ClientPortHandle);
                Record->ClientPortHandle = nullptr;
            }

            return TRUE;
        });

        if (LpcManager::LpcPortHandle != nullptr) {
            LpcManager::_NtAlpcDisconnectPort(LpcManager::LpcPortHandle, 0);
            NtClose(LpcManager::LpcPortHandle);
            LpcManager::LpcPortHandle = nullptr;
        }

        if (LpcManager::LpcServerThreadHandle != nullptr) {
            PETHREAD ThreadObject;

            if (NT_SUCCESS(ObReferenceObjectByHandle(LpcManager::LpcServerThreadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, reinterpret_cast<PVOID *>(&ThreadObject), NULL))) {
                LOG_DEBUG("Waiting for the LPC service thread to exit.");
                KeWaitForSingleObject(reinterpret_cast<PVOID>(ThreadObject), Executive, KernelMode, FALSE, NULL);
                ObDereferenceObject(reinterpret_cast<PVOID>(ThreadObject));
                LOG_DEBUG("Successfully waited the thread exit signal.");
            }

            LpcManager::LpcServerThreadHandle = nullptr;
        }

        LpcManager::ClientRecord.KQueueClear();
    }

    BOOL LpcManager::HandleProcessClose(HANDLE UniqueProcess) {
        LpcManager::ClientRecord.RemoveEntity([&](LpcClientRecord *Record) -> BOOL {
            if (Record->UniqueProcess == UniqueProcess) {
                LOG_DEBUG("The connection %08X from program %08X has failed, remove it", Record->ClientPortHandle, Record->UniqueProcess);
                return TRUE;
            } else {
                return FALSE;
            }
        });

        return TRUE;
    }

    BOOL LpcManager::HandleConnectRequest(PPORT_MESSAGE LpcMessageBuffer) {
        NTSTATUS                        Status                   = STATUS_SUCCESS;
        HANDLE                          UseLessHandle            = nullptr;
        LpcClientRecord *               CreatedClient            = nullptr;
        PLPC_CLIENT_CONNECT_INFORMATION ClientConnectInformation = nullptr;

        if (LpcMessageBuffer->u1.s1.DataLength != sizeof(LPC_CLIENT_CONNECT_INFORMATION)) {
            LOG_ERROR("Illegal data length, connection refused.");
            goto RefuseConnect;
        }

        ClientConnectInformation = reinterpret_cast<PLPC_CLIENT_CONNECT_INFORMATION>(reinterpret_cast<PUCHAR>(LpcMessageBuffer) + sizeof(PORT_MESSAGE));

        if (!(ClientConnectInformation->ClientType < LPC_CLIENT_TYPE::LPC_CLIENT_TYPE_MAX)) {
            LOG_ERROR("Illegal client type, connection refused.");
            goto RefuseConnect;
        }

        if (LpcManager::ClientRecord.SearchEntity([&](LpcClientRecord *Record) -> BOOLEAN { return Record->UniqueProcess == LpcMessageBuffer->ClientId.UniqueProcess; })) {
            LOG_ERROR("This process has already created a connection, connection refused.");
            goto RefuseConnect;
        }

        LOG_DEBUG("There are currently no connected control ports, ready to accept connections.");
        if ((CreatedClient = LpcManager::ClientRecord.AllocEntity()) != nullptr) {
            if (NT_SUCCESS(Status = LpcManager::_NtAlpcAcceptConnectPort(&CreatedClient->ClientPortHandle, LpcManager::LpcPortHandle, 0, NULL, NULL, NULL, LpcMessageBuffer, NULL, TRUE))) {

                CreatedClient->UniqueProcess = LpcMessageBuffer->ClientId.UniqueProcess;
                CreatedClient->Type          = ClientConnectInformation->ClientType;

                LpcManager::ClientRecord.PushEntity(CreatedClient);

                LOG_DEBUG("Successfully accepted the client's connection request.");
                LOG_DEBUG("Client port handle = %08X, Client type = %s.", CreatedClient->ClientPortHandle, CreatedClient->Type == LPC_CLIENT_TYPE::CONTROL ? "Control Client" : "Protected Client");

                return TRUE;
            } else {
                LOG_ERROR("Unable to accept the client's connection request. The error code returned by the system is %08X.", Status);
                return FALSE;
            }
        } else {
            LOG_ERROR("Unable to allocate memory.");
            return FALSE;
        }

    RefuseConnect:
        if (NT_SUCCESS(Status = LpcManager::_NtAlpcAcceptConnectPort(&UseLessHandle, LpcManager::LpcPortHandle, 0, NULL, NULL, NULL, LpcMessageBuffer, NULL, FALSE))) {
            LpcManager::_NtAlpcDisconnectPort(UseLessHandle, 0);
            NtClose(UseLessHandle);
            LOG_DEBUG("The connection was successfully refused.");
            return TRUE;
        } else {
            LOG_ERROR("Unable to refuse the client's connection request. The error code returned by the system is %08X.", Status);
            return FALSE;
        }
    }

    BOOL LpcManager::LpcSendMessageAndWaitReply(LPC_CLIENT_TYPE ClientType, LPC_MESSAGE_TYPE LpcMessageType, PUCHAR Message, SIZE_T MessageSize, PUCHAR ReplyBuffer, SIZE_T ReplyBufferSize, PSIZE_T ReplySize) {
        PPORT_MESSAGE                  LpcSendMessageBuffer        = nullptr;
        PPORT_MESSAGE                  LpcReceiveMessageBuffer     = nullptr;
        SIZE_T                         LpcReceiveMessageBufferSize = PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH;
        PLPC_MESSAGE_INFORMATION       MessageInformation          = nullptr;
        PLPC_REPLY_MESSAGE_INFORMATION ReplyMessageInformation     = nullptr;
        PUCHAR                         MessageToBeSent             = nullptr;
        PUCHAR                         MessageReceived             = nullptr;
        NTSTATUS                       Status                      = STATUS_SUCCESS;
        BOOL                           Result                      = FALSE;

        if (MessageSize > PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(LPC_MESSAGE_INFORMATION)) {
            LOG_ERROR("The message exceeds the maximum size of the message allowed to be sent, and the sending is canceled.");
            return FALSE;
        }

        LpcManager::ClientRecord.VisitEntity([&](LpcClientRecord *Record) -> BOOLEAN {
            if (Record->Type == ClientType) {
                if ((LpcSendMessageBuffer = reinterpret_cast<PPORT_MESSAGE>(ExAllocatePoolZero(PagedPool, PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH, 'CABL'))) == nullptr) {
                    LOG_ERROR("Unable to allocate memory, cancel sending.");
                    goto Clean;
                }

                if ((LpcReceiveMessageBuffer = reinterpret_cast<PPORT_MESSAGE>(ExAllocatePoolZero(PagedPool, PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH, 'CABL'))) == nullptr) {
                    LOG_ERROR("Unable to allocate memory, cancel sending.");
                    goto Clean;
                }

                RtlZeroMemory(LpcSendMessageBuffer, PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH);
                RtlZeroMemory(LpcReceiveMessageBuffer, PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH);

                if (ReplyBuffer != nullptr && ReplyBufferSize > 0) {
                    RtlZeroMemory(ReplyBuffer, ReplyBufferSize);
                }

                LpcSendMessageBuffer->u1.s1.DataLength  = static_cast<CSHORT>(sizeof(LPC_MESSAGE_INFORMATION) + MessageSize);
                LpcSendMessageBuffer->u1.s1.TotalLength = static_cast<CSHORT>(sizeof(PORT_MESSAGE) + sizeof(LPC_MESSAGE_INFORMATION) + MessageSize);
                MessageInformation                      = reinterpret_cast<PLPC_MESSAGE_INFORMATION>((reinterpret_cast<PUCHAR>(LpcSendMessageBuffer) + sizeof(PORT_MESSAGE)));
                MessageToBeSent                         = reinterpret_cast<PUCHAR>((reinterpret_cast<PUCHAR>(LpcSendMessageBuffer) + sizeof(PORT_MESSAGE) + sizeof(LPC_MESSAGE_INFORMATION)));
                MessageInformation->MessageControl      = LPC_MESSAGE_CONTROL::REPLY_AS_CALLBACK;
                MessageInformation->MessageType         = LpcMessageType;

                if (MessageSize > 0) {
                    RtlCopyMemory(MessageToBeSent, Message, MessageSize);
                }

                if (NT_SUCCESS(Status = LpcManager::_NtAlpcSendWaitReceivePort(Record->ClientPortHandle, ALPC_MSGFLG_SYNC_REQUEST, LpcSendMessageBuffer, NULL, LpcReceiveMessageBuffer, &LpcReceiveMessageBufferSize, NULL, NULL))) {
                    LOG_DEBUG("Successfully received the information returned by the client, The length of the returned data is %04X. message category is %02X. Client process id is %d, Message id is %08X.", LpcReceiveMessageBuffer->u1.s1.DataLength, LpcReceiveMessageBuffer->u2.s2.Type, LpcReceiveMessageBuffer->ClientId.UniqueProcess, LpcReceiveMessageBuffer->MessageId);

                    if (LpcReceiveMessageBuffer->u1.s1.DataLength - sizeof(LPC_REPLY_MESSAGE_INFORMATION) <= ReplyBufferSize) {
                        ReplyMessageInformation = reinterpret_cast<PLPC_REPLY_MESSAGE_INFORMATION>(reinterpret_cast<PUCHAR>(LpcReceiveMessageBuffer) + sizeof(PORT_MESSAGE));

                        if (ReplyMessageInformation->ReplyControl == LPC_REPLY_CONTROL::PROCEED) {
                            MessageReceived = reinterpret_cast<PUCHAR>(reinterpret_cast<PUCHAR>(LpcReceiveMessageBuffer) + sizeof(PORT_MESSAGE) + sizeof(LPC_REPLY_MESSAGE_INFORMATION));
                            if (ReplySize != nullptr) {
                                *ReplySize = LpcReceiveMessageBuffer->u1.s1.DataLength - sizeof(LPC_REPLY_MESSAGE_INFORMATION);

                                if (ReplyBuffer != nullptr && *ReplySize > 0 && ReplyBufferSize > 0) {
                                    RtlCopyMemory(ReplyBuffer, MessageReceived, *ReplySize > ReplyBufferSize ? ReplyBufferSize : *ReplySize);
                                }
                            }
                            
                            Result = TRUE;

                            goto Clean;
                        }

                    } else {
                        LOG_ERROR("The returned data is too long and the buffer provided cannot be saved. Available length: %08X, required length: %08X.", ReplyBufferSize, LpcReceiveMessageBuffer->u1.s1.DataLength);
                    }
                } else {
                    LOG_ERROR("Unable to receive Callback messages from the client. The error code returned by the system is %08X.", Status);
                }

            Clean:
                if (LpcSendMessageBuffer != nullptr) {
                    ExFreePoolWithTag(LpcSendMessageBuffer, 'CABL');
                }

                if (LpcReceiveMessageBuffer != nullptr) {
                    ExFreePoolWithTag(LpcReceiveMessageBuffer, 'CABL');
                }

                if (Result) {
                    return FALSE;
                } else {
                    return TRUE;
                }
            }

            return TRUE;
        });

        return Result;
    }

    VOID LpcManager::LpcSendMessage(LPC_CLIENT_TYPE ClientType, LPC_MESSAGE_TYPE LpcMessageType, PUCHAR Message, SIZE_T MessageSize) {
        PPORT_MESSAGE            LpcSendMessageBuffer = nullptr;
        PLPC_MESSAGE_INFORMATION MessageInformation   = nullptr;
        PUCHAR                   MessageToBeSent      = nullptr;
        NTSTATUS                 Status               = STATUS_SUCCESS;

        if (MessageSize > PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(LPC_MESSAGE_INFORMATION)) {
            LOG_ERROR("The message exceeds the maximum size of the message allowed to be sent, and the sending is canceled.");
            return;
        }

        LpcManager::ClientRecord.VisitEntity([&](LpcClientRecord *Record) -> BOOLEAN {
            if (Record->Type == ClientType) {
                if ((LpcSendMessageBuffer = reinterpret_cast<PPORT_MESSAGE>(ExAllocatePoolZero(PagedPool, PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH, 'CABL'))) == nullptr) {
                    LOG_ERROR("Unable to allocate memory, cancel sending.");
                    goto Clean;
                }

                RtlZeroMemory(LpcSendMessageBuffer, PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH);

                LpcSendMessageBuffer->u1.s1.DataLength  = static_cast<CSHORT>(MessageSize + sizeof(LPC_MESSAGE_INFORMATION));
                LpcSendMessageBuffer->u1.s1.TotalLength = static_cast<CSHORT>(sizeof(PORT_MESSAGE) + MessageSize + sizeof(LPC_MESSAGE_INFORMATION));
                MessageInformation                      = reinterpret_cast<PLPC_MESSAGE_INFORMATION>((reinterpret_cast<PUCHAR>(LpcSendMessageBuffer) + sizeof(PORT_MESSAGE)));
                MessageToBeSent                         = reinterpret_cast<PUCHAR>((reinterpret_cast<PUCHAR>(LpcSendMessageBuffer) + sizeof(PORT_MESSAGE) + sizeof(LPC_MESSAGE_INFORMATION)));
                MessageInformation->MessageControl      = LPC_MESSAGE_CONTROL::IGNORE_REPLY;
                MessageInformation->MessageType         = LpcMessageType;

                if (MessageSize) {
                    RtlCopyMemory(MessageToBeSent, Message, MessageSize);
                }

                if (NT_SUCCESS(Status = LpcManager::_NtAlpcSendWaitReceivePort(Record->ClientPortHandle, NULL, LpcSendMessageBuffer, NULL, NULL, NULL, NULL, NULL))) {
                    LOG_DEBUG("Successfully sent data to the other party.");
                } else {
                    LOG_ERROR("Unable to receive Callback messages from the client. The error code returned by the system is %08X.", Status);
                }

            Clean:
                if (LpcSendMessageBuffer != nullptr) {
                    ExFreePoolWithTag(LpcSendMessageBuffer, 'CABL');
                }
            }

            return TRUE;
        });
    }

    KQueue<LpcClientRecord>                       LpcManager::ClientRecord                   = KQueue<LpcClientRecord>();
    BOOLEAN                                       LpcManager::StopLpcServer                  = FALSE;
    LpcManager::NtAlpcCreatePortType              LpcManager::_NtAlpcCreatePort              = nullptr;
    LpcManager::NtAlpcSendWaitReceivePortType     LpcManager::_NtAlpcSendWaitReceivePort     = nullptr;
    LpcManager::NtAlpcAcceptConnectPortType       LpcManager::_NtAlpcAcceptConnectPort       = nullptr;
    LpcManager::NtAlpcDisconnectPortType          LpcManager::_NtAlpcDisconnectPort          = nullptr;
    LpcManager::NtAlpcQueryInformationMessageType LpcManager::_NtAlpcQueryInformationMessage = nullptr;
    HANDLE                                        LpcManager::LpcPortHandle                  = nullptr;
    HANDLE                                        LpcManager::LpcServerThreadHandle          = nullptr;
} // namespace Binklac::Kernel::Lpc