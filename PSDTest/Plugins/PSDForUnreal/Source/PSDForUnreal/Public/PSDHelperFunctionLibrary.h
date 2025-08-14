// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PSDHelperFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class PSDFORUNREAL_API UPSDHelperFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "PSD")
    static void ConvertPSDToUMG();
	
};
