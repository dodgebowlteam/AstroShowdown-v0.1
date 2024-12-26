#pragma once

#include "CommonGameInstance.h"
#include "AstroGameInstance.generated.h"

UCLASS(Config = Game)
class ASTROSHOWDOWN_API UAstroGameInstance : public UCommonGameInstance
{
	GENERATED_BODY()

public:
	UAstroGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};