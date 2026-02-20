#include "YIEditorMessageLog.h"

TArray<FYIEditorLogEntry> FYIEditorMessageLog::Entries;
FYIEditorMessageLog::FOnLogChanged FYIEditorMessageLog::LogChanged;

void FYIEditorMessageLog::Add(EYIEditorLogSeverity Severity, const FText& Message, const FText& Context)
{
	FYIEditorLogEntry Entry;
	Entry.TimeUtc = FDateTime::UtcNow();
	Entry.Severity = Severity;
	Entry.Message = Message;
	Entry.Context = Context;
	Entries.Add(MoveTemp(Entry));
	LogChanged.Broadcast();
}

void FYIEditorMessageLog::Clear()
{
	Entries.Reset();
	LogChanged.Broadcast();
}

const TArray<FYIEditorLogEntry>& FYIEditorMessageLog::GetEntries()
{
	return Entries;
}

FYIEditorMessageLog::FOnLogChanged& FYIEditorMessageLog::OnLogChanged()
{
	return LogChanged;
}
