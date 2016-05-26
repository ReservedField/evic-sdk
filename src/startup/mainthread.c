/*
 * This file is part of eVic SDK.
 *
 * eVic SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * eVic SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eVic SDK.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 ReservedField
 */

#include <M451Series.h>
#include <Thread.h>

extern void __libc_init_array();
extern void __libc_fini_array();
extern int main();

/**
 * Main thread.
 *
 * @param args Unused.
 *
 * @return NULL.
 */
void *Startup_MainThread(void *args) {
	// Invoke init functions (e.g. static C++ constructors)
	__libc_init_array();

	// Run user main
	main();

	// Invoke fini functions (e.g. static C++ destructors)
	__libc_fini_array();

	return NULL;
}

/**
 * Creates the main thread.
 * The caller must follow up with an infinite loop while
 * waiting for the scheduler to start up.
 * Once the scheduler executes, the caller won't run anymore.
 */
void Startup_CreateMainThread() {
	Thread_t mainThread;

	Thread_Create(&mainThread, Startup_MainThread, NULL, 1024);
}
