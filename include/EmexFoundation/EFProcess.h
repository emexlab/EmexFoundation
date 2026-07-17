/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of EmexFoundation.
 *
 * EmexFoundation is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EmexFoundation is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with EmexFoundation. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EFPROCESS_H
#define EFPROCESS_H

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFRuntime.h>
#include <EmexFoundation/EFArray.h>
#include <EmexFoundation/EFString.h>

typedef struct __EFProcess *EFProcessRef;

extern EFProcessRef EFProcessCurrent;

extern EFTypeID EFProcessGetTypeID(void);

extern EFProcessRef EFProcessCreateWithCommand(EFAllocatorRef allocatorRef, EFStringRef commandRef, EFArrayRef arguments);
extern EFProcessRef EFProcessCreateWithPath(EFAllocatorRef allocatorRef, EFStringRef pathRef, EFArrayRef arguments);
extern EFProcessRef EFProcessCreateWithProcessIdentifier(EFAllocatorRef allocatorRef, SInt32 processIdentifier);

extern SInt32 EFProcessGetProcessIdentifier(EFProcessRef processRef);
extern SInt32 EFProcessGetParentProcessIdentifier(EFProcessRef processRef);
extern SInt32 EFProcessGetUserIdentifier(EFProcessRef processRef);
extern SInt32 EFProcessGetGroupIdentifier(EFProcessRef processRef);

extern Boolean EFProcessSendSignal(EFProcessRef processRef, SInt32 signal);
extern Boolean EFProcessSuspend(EFProcessRef processRef);
extern Boolean EFProcessResume(EFProcessRef processRef);
extern Boolean EFProcessTerminate(EFProcessRef processRef);
extern Boolean EFProcessForceKill(EFProcessRef processRef);

extern Boolean EFProcessIsAlive(EFProcessRef processRef);

extern EFStringRef EFProcessGetCommand(EFProcessRef processRef);
extern EFStringRef EFProcessGetExecutablePath(EFProcessRef processRef);
extern EFArrayRef EFProcessGetArguments(EFProcessRef processRef);

extern SInt32 EFProcessWaitPID(EFProcessRef processRef, int *status, int options);

#endif /* EFPROCESS_H */
