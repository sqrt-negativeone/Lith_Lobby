
internal os_data_time
OS_GetCurrentDateTime()
{
#if defined(IMPL_GetCurrentDateTime)
    os_data_time Result = IMPL_GetCurrentDateTime();
#endif
    return Result;
}

internal os_time
OS_GetCurrentTime()
{
#if defined(IMPL_GetCurrentTime)
    os_time Result = IMPL_GetCurrentTime();
#endif
    return Result;
}

internal os_data_time
OS_GetDateTimeFromTime(os_time Time)
{
#if defined(IMPL_GetDateTimeFromTime)
    os_data_time Result = IMPL_GetDateTimeFromTime(Time);
#endif
    return Result;
}

internal os_time
OS_GetTimeFromDateTime(os_data_time DateTime)
{
#if defined(IMPL_GetTimeFromDateTime)
    os_time Result = IMPL_GetTimeFromDateTime(DateTime);
#endif
    return Result;
}

internal os_time
OS_GetCurrentTimeMicroSeconds()
{
#if defined(IMPL_GetCurrentTimeMicroSeconds)
    os_time Result = IMPL_GetCurrentTimeMicroSeconds();
#endif
    return Result;
}