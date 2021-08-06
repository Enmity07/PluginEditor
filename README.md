# PluingEditor
N-AI Plugin Repo

This is mostly useless code, it's an in-progress plugin for UE4/UE5 which will provide the ability to have 1000's of AI's moving while performing avoidance. This requires a full custom AI "Agent" type, which is simply an Actor at it's base, and it has a load of initialization on it, along with some delegates. Another type which is simply an Actor is the AgentManager which is where all the work actually gets done. Each individual Agent does not tick, only the manager does. All pathfinding/raytracing is done using Unreal's Async system, which makes everything multi-threaded in an easy way. If the overhead of the setup for each Async task becomes too heavy, all of that will be offloaded into it's own worker threads too.
