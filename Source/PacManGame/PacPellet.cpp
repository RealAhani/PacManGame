// Fill out your copyright notice in the Description page of Project Settings.

// clang-format off
#include "PacPellet.h"
// clang-format on

// Sets default values
APacPellet::APacPellet()
{
    // Set this actor to call Tick() every frame.  You can turn this off to
    // improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;

    BaseComponent = CreateDefaultSubobject<USceneComponent>(
        TEXT("RootComponent"));
    RootComponent = BaseComponent;
}

void APacPellet::Initialize(FPacGridPosition const & InGridPosition)
{
    GridPosition = InGridPosition;
}
