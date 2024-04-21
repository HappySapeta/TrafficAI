# Traffic AI
A traffic simulation model developed in Unreal Engine 5.

Inspired by modern open-world video games, this project was developed to learn the basics of a large-scale real-time traffic simulation.

Please note, that TrafficAI does not use UE5's Mass Entity Framework. 
The goal was to learn about AI used in games and employ strategies that help simulate on a large scale. 
Hence, a custom Data-Oriented-Design approach was favored against an existing framework.

## Bird's eye view demo - Simulating 100 vehicles.
https://github.com/AnupamSahu/TrafficAI/assets/35849508/6ec4b69c-c9f1-41ef-8bfc-9bbf3ec72c49

## Core Systems
Behind the scenes, the simulation is managed by two core systems - `TrRepresentationSystem` and `TrSimulationSystem`.
1. `TrSimulationSystem`
   - It is the brain of the simulation and is responsible for driving all autonomous vehicles.
   For each vehicle, `TrSimulationSystem` uses the [Kinematic Bicycle Model](https://thomasfermi.github.io/Algorithms-for-Automated-Driving/Control/BicycleModel.html#id1) to steer and move the vehicle, [Intelligent Driver Model (IDM)](https://www.researchgate.net/publication/1783975_Congested_Traffic_States_in_Empirical_Observations_and_Microscopic_Simulations) to determine a safe acceleration value to prevent collision against the leading vehicle
   and [Craig Reynold's](https://www.researchgate.net/publication/2495826_Steering_Behaviors_For_Autonomous_Characters) path following algorithm to keep the vehicle on track.
   - Additionally, for each vehicle, it queries vehicles in the vicinity and filters those on a direct course of collision.
   - Although no collision avoidance maneuvers are applied, the braking effect caused by the IDM is enough to prevent head-on collisions.
   - The system also employs another system called `TrIntersectionManager` that simulates traffic signals
     by periodically blocking certain nodes at intersections while allowing the passage of traffic from the rest of the nodes.
   - Last but not least, `TrSimulationSystem` is based on a DOD solution that treats vehicles as numerical entities. It incorporates multiple arrays of floating point values that define the state of each 
     entity. This plays a major role in making the simulation run on the CPU at respectable framerates.
     The following values are used to define the state of a vehicle/entity: Position, Velocity, Acceleration, Heading, Goal (the location the vehicle is supposed to go to), and some metadata 
     values such as the index of the vehicle directly in front of the current vehicle, and information about its current path.
2. `TrRepresentationSystem`
   - No matter how realistic an AI system becomes, in most games it becomes useless if it cannot be interacted with.
   - This system allows players to interact with the vehicles without taxing the CPU too much.
   - Each vehicle has two LODs. One with the least amount of detail is represented with an Instanced Static Mesh while the other with the highest amount of detail
     is represented by an actor.
   - Actors can be controlled by players and can physically interact with the world and other vehicles.
   - `TrRepresentationSystem` seamlessly swaps ISMCs with Actors and vice-versa as they come in and out of range of the player.
   - While ISMCs are moved by directly overriding their position & orientation received from `TrSimulationSystem`, actors are moved by a more sophisticated system.
   - Vehicle Actors are types of [`AWheeledVehiclePawn`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/ChaosVehicles/AWheeledVehiclePawn?application_version=5.3) derived from
     [UE's Chaos Vehicle System](https://dev.epicgames.com/documentation/en-us/unreal-engine/vehicles-in-unreal-engine?application_version=5.3).
     A PID Controller is used, to determine the magnitude and direction of the force required to anchor the vehicle to the desired position and steer the vehicle's actor to match its orientation, to
     the values received from the simulation system.

## Collision Avoidance and Path Following Demo
https://github.com/AnupamSahu/TrafficAI/assets/35849508/d815abe7-0550-4428-bdb0-4c93fc767014

In TrafficAI, vehicles don't actively try to avoid obstacles or other vehicles by using specialized maneuvers. Instead, they detect vehicles that are directly in front of them and choose the one that poses the greatest risk of collision. They then apply brakes to prevent an imminent collision. 

Detecting objects in the surroundings can be a challenging task, especially if we want to avoid doing too many comparisons. To solve this problem, a spatial acceleration structure called Implicit Grid was used, which allows us to query nearby vehicles very quickly.

After the query is complete, a mathematical operation using the position of each queried vehicle is performed to determine which vehicles overlap against the sensing region of the current vehicle.
Here's how it has been implemented in TrafficAI - https://github.com/HappySapeta/TrafficAI/blob/99e33e7cdaeba1389d6181efb5f138dfe63e7a48/Source/TrafficAI/Simulation/TrSimulationSystem.cpp#L385-L407
It is a straightforward procedure that converts a world position into a relative local position relative to the sensing vehicle's position and heading,
and verifies if it falls within the collision boundaries of the vehicle.

This, by far is the most time-consuming operation in the entire simulation update. Any efforts to further optimize the system must be focussed on this region in [UTrSimulationSystem::UpdateCollisionData](https://github.com/HappySapeta/TrafficAI/blob/99e33e7cdaeba1389d6181efb5f138dfe63e7a48/Source/TrafficAI/Simulation/TrSimulationSystem.cpp#L373)

## References

* [5.7 Path Following (Steering) - Nature of Code](https://www.youtube.com/watch?v=rlZYT-uvmGQ)
* [Proportional–integral–derivative controller](https://en.wikipedia.org/wiki/Proportional%E2%80%93integral%E2%80%93derivative_controller)
* [CAR STEERING](https://kidscancode.org/godot_recipes/4.x/2d/car_steering/index.html)
* [Simple 2D car steering physics in games](https://engineeringdotnet.blogspot.com/2010/04/simple-2d-car-physics-in-games.html)

### Note
The paid assets used in the demonstrations are not included in the repository but can be found at [Polygon - City Pack](https://www.unrealengine.com/marketplace/en-US/product/polygon-city-pack).
