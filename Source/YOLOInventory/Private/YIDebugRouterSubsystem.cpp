#include "YIDebugRouterSubsystem.h"

void UYIDebugRouterSubsystem::GetBufferedMessages(TArray<FYIDebugMessageRecord>& OutMessages) const
{
	UYIDebugLibrary::GetDebugMessageHistory(OutMessages);
}

void UYIDebugRouterSubsystem::BroadcastDebugMessage(const FYIDebugMessageRecord& Message)
{
	OnDebugMessage.Broadcast(Message);
}
