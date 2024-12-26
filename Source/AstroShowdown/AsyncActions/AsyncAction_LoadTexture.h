/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Engine/CancellableAsyncAction.h"
#include "Engine/Texture2D.h"
#include "UObject/SoftObjectPtr.h"
#include "AsyncAction_LoadTexture.generated.h"

class APlayerController;
class UGameInstance;
class UWorld;
struct FFrame;
struct FStreamableHandle;

/**
 * Loads a texture asynchronously, and once it's loaded, returns it on OnComplete.
 */
UCLASS(BlueprintType)
class UAsyncAction_LoadTexture : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoadTextureDelegate, UTexture2D*, Texture);
	UPROPERTY(BlueprintAssignable)
	FLoadTextureDelegate OnComplete;

	DECLARE_MULTICAST_DELEGATE_OneParam(FLoadTextureStaticDelegate, UTexture2D*);
	FLoadTextureStaticDelegate OnCompleteDelegate;

private:
	TWeakObjectPtr<UWorld> World = nullptr;
	TWeakObjectPtr<UGameInstance> GameInstance = nullptr;
	TSoftObjectPtr<UTexture2D> TextureSoftPtr = nullptr;
	TSharedPtr<FStreamableHandle> StreamingHandle = nullptr;

public:
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true"))
	static UAsyncAction_LoadTexture* LoadTexture(UObject* WorldContextObject, TSoftObjectPtr<UTexture2D> TextureSoftPtr);

	virtual void Activate() override;
	virtual void Cancel() override;

private:
	void OnTextureLoaded();

};
