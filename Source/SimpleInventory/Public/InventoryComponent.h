// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryItem.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInvChangedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRowsAddedDelegate);

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SIMPLEINVENTORY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1", UMin = "1", ToolTip = "The number of rows to put in this inventory, rows are 5 columns each."))
	int inventoryRows = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1", UMin = "1", ToolTip = "The Max number of rows that can be added to this inventory."))
	int maxInventoryRows = 5;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1", UMin = "1", ToolTip = "The number of slots per row."))
	int slotsPerRow = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "2000", UMin = "0", UMax = "2000", ToolTip = "The range to add to a lootbag instead of creating a new one."))
	float mergeDist = 500;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AActor> lootBag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Tooltip ="The item needed to upgrade this inventory(must be in this specific inventory to use)"))
	UItemAsset* upgradeItem = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Tooltip = "Amount of the upgrade item needed"))
	int amtToUpgrade = 1;

	UItemAsset* findItemAssetByID(int uniqueID);

public:	
	UPROPERTY(BlueprintAssignable, Category = "Delegates")
	FOnInvChangedDelegate OnInvChanged;
	UPROPERTY(BlueprintAssignable, Category = "Delegates")
	FOnRowsAddedDelegate OnRowsAddedd;


	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FInvItem> inventoryArray;

	UFUNCTION(BlueprintCallable, meta=(ToolTip = "Try to add new rows up the max set"))
	bool addNewRows(int numRows = 1, bool ignoreUpgradeItem = false);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Adds as much from the stack of new item to the inventory as possible and drops the rest"))
	FAddItemStatus addNewItem(const FInvItem& newItem, bool dropIfNoneAdded = false, bool dropIfPartialAdded = true);

	UFUNCTION(BlueprintCallable, meta = (Tooltip ="Try to add an item at a specific spot, DOES NOT CHECK IF SLOT IS EMPTY FIRST"))
	void addItemAtSlot(const FInvItem& newItem, int slot);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Move item from one slot to another"))
	void moveItem(int from, int to);
		
	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Remove Item from inventory"))
	void removeItem(int slot, bool bShouldDrop = true);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Move from one inventory comp to another"))
	bool moveToNewInvComp(int slot, UInventoryComponent* newComp);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Get the amount of an item in this inventory"))
	int getItemQuantity(int uniqueID);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Find the item in a specific spot"))
	FInvItem getItemAtSlot(int slot);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Check if an item of a specific type exists in the inventory"))
	bool itemTypeExists(const FName typeToSearchFor);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Find the next item of a specific type starting from startPos ind"))
	int findNextItemOfType(int startPos, int direction, const FName itemType);

	UFUNCTION()
	int changeQuantity(int uniqueID, int quantityToChange);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Split a stack into multiple slots"))
	bool splitStack(int slot, int newStackSize);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Drop item on the ground and put it in a loot bag"))
	void createLootBag(const FInvItem& itemToDrop, int slot = -1);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Check if the inventory has any items"))
	bool isEmpty();

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Get a copy of the inventory"))
	const TArray<FInvItem> getInventory() { return inventoryArray; };

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Get current amount of rows"))
	int getRows();

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Load inventory from array"))
	void loadInventory(TArray<FInvItem> newInv);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Get amount of upgrade item needed to upgrage"))
	int getAmtToUpgrade() { return amtToUpgrade; }

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Get lootbag class"))
	TSubclassOf<class AActor> getLootBagClass() { return lootBag; } 

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Get free slots in bag"))
	int getAmountOfEmptySlots() { return getItemQuantity(-1); }

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Get slots per row"))
	int getSlotsPerRow() { return slotsPerRow; }
};
