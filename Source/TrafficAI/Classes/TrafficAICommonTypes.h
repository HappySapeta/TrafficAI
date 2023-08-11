// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#define DISABLE_ACTOR(Actor) Actor->SetActorEnableCollision(false); Actor->SetActorHiddenInGame(true); Actor->SetActorTickEnabled(false);
#define ENABLE_ACTOR(Actor) Actor->SetActorEnableCollision(true); Actor->SetActorHiddenInGame(false); Actor->SetActorTickEnabled(true);