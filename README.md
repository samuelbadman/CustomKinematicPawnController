# Custom Kinematic Pawn Controller

An Unreal Engine 5 project containing a kinematic pawn controller component that implements motion mechanics for a third person pawn.

<a href="https://www.youtube.com/watch?v=7_GY_lDoqdk"><b>Demo video</b></a>

The component source code is located at Source/ProjectSolis/ActorComponents/MovementComponents/CharacterPawnMovementComponent.h/cpp.

## Highlights
* Kinematic motion equations used for code driven motion mechanics.
* Surface slide collision response.
* Gravity simulation.
* Code driven jumping/falling.
* Supports ramps with designer defined maximum walkable slope angle.
* Supports staircases using stepped collision geometry.
* Supports uneven collision surfaces.
* Supports animated root bone driven motion for root motion mechanics.
* Supports multiple collision shapes (vertical capsule, horizontal capsule, box, sphere).

The component implementation is not replicated for online multiplayer.
