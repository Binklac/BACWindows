#pragma once

#include "../ThirdParty/phnt/phnt_windows.h"
#include "../ThirdParty/phnt/phnt.h"
#include "../BACKernel/LpcMessageDefine.h"

class LpcClient {
  public:
    static BOOL StopLpcClient;

    public:
    static VOID DispathLpcMessage(LPC_MESSAGE_TYPE MessageType, PBYTE MessageBuffer, SIZE_T MessageLength, PBYTE ResponseBuffer, PSIZE_T ResponseLength);

  public:
    static BOOL ConnectToLpcControlPort(LPC_CLIENT_TYPE ClientType);
};
