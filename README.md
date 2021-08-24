<p align="center">
  <img width="399" height="44" src="https://lh3.googleusercontent.com/pw/AM-JKLUACRdnWpy8JD4ImQ4cmmkTvhx6x_x-dfieSJKFySk8E8f_Gd_MY88ljT0KgAtNhOD8joNrkbg2yIrKc5bjhA4pKb3s4337QnOYpW15Mds65too1LErqbW7RemrwTuI43yhqAb5Rpnh_JUP4BX5cPwy=w399-h44-no?authuser=0" alt="">
</p>

# N-AI
This repo contains the .uproject that is being used
to develop a UE4/UE5 plugin called N-AI, which is 
intended to be an all inclusive solution for creating
AI's on massive scales.

## TOC
* [Introduction](#introduction)
* [The Plugin](#the-plugin)
* [Installation Instructions](#installation-instructions)
* [Usage](#usage)
* [Licence](#licence)

## Introduction
I'll do a better job of describing this in future, but
for now, on the point of AI on massive scales, it is 
true that achieving high quality pathfollowing 
dynamics across even 100 agents, results in terrible
performance. The main issue is avoidance, which upon
profiling and digging through the engines source,
it is clear that way too much is being done. Unreal's
systems for AI are extremely sophisticated and must
assume a worst case scenario all the time, which
means tons of checks get performed constantly and
the incurred cost quickly becomes too heavy. So, 
to avoid all the costs, you either have
to settle for simpler AI, or find a plugin for custom
AI in some capacity. This plugin intends to eventually
be a great solution to this problem. I can already
run thousands of agents with no impact on the 
framerate. The only AI system from Unreal that is used
for these agents is the NavMesh system, which is
really feature rich, and can be used to save a lot 
of time implementing a custom graph system, to then
make a custom NavMesh asset type. Outside of that,
we want to deffer everything to Unreal's Async 
systems, along with the Rendering & Physics systems.
We manage all processing that the agents need to do
via a Manager asset which can Tick. Agents themselves
can't Tick, they are not Characters or Pawns, they
are just simple Actors, that  handle their own
construction and deconstruction, and nothing else.
The Manager contains a TMap
that holds a struct wrapper type for each Agent,
where the Key type is an FGuid used to uniquely 
identify each agent. This wrapper
type allows us to almost never have to directly access
the agents object, we just give instruction to Unreal
to do things to each agent via the Manager asset.
Having only one asset Tick, instead of
thousands instantly saves a load of frames.

## The Plugin
The NAI Plugin is my work so far on implementing this. It’s still a heavy 
work-in-progress, but there’s a lot of stuff in 
there already, including a lot of use of Unreal’s 
Async systems, which took a while to figure out 
due to lack of documentation on Epic’s side. I 
will eventually be selling the plugin on the 
Marketplace once it’s completed, but in the 
meantime while I’m working on it, feel free 
to use it for reference, or to copy. It will
eventually become a private repo, however.

## Installation Instructions
1. Make sure you're using UE4 version is 4.27.
2. Create a new project.
3. Inside the directory for your new project, 
create a folder called Plugins, with an uppercase P.
4. Copy the NAI directory, located in the Plugins
folder of this repo, into your projects newly created
Plugins folder.
5. Restart/Open the engine.
6. From within Unreal, open up the Plugins window,
then search for "N-AI", with the hyphen.
7. Enable the Plugin, then restart the engine.

## Usage
_Todo_

## Licence
This Plugin uses the GPL licence. So basically,
you can do what you want with whatever you find
in here.
