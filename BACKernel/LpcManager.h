#pragma once

#include "GlobalDeclaration.h"
#include "NtLpcApi.h"
#include "LpcMessageDefine.h"
#include "KQueue.h"

namespace Binklac::Kernel::Lpc {
    struct LpcClientRecord {
        LPC_CLIENT_TYPE Type             = LPC_CLIENT_TYPE::LPC_CLIENT_TYPE_MAX;
        HANDLE          UniqueProcess    = nullptr;
        HANDLE          ClientPortHandle = nullptr;
    };

    class LpcManager {
      private:
        typedef NTSTATUS(NTAPI *NtAlpcCreatePortType)(PHANDLE, POBJECT_ATTRIBUTES, PALPC_PORT_ATTRIBUTES);
        typedef NTSTATUS(NTAPI *NtAlpcSendWaitReceivePortType)(HANDLE, ULONG, PPORT_MESSAGE, PALPC_MESSAGE_ATTRIBUTES, PPORT_MESSAGE, PSIZE_T, PALPC_MESSAGE_ATTRIBUTES, PLARGE_INTEGER);
        typedef NTSTATUS(NTAPI *NtAlpcAcceptConnectPortType)(PHANDLE, HANDLE, ULONG, POBJECT_ATTRIBUTES, PALPC_PORT_ATTRIBUTES, PVOID, PPORT_MESSAGE, PALPC_MESSAGE_ATTRIBUTES, BOOLEAN);
        typedef NTSTATUS(NTAPI *NtAlpcDisconnectPortType)(HANDLE, ULONG);
        typedef NTSTATUS(NTAPI *NtAlpcQueryInformationMessageType)(HANDLE, PPORT_MESSAGE, ALPC_MESSAGE_INFORMATION_CLASS, PVOID, ULONG, PULONG);

      private:
        static NtAlpcCreatePortType              _NtAlpcCreatePort;
        static NtAlpcSendWaitReceivePortType     _NtAlpcSendWaitReceivePort;
        static NtAlpcAcceptConnectPortType       _NtAlpcAcceptConnectPort;
        static NtAlpcDisconnectPortType          _NtAlpcDisconnectPort;
        static NtAlpcQueryInformationMessageType _NtAlpcQueryInformationMessage;

      private:
        static KQueue<LpcClientRecord> ClientRecord;

      private:
        static BOOLEAN StopLpcServer;

      private:
        static HANDLE LpcPortHandle;
        static HANDLE LpcServerThreadHandle;

      private:
        static VOID LpcServerThreadRoutine(PVOID StartContext);

      public:
        static BOOLEAN Initialization();
        static BOOLEAN CreateLpcPort();
        static BOOLEAN StartLpcServerThread();
        static VOID    CloseLpcPort();

      private:
        static BOOL HandleConnectRequest(PPORT_MESSAGE LpcMessageBuffer);

      public:
        static BOOL HandleProcessClose(HANDLE UniqueProcess);

      public:
        static BOOL LpcSendMessageAndWaitReply(LPC_CLIENT_TYPE ClientType, LPC_MESSAGE_TYPE LpcMessageType, PUCHAR Message, SIZE_T MessageSize, PUCHAR ReplyBuffer, SIZE_T ReplyBufferSize, PSIZE_T ReplySize);
        static VOID LpcSendMessage(LPC_CLIENT_TYPE ClientType, LPC_MESSAGE_TYPE LpcMessageType, PUCHAR Message, SIZE_T MessageSize);
    };
} // namespace Binklac::Kernel::Lpc
