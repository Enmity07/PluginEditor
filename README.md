# PluingEditor
N-AI Plugin Repo

A few years back, I was experimenting with different ways of achieving good quality avoidance on AI’s, while having the ability to run thousands to tens of thousands of them at once.

Turns out that it largely comes down to limiting how much each AI Pawn does as much as possible. In fact, it’s best to just not bother with much of Unreal’s features for AI, since they incur too much cost when done on large scales. So in order to have the situation we desire, we’re almost going to have to start from the ground up. We’ll still want to use Unreal’s Pathfinding systems, since they’re awesome. All this means is we’ll still calculate Paths using Unreal’s NavMesh systems, but we won’t be using Unreal’s Pathfollowing systems to move along them.

With the above in mind, we’ll just have each AI be an Actor instead of a Pawn, because we won’t need a Controller, as that relates to Pathfollowing which we’ll be doing ourselves. Having each AI be able to Tick is also not needed, nor is it desired, because we want each AI to be as simple as possible. So how do we do custom Pathfollowing if each AI can’t Tick? Well it’s simple, we just offload all of that stuff to one single Actor which does have the ability to Tick. This Actor will serve as a Manager for all the AI’s spawned in the world. Taking things further, we’ll organize anything and everything that each AI wants to do into “Tasks” which will be processed by the Manager, such that each AI’s Actor does virtually nothing, only construction and deconstruction. This is essentially identical to the Client/Server Model for multiplayer games.
The Plugin

The NAI Plugin is my work so far on implementing this. It’s still a heavy work-in-progress, but there’s a lot of stuff in there already, including a lot of use of Unreal’s Async systems, which took a while to figure out due to lack of documentation on Epic’s side. I will eventually be selling the plugin on the Marketplace once it’s completed, but in the meantime while I’m working on it, feel free to use it for reference.
