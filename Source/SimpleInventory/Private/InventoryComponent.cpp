// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"
#include "Kismet/KismetMathLibrary.h" 

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	inventoryArray = TArray<FInvItem>();

	/*Put a reference to a backup upgrade item here
	if (upgradeItem == nullptr)
	{
		FSoftObjectPath itemAssetPath("REFERNCE TO BACKUP UPGRADE ITEM");
		UObject* itemObject = itemAssetPath.TryLoad();
		UItemAsset* itemAsset = Cast<UItemAsset>(itemObject);
		if (IsValid(itemAsset))
		{
			upgradeItem = itemAsset;
		}
	}
	*/
}

// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	addNewRows(inventoryRows, true);
}

//Adds another row of empty slots to the inventory
bool UInventoryComponent::addNewRows(int numRows, bool ignoreUpgradeItem)
{
	if (inventoryArray.Num() / slotsPerRow == maxInventoryRows)
	{
		return false;
	}
	else if (!ignoreUpgradeItem && IsValid(upgradeItem))
	{
		int itemAmt = getItemQuantity(upgradeItem->uniqueID);
	
		if(itemAmt >= amtToUpgrade)
		{
			changeQuantity(upgradeItem->uniqueID, -amtToUpgrade);
		}
		else
		{
			return false;
		}
	}

	for (int i = 0; i < (numRows * slotsPerRow); ++i)
	{
		if (inventoryArray.Num() / slotsPerRow == maxInventoryRows)
		{
			break;
		}

		FInvItem newEmptyItem = FInvItem();
		newEmptyItem.quantity = 0;
		newEmptyItem.item = nullptr;
		inventoryArray.Add(newEmptyItem);
	}

	for (int i = 0; i < numRows; ++i)
	{
		OnRowsAddedd.Broadcast();
	}
	
	OnInvChanged.Broadcast();

	return true;
}

//Adds to new slot if there is none in the inventory already, otherwise adds to stack
FAddItemStatus UInventoryComponent::addNewItem(const FInvItem& newItem, bool dropIfFull, bool dropIfPartialAdded)
{
	UItemAsset* itemAsset = newItem.item;
	FAddItemStatus statusReturn;

	if (!IsValid(itemAsset))
	{
		statusReturn.addStatus = false;
		statusReturn.leftOvers = 0;
		return statusReturn;
	}

	int quant = getItemQuantity(itemAsset->uniqueID);
	if (quant == 0 && newItem.quantity <= 0)
	{
		statusReturn.addStatus = false;
		statusReturn.leftOvers = 0;
		return statusReturn;
	}
	else if (quant == 0 || quant % newItem.item->maxStackSize == 0)
	{
		//Put it in the first empty position
		for (int i = 0; i < inventoryArray.Num(); ++i)
		{
			UItemAsset* curItem = inventoryArray[i].item;
			if (curItem == nullptr)
			{
				inventoryArray[i] = newItem;
				OnInvChanged.Broadcast();
				statusReturn.addStatus = true;
				statusReturn.leftOvers = 0;
				return statusReturn;
			}
			else if (i == inventoryArray.Num() - 1)
			{
				//No room in inventory so create a lootbag if it was request thend return as failed
				statusReturn.addStatus = false;

				if (dropIfFull)
				{
					createLootBag(newItem);
				}
			}
		}

		return statusReturn;
	}
	else
	{
		int leftOvers = changeQuantity(itemAsset->uniqueID, newItem.quantity);

		if (leftOvers == newItem.quantity)//None was able to be added
		{
			statusReturn.addStatus = false;
			statusReturn.leftOvers = newItem.quantity;

			if (dropIfFull)
			{
				createLootBag(newItem);
			}

			return statusReturn;
		}
		else if (leftOvers > 0)//Some was able to be added
		{
			FInvItem itemToDrop = newItem;
			itemToDrop.quantity = leftOvers;

			if (dropIfPartialAdded)
			{
				createLootBag(itemToDrop);
			}
		}

		statusReturn.addStatus = true;
		statusReturn.leftOvers = leftOvers;
		return statusReturn;
	}
}

