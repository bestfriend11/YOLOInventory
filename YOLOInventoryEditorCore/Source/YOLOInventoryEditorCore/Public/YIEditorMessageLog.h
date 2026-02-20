#pragma once

#include "CoreMinimal.h"

enum class EYIEditorLogSeverity : uint8
{
	Info,
	Warning,
	Error
};

struct FYIEditorLogEntry
{
	FDateTime TimeUtc;
	EYIEditorLogSeverity Severity = EYIEditorLogSeverity::Info;
	FText Message;
	FText Context;
};

class YOLOINVENTORYEDITORCORE_API FYIEditorMessageLog
{
public:
	DECLARE_MULTICAST_DELEGATE(FOnLogChanged);

	static void Add(EYIEditorLogSeverity Severity, const FText& Message, const FText& Context = FText());
	static void Clear();
	static const TArray<FYIEditorLogEntry>& GetEntries();
	static FOnLogChanged& OnLogChanged();

private:
	static TArray<FYIEditorLogEntry> Entries;
	static FOnLogChanged LogChanged;
};
