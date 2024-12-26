/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AsyncAction_LoadTexture.h"
#include "AstroAssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "UObject/Stack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_LoadTexture)


UAsyncAction_LoadTexture* UAsyncAction_LoadTexture::LoadTexture(UObject* InWorldContextObject, TSoftObjectPtr<UTexture2D> InTextureSoftPtr)
{
	if (InTextureSoftPtr.IsNull())
	{
		FFrame::KismetExecutionMessage(TEXT("LoadTexture was passed a null TextureSoftPtr"), ELogVerbosity::Error);
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	UAsyncAction_LoadTexture* Action = NewObject<UAsyncAction_LoadTexture>();
	Action->TextureSoftPtr = InTextureSoftPtr;
	Action->World = World;
	Action->GameInstance = World->GetGameInstance();
	Action->RegisterWithGameInstance(World);

	return Action;
}

void UAsyncAction_LoadTexture::Activate()
{
	TWeakObjectPtr<UAsyncAction_LoadTexture> LocalWeakThis(this);
	StreamingHandle = UAstroAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
		TextureSoftPtr.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &UAsyncAction_LoadTexture::OnTextureLoaded),
		FStreamableManager::AsyncLoadHighPriority);
}

void UAsyncAction_LoadTexture::Cancel()
{
	Super::Cancel();

	if (StreamingHandle.IsValid())
	{
		StreamingHandle->CancelHandle();
		StreamingHandle.Reset();
	}
}

void UAsyncAction_LoadTexture::OnTextureLoaded()
{
	// If the load as successful, send it, otherwise don't complete this.
	if (TextureSoftPtr.IsValid())
	{
		UTexture2D* Texture = TextureSoftPtr.Get();
		OnComplete.Broadcast(Texture);
		OnCompleteDelegate.Broadcast(Texture);
	}

	StreamingHandle.Reset();

	SetReadyToDestroy();
}