//Assume its only called for empty slots, currently only used from drag and drop UI
void UInventoryComponent::addItemAtSlot(const FInvItem& newItem, int slot)
{
	if (slot < inventoryArray.Num())
	{
		inventoryArray[slot] = newItem;
		OnInvChanged.Broadcast();
	}
}


void UInventoryComponent::moveItem(int from, int to)
{
	if ((from < 0 || from >= inventoryArray.Num()) || (to < 0 || to > inventoryArray.Num()))
	{
		return;
	}

	FInvItem prevItem = inventoryArray[to];
	UItemAsset* prevItemAsset = prevItem.item;

	UItemAsset* toItemAsset = inventoryArray[from].item;

	//If items are the same item combine stacks if possible
	if (prevItemAsset == nullptr)
	{
		inventoryArray[to] = inventoryArray[from];
		inventoryArray[from] = prevItem;
	}
	else if (IsValid(prevItemAsset) && prevItemAsset->uniqueID == toItemAsset->uniqueID)
	{
		//If combined they are a full stack or less combine into one stack, otherwise move a quantity
		if (prevItem.quantity + inventoryArray[from].quantity <= prevItemAsset->maxStackSize)
		{
			inventoryArray[to].quantity += inventoryArray[from].quantity;
			removeItem(from, false);
		}
		else
		{
			int amountToLeave = (prevItem.quantity + inventoryArray[from].quantity ) - prevItemAsset->maxStackSize;
			inventoryArray[to].quantity = prevItemAsset->maxStackSize;
			inventoryArray[from].quantity = amountToLeave;
		}
	}
	else
	{
		inventoryArray[to] = inventoryArray[from];
		inventoryArray[from] = prevItem;
	}

	OnInvChanged.Broadcast();
}

void UInventoryComponent::removeItem(int slot, bool bShouldDrop)
{
	if (slot < 0 || slot >= inventoryArray.Num())
	{
		return;
	}

	FInvItem newEmptyItem = FInvItem();
	newEmptyItem.quantity = 0;
	newEmptyItem.item = nullptr;

	if (bShouldDrop)
	{
		createLootBag(inventoryArray[slot]);
	}

	inventoryArray[slot] = newEmptyItem;
	OnInvChanged.Broadcast();
}

int UInventoryComponent::getItemQuantity(int uniqueID)
{
	int curAmt = 0;
	for(int i = 0; i < inventoryArray.Num(); ++i)
	{
		UItemAsset* curItemAsset = inventoryArray[i].item;

		if (IsValid(curItemAsset))
		{
			if (curItemAsset->uniqueID == uniqueID)
			{
				curAmt += inventoryArray[i].quantity;
			}
		}
		else if (uniqueID == -1)
		{
			curAmt += 1;
		}
	}
	return curAmt;
}

FInvItem UInventoryComponent::getItemAtSlot(int slot)
{
	if(slot >= inventoryArray.Num() || slot < 0)
		return FInvItem();
	else
		return inventoryArray[slot];
}


//Crashes if called from C++ for some reason
//-1 returned means no item of type found
int UInventoryComponent::findNextItemOfType(int startPos, int direction, const FName type)
{
	//-2 is a arbitrary number just used to get the first item of found of this type
	if((startPos < 0 && startPos != -2) || startPos >= inventoryArray.Num())
		return -1;

	bool looped = false;

	for (int i = startPos; i <= inventoryArray.Num(); i = i + (1 * direction))
	{
		if (i == -2)
		{
			i = -1;
			continue;
		}
		if (i == -1 && !looped)
		{
			i = inventoryArray.Num();
			looped = true;
			continue;
		}
		else if (i == inventoryArray.Num() && !looped)
		{
			i = -1;
			looped = true;
			continue;
		}
		else if (i == startPos && looped)
		{
			return -1;
		}
		else if (startPos == -2 && i == inventoryArray.Num() && looped)
		{
			return -1;
		}

		UItemAsset* curItem = inventoryArray[i].item;

		if (curItem->type == type && i != startPos)
		{
			return i;
		}
	
	}

	return -1;
}

