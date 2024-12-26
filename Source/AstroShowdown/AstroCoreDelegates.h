/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Delegates/Delegate.h"

/** Follows a similar idea to Unreal's CoreDelegates. Essentially a collection of utility multicast delegates. */
class FAstroCoreDelegates
{
public:
	/** Called right before the current level is restarted. */
	static inline TMulticastDelegate<void(bool& bShouldDeferRestart)> OnPreRestartCurrentLevel;

};
