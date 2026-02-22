#pragma once

#include "IPropertyTypeCustomization.h"

class FProperty;
class IPropertyHandle;
class UScriptStruct;

/**
 * Generic property customization applied to inventory fragment structs.
 * Adds richer field tooltips and clickable field labels that navigate to native source.
 */
class YOLOINVENTORYEDITORSCHEMA_API FYIFragmentStructCustomization final : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	static TSharedRef<SWidget> MakeClickableStructLabelWidget(TSharedRef<IPropertyHandle> StructPropertyHandle);
	static TSharedRef<SWidget> MakeClickableFieldLabelWidget(TSharedRef<IPropertyHandle> PropertyHandle);
	static FText BuildStructTooltip(const UScriptStruct* StructType);
	static FText BuildFieldTooltip(const FProperty* Property);
	static FText GetReadableFieldLabel(TSharedRef<IPropertyHandle> PropertyHandle);
	static FReply NavigateToPropertySource(const FProperty* Property);
	static FReply NavigateToStructSource(const UScriptStruct* StructType);
};

