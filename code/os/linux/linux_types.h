/* date = June 21st 2022 10:28 am */

#ifndef LINUX_TYPES_H
#define LINUX_TYPES_H


#if COMPILER_GCC
#define AtomicCompareAndExchange(Address, ExpectedValue, ExchangeValue) __atomic_compare_exchange_n (Address, &ExpectedValue, ExchangeValue, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define CompletePreviousWritesBeforeFutureWrites() __atomic_thread_fence (__ATOMIC_SEQ_CST)
#define AtomicIncrement(Address) __atomic_fetch_add (Address, 1, __ATOMIC_SEQ_CST)
#endif

#endif //LINUX_TYPES_H
