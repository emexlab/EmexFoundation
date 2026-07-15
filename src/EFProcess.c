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

/* ----------------------------------------------------------------------
 *  System Headers
 * -------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFRuntime.h>
#include <EmexFoundation/EFProcess.h>

typedef struct __EFProcess {
    EFObject header;

    /* basic information */
    SInt32 processIdentifier;
    SInt32 parentProcessIdentifier;
    SInt32 userIdentifier;
    SInt32 groupIdentifier;

    /* nice to have ^^ */
    EFStringRef command;
    EFStringRef executablePath;
    EFArrayRef arguments;
} *__EFProcess;

static void __EFProcessDeinit(EFObjectRef processRef)
{
    __EFProcess process = (__EFProcess)processRef;
    if(process->arguments != NULL)
    {
        EFRelease(process->arguments);
    }
}

static EFStringRef __EFProcessCopyDescription(EFObjectRef processRef)
{
    __EFProcess process = (__EFProcess)processRef;
    EFAllocatorRef allocator = EFGetAllocator(processRef);
    return EFStringCreateWithFormat(allocator, EFSTR("<EFProcess %p>{processIdentifier = %ld, parentProcessIdentifier = %ld, userIdentifier = %ld, groupIdentifier = %ld, command = %@, executablePath = %@, arguments = %@}"), processRef, process->processIdentifier, process->parentProcessIdentifier, process->userIdentifier, process->groupIdentifier, process->command, process->executablePath, process->arguments);
}

static EFClass EFProcessClass = {
    .name = "EFProcess",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EFProcessDeinit,
    .equal = NULL,
    .copyDescription = __EFProcessCopyDescription,
    .hash = NULL,
};

static void EFProcessRegisterClass(void)
{
    EFClassRegister(&EFProcessClass);
}

EFTypeID EFProcessGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFProcessRegisterClass);
    return EFProcessClass.typeID;
}

EFProcessRef EFProcessCreateWithProcessIdentifier(EFAllocatorRef allocatorRef,
                                                  SInt32 processIdentifier)
{
    if(processIdentifier < 0)
    {
        return NULL;
    }

    __EFProcess process = (__EFProcess)EFObjectAlloc(allocatorRef, EFProcessGetTypeID(), sizeof(struct __EFProcess));
    if(process == NULL)
    {
        return NULL;
    }

    /* TODO: collecting info about processes comes later  */
    process->executablePath = NULL;
    process->command = NULL;
    process->arguments = NULL;
    process->processIdentifier = processIdentifier;
    process->parentProcessIdentifier = 0;
    process->userIdentifier = 0;
    process->groupIdentifier = 0;

    return (EFProcessRef)process;
}

SInt32 EFProcessGetProcessIdentifier(EFProcessRef processRef)
{
    __EFProcess process = (__EFProcess)processRef;
    if(process == NULL)
    {
        return 0;
    }

    return process->processIdentifier;
}

SInt32 EFProcessGetParentProcessIdentifier(EFProcessRef processRef)
{
    __EFProcess process = (__EFProcess)processRef;
    if(process == NULL)
    {
        return 0;
    }

    return process->parentProcessIdentifier;
}

SInt32 EFProcessGetUserIdentifier(EFProcessRef processRef)
{
    __EFProcess process = (__EFProcess)processRef;
    if(process == NULL)
    {
        return 0;
    }

    return process->userIdentifier;
}

SInt32 EFProcessGetGroupIdentifier(EFProcessRef processRef)
{
    __EFProcess process = (__EFProcess)processRef;
    if(process == NULL)
    {
        return 0;
    }

    return process->groupIdentifier;
}

EFArrayRef EFProcessGetArguments(EFProcessRef processRef)
{
    __EFProcess process = (__EFProcess)processRef;
    if(process == NULL)
    {
        return NULL;
    }

    return process->arguments;
}

EFProcessRef EFProcessCurrent;

__attribute__((constructor))
void EFProcessConstructor(int argc, const char *argv[])
{
    EFMutableArrayRef mutableArguments = EFArrayCreateMutable(kEFAllocatorDefault, kEFArrayCallbacksObjectCallbacks, argc);
    if(mutableArguments == NULL)
    {
        fprintf(stderr, "EFProcessConstructor: couldn't allocate memory for current processes argument's\n");
        exit(1);
    }

    for(EFIndex index = 0; index < (EFIndex)argc; index++)
    {
        EFStringRef argument = EFStringCreateWithCString(kEFAllocatorDefault, argv[index], kEFStringEncodingUTF8);
        if(argument == NULL)
        {
            fprintf(stderr, "EFProcessConstructor: couldn't allocate memory for current processes argument's\n");
            EFRelease(mutableArguments);
            exit(1);
        }

        Boolean success = EFArrayAppendValue(mutableArguments, argument);
        EFRelease(argument);
        if(!success)
        {
            fprintf(stderr, "EFProcessConstructor: couldn't allocate memory for current processes argument's\n");
            EFRelease(mutableArguments);
            exit(1);
        }
    }

    EFArrayRef arguments = EFArrayCreateCopy(kEFAllocatorDefault, mutableArguments);
    EFRelease(mutableArguments);
    if(arguments == NULL)
    {
        fprintf(stderr, "EFProcessConstructor: couldn't allocate memory for current processes argument's\n");
        exit(1);
    }

    EFProcessCurrent = (__EFProcess)EFObjectAlloc(kEFAllocatorDefault, EFProcessGetTypeID(), sizeof(struct __EFProcess));
    if(EFProcessCurrent == NULL)
    {
        fprintf(stderr, "EFProcessConstructor: couldn't allocate memory for current process\n");
        EFRelease(mutableArguments);
        exit(1);
    }

    EFProcessCurrent->command = NULL;
    EFProcessCurrent->executablePath = NULL;
    EFProcessCurrent->arguments = arguments;
    EFProcessCurrent->processIdentifier = getpid();
    EFProcessCurrent->parentProcessIdentifier = getppid();
    EFProcessCurrent->userIdentifier = getuid();
    EFProcessCurrent->groupIdentifier = getgid();
}
