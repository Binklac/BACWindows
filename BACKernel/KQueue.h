#pragma once

#include "GlobalDeclaration.h"

template<typename T>
class KQueue {
  private:
    typedef struct _KQE {
        struct _KQE *NextEntry;
        T            DataEntry;
    } KQE, *PKQE;

  private:
    LOOKASIDE_LIST_EX LookasideList = {};
    KQE               Queue         = {};

  private:
    BOOLEAN IsQueueFinishedInitize = FALSE;

  public:
    VOID KQueueInit() {
        if (NT_SUCCESS(ExInitializeLookasideListEx(&LookasideList, NULL, NULL, NonPagedPool, EX_LOOKASIDE_LIST_EX_FLAGS_RAISE_ON_FAIL, sizeof(KQE), 'MCAB', 0))) {
            Queue.NextEntry        = nullptr;
            IsQueueFinishedInitize = TRUE;
        }
    }

    VOID KQueueClear() {
        if (IsQueueFinishedInitize) {
            ExDeleteLookasideListEx(&LookasideList);
        }
    }

  public:
    T *AllocEntity() {
        if (IsQueueFinishedInitize) {
            auto entity = reinterpret_cast<PKQE>(ExAllocateFromLookasideListEx(&LookasideList));
            if (entity != nullptr) {
                RtlZeroMemory(entity, sizeof(KQE));
                return &entity->DataEntry;
            }
        }

        return nullptr;
    }

    VOID PushEntity(T *entity) {
        if (IsQueueFinishedInitize && MmIsAddressValid(entity)) {
            PKQE listEntity      = reinterpret_cast<PKQE>(CONTAINING_RECORD(entity, KQE, DataEntry));
            PKQE savedFirstEntry = nullptr;

            __try {
                do {
                    savedFirstEntry = Queue.NextEntry;
                } while (InterlockedCompareExchangePointer(reinterpret_cast<PVOID *>(&Queue.NextEntry), listEntity, savedFirstEntry) != savedFirstEntry);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return;
            }
        }
    }

    T *RemoveHeadEntity() {
        PKQE savedFirstEntry  = nullptr;
        PKQE savedSecondEntry = nullptr;

        if (IsQueueFinishedInitize && Queue.NextEntry != nullptr) {
            __try {
                do {
                    savedFirstEntry  = Queue.NextEntry;
                    savedSecondEntry = Queue.NextEntry->NextEntry;
                } while (InterlockedCompareExchangePointer(reinterpret_cast<PVOID *>(&Queue.NextEntry), savedSecondEntry, savedFirstEntry) != savedFirstEntry);

                if (MmIsAddressValid(savedFirstEntry)) {
                    savedFirstEntry->NextEntry = nullptr;
                    return &savedFirstEntry->DataEntry;
                } else {
                    return nullptr;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return nullptr;
            }
        }

        return nullptr;
    }

    T *NextEntity(T *entity) {
        PKQE listEntity = nullptr;

        if (IsQueueFinishedInitize) {
            __try {
                if (!MmIsAddressValid(entity)) {
                    return nullptr;
                }

                listEntity = reinterpret_cast<PKQE>(CONTAINING_RECORD(entity, KQE, DataEntry));

                if (!MmIsAddressValid(listEntity)) {
                    return nullptr;
                }

                return &reinterpret_cast<PKQE>(listEntity->NextEntry)->DataEntry;

            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return nullptr;
            }
        }
    }

    VOID RemoveEntity(T *entity) {
        PKQE listEntity  = nullptr;
        PKQE aheadEntity = nullptr;
        PKQE nextEntry   = nullptr;

        if (IsQueueFinishedInitize) {
            __try {
                if (!MmIsAddressValid(entity)) {
                    return;
                }

                listEntity = reinterpret_cast<PKQE>(CONTAINING_RECORD(entity, KQE, DataEntry));

                if (!MmIsAddressValid(listEntity)) {
                    return;
                }

                do {
                    aheadEntity = Queue.NextEntry;

                    while (aheadEntity->NextEntry != nullptr && aheadEntity->NextEntry != listEntity) {
                        aheadEntity = aheadEntity->NextEntry;
                    }

                    if (aheadEntity->NextEntry == nullptr) {
                        return;
                    }

                    nextEntry = listEntity->NextEntry;

                } while (InterlockedCompareExchangePointer(reinterpret_cast<PVOID *>(&aheadEntity->NextEntry), nextEntry, listEntity) != listEntity);

                ExFreeToLookasideListEx(&LookasideList, listEntity);

            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return;
            }
        }
    }

    template<typename F>
    VOID RemoveEntity(F const &comparator) {
        PKQE entry       = nullptr;
        PKQE nextEntry   = nullptr;
        PKQE aheadEntity = nullptr;

        __try {

            if (IsQueueFinishedInitize) {
                entry = reinterpret_cast<PKQE>(Queue.NextEntry);

                while (reinterpret_cast<PKQE>(entry) != nullptr) {
                    if (MmIsAddressValid(entry) && comparator(&entry->DataEntry)) {

                        do {
                            aheadEntity = Queue.NextEntry;

                            while (aheadEntity->NextEntry != nullptr && aheadEntity->NextEntry != entry) {
                                aheadEntity = aheadEntity->NextEntry;
                            }

                            if (aheadEntity->NextEntry == nullptr) {
                                return;
                            }

                            nextEntry = entry->NextEntry;

                        } while (InterlockedCompareExchangePointer(reinterpret_cast<PVOID *>(&aheadEntity->NextEntry), nextEntry, entry) != entry);

                        ExFreeToLookasideListEx(&LookasideList, entry);

                        entry = reinterpret_cast<PKQE>(nextEntry);
                    } else {
                        entry = reinterpret_cast<PKQE>(entry->NextEntry);
                    }
                }
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return;
        }
    }

    template<typename F>
    BOOLEAN SearchEntity(F const &visitor) {
        PKQE    entry  = nullptr;
        BOOLEAN result = FALSE;

        if (IsQueueFinishedInitize) {
            __try {
                entry = reinterpret_cast<PKQE>(Queue.NextEntry);

                while (reinterpret_cast<PKQE>(entry) != nullptr) {
                    if (visitor(&entry->DataEntry)) {
                        result = TRUE;
                        break;
                    }

                    entry = reinterpret_cast<PKQE>(entry->NextEntry);
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return FALSE;
            }
        }

        return result;
    }

    template<typename F>
    VOID VisitEntity(F const &visitor) {

        PKQE entry = nullptr;

        if (IsQueueFinishedInitize) {
            __try {
                entry = reinterpret_cast<PKQE>(Queue.NextEntry);

                while (reinterpret_cast<PKQE>(entry) != nullptr) {
                    if (!visitor(&entry->DataEntry)) {
                        break;
                    }
                    entry = reinterpret_cast<PKQE>(entry->NextEntry);
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return;
            }
        }
    }
};