//Simple check to see if ANY of the item type is in the inventory
bool UInventoryComponent::itemTypeExists(const FName typeToSearchFor)
{
	for (int i = 0; i < inventoryArray.Num(); ++i)
	{
		UItemAsset* curItem = inventoryArray[i].item;

		if(curItem->type == typeToSearchFor)
			return true;
	}
	return false;
}

UItemAsset* UInventoryComponent::findItemAssetByID(int uniqueID)
{
	for (FInvItem curItem : inventoryArray)
	{
		if (IsValid(curItem.item) && curItem.item->uniqueID == uniqueID)
		{
			return curItem.item;
		}
	}

	return nullptr;
}

//Cannot add or remove MORE than max stack at one time
//When removing assume this is ONLY called if there is enough to remove
//When adding there can be leftovers to create new stack
//Returns leftovers in the case of a full inventory 
//Return -1 means there is no item of this type in inventory
//Return -2 means you tried to change more than max stack at one time
int UInventoryComponent::changeQuantity(int uniqueID, int quantityToChange)
{
	UItemAsset* itemToChange = findItemAssetByID(uniqueID);

	if(!IsValid(itemToChange) || FMath::Abs(quantityToChange) > itemToChange->maxStackSize)
		return -2;
	
	//Checks for existing stacks to edit first
	int amountLeftToChange = quantityToChange;

	for(int i = 0; i < inventoryArray.Num(); ++i)
	{
		UItemAsset* curItem = inventoryArray[i].item;

		if (IsValid(curItem) && curItem->uniqueID == uniqueID)
		{
			int curAmt = inventoryArray[i].quantity;

			if (curAmt + amountLeftToChange == 0)
			{
				removeItem(i, false);
				OnInvChanged.Broadcast();
				return 0;
			}
			else if (curAmt + amountLeftToChange <= 0)
			{
				amountLeftToChange = amountLeftToChange - inventoryArray[i].quantity;
				removeItem(i, false);
				OnInvChanged.Broadcast();
				i = 0;
				return 0;
			}
			else if(UKismetMathLibrary::SignOfInteger(amountLeftToChange) > 0 && curAmt + amountLeftToChange > itemToChange->maxStackSize && curAmt != itemToChange->maxStackSize)
			{
				amountLeftToChange = amountLeftToChange - (itemToChange->maxStackSize - inventoryArray[i].quantity);
				inventoryArray[i].quantity = itemToChange->maxStackSize;
				OnInvChanged.Broadcast();
			}
			else if (UKismetMathLibrary::SignOfInteger(amountLeftToChange) < 0)
			{
				inventoryArray[i].quantity += amountLeftToChange;
				OnInvChanged.Broadcast();
				return 0;
			}
			else if(inventoryArray[i].quantity != itemToChange->maxStackSize)
			{
				inventoryArray[i].quantity += amountLeftToChange;
				OnInvChanged.Broadcast();
				return 0;
			}
		}
	}

	//Leftovers remain after adding to existing stacks
	if (amountLeftToChange > 0)
	{
		FInvItem tempCopy = FInvItem();
		int emptySlot = -1;

		for(int i = 0; i < inventoryArray.Num(); ++i)
		{
			UItemAsset* curItem = inventoryArray[i].item;

			if (curItem == nullptr && emptySlot == -1)
			{
				emptySlot = i;
			}
			else if (IsValid(curItem) && curItem->uniqueID == uniqueID)
			{
				tempCopy = inventoryArray[i];
			}
		}

		UItemAsset* tempItem = tempCopy.item;

		if (emptySlot == -1)
		{
			return amountLeftToChange;
		}
		else if (tempItem == nullptr)
		{
			return amountLeftToChange;
		}
		else
		{
			inventoryArray[emptySlot] = tempCopy;
			inventoryArray[emptySlot].quantity = amountLeftToChange;
			OnInvChanged.Broadcast();
			return 0;
		}
	}
	//should never reach this
	return -3;
}

