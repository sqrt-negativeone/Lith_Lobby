/* date = July 17th 2022 10:56 pm */

#ifndef WIN32_TYPES_H
#define WIN32_TYPES_H

#if COMPILER_CL
#define AtomicCompareAndExchange(Address, ExpectedValue, ExchangeValue) \
(InterlockedCompareExchange(Address, ExchangeValue, ExpectedValue) == (ExpectedValue))
#define CompletePreviousWritesBeforeFutureWrites() MemoryBarrier(); __faststorefence()
#define AtomicIncrement(Address) InterlockedIncrement(Address)
#endif

#endif //WIN32_TYPES_H
