#pragma once
#include "CoreMinimal.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

inline void YI_ShowToast(const FText& Msg, SNotificationItem::ECompletionState State = SNotificationItem::CS_None)
{
	FNotificationInfo Info(Msg);
	Info.bUseLargeFont = false;
	Info.FadeOutDuration = 1.5f;
	Info.ExpireDuration = 2.0f;
	auto Item = FSlateNotificationManager::Get().AddNotification(Info);
	if (Item.IsValid()) { Item->SetCompletionState(State); }
}
