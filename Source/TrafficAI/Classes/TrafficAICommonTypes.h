// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#define SET_ACTOR_ENABLED(Actor, Value) Actor->SetActorEnableCollision(Value); Actor->SetActorHiddenInGame(!Value); Actor->SetActorTickEnabled(Value);