//Split position into two stacks
bool UInventoryComponent::splitStack(int slot, int newStackSize)
{
	if(slot < 0 || slot >= inventoryArray.Num() || newStackSize >= inventoryArray[slot].quantity)
		return false;

	//Find empty spot then split or display error if no slots
	for (int i = 0; i < inventoryArray.Num(); ++i)
	{
		UItemAsset* curItem = inventoryArray[i].item;

		if (curItem == nullptr)
		{
			inventoryArray[i] = inventoryArray[slot];
			inventoryArray[i].quantity = newStackSize;
			inventoryArray[slot].quantity = inventoryArray[slot].quantity - newStackSize;
			OnInvChanged.Broadcast();
			return true;
		}
	}

	//Display inventory full error
	return false;
}


//Move from one inventory to another for usage with chests
bool UInventoryComponent::moveToNewInvComp(int slot, UInventoryComponent* newComp)
{
	//Check if any stacks already exist and add to them if possible
	FAddItemStatus addStatus = newComp->addNewItem(inventoryArray[slot], false, false);
	if (!addStatus.addStatus)
	{
		return false;
	}
	else if(addStatus.leftOvers > 0)
	{
		inventoryArray[slot].quantity = addStatus.leftOvers;
		OnInvChanged.Broadcast();
		return true;
	}
	else
	{
		removeItem(slot, false);
		OnInvChanged.Broadcast();
		return true;
	}
}


//Function to allow the user to drop items on the ground or for say plants to request a loot bag dropped if the new amount would overflow
void UInventoryComponent::createLootBag(const FInvItem& itemToDrop, int slot)
{
	if(!IsValid(lootBag))
		return;

	//grab all lootbag actors
	//run through and remove all out of range
	//Try to add the item, without the ability to drop
	//if leftovers try to add to other lootbags nearby
	//or if no other lootbag make a new one
	FInvItem item = itemToDrop;

	TArray<AActor*> lootBags;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), lootBag, lootBags);

	if (lootBags.Num() > 0)
	{
		for (int i = 0; i < lootBags.Num(); ++i)
		{
			if (FVector::Distance(GetOwner()->GetActorLocation(), lootBags[i]->GetActorLocation()) > mergeDist)
			{
				lootBags.RemoveAt(i);
				i = 0;
			}
		}

		if (lootBags.Num() > 0)
		{
			for (int i = 0; i < lootBags.Num(); ++i)
			{
				UInventoryComponent* curInv = Cast<UInventoryComponent>(lootBags[i]->GetComponentByClass(UInventoryComponent::StaticClass()));

				if (IsValid(curInv))
				{
					FAddItemStatus newStatus = curInv->addNewItem(item, false, false);

					if (newStatus.leftOvers == 0)
					{
						removeItem(slot, false);
						return;
					}
					else
					{
						item.quantity = newStatus.leftOvers;
					}
				}
			}
		}
	}


	FVector spawnLoc = GetOwner()->GetActorLocation() + ( GetOwner()->GetActorForwardVector() * 200);
	AActor* newLootBag = GetWorld()->SpawnActor<AActor>(lootBag, spawnLoc, GetOwner()->GetActorRotation());

	UInventoryComponent* newInvComp = Cast<UInventoryComponent>(newLootBag->GetComponentByClass(UInventoryComponent::StaticClass()));

	if (IsValid(newInvComp))
	{
		FAddItemStatus tryDrop = Cast<UInventoryComponent>(newInvComp)->addNewItem(item);

		if (tryDrop.addStatus && slot >= 0 && slot < inventoryArray.Num())
		{
			removeItem(slot, false);
		}
		else if (!tryDrop.addStatus)
		{
			newLootBag->Destroy();
		}
	}
}



bool UInventoryComponent::isEmpty()
{
	for (int i = 0; i < inventoryArray.Num(); ++i)
	{
		UItemAsset* curItem = inventoryArray[i].item;

		if(curItem != nullptr)
			return false;
	}

	return true;
}

int UInventoryComponent::getRows()
{
	return inventoryArray.Num() / slotsPerRow;
}

void UInventoryComponent::loadInventory(TArray<FInvItem> newInv)
{
	if (newInv.Num() > 0)
	{
		for (int i = 0; i < inventoryArray.Num(); ++i)
		{
			inventoryArray[i] = newInv[i];
		}
	}

	OnInvChanged.Broadcast();
}