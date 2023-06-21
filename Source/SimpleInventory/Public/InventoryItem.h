// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "InventoryItem.generated.h"


//General Data table stuff
USTRUCT(BlueprintType, Blueprintable)
struct FAddItemStatus
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	bool addStatus = false;
	UPROPERTY(BlueprintReadOnly)
	int leftOvers = 0;
};

USTRUCT(BlueprintType, Blueprintable)
struct FInvTableItem : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UItemAsset* item;
};
//

//Base item asset used for most things
UCLASS(BlueprintType)
class SIMPLEINVENTORY_API UItemAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName uniqueID = "-1";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName name = "Empty";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString description = "Empty";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName type = "None";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* icon = nullptr;
};


USTRUCT(BlueprintType, Blueprintable)
struct FInvItem
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UItemAsset* item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "99", UIMin = "0", UIMax = "99"))
	int32 quantity;

	FInvItem()
	{
		item = nullptr;
		quantity = 0;
	}

	bool operator==(const FInvItem& other) const 
	{

		if (item->uniqueID == other.item->uniqueID) { return true; }
		else { return false; }
	}
};
//