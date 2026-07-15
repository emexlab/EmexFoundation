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
#include <sys/sysctl.h>
#include <string.h>

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
    if(process->command != NULL)
    {
        EFRelease(process->command);
    }
    if(process->executablePath != NULL)
    {
        EFRelease(process->executablePath);
    }
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

    /* need it's bsd information (please be portable, otherwise I cry, I don't know if Linux and FreeBSD have those MIB's bwaa) */
    EFMutableArrayRef mutableArguments = NULL;
    EFStringRef executablePath = NULL;
    int procMib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, processIdentifier };
    int argMaxMib[2] = { CTL_KERN, KERN_ARGMAX };
    int argsMib[3] = { CTL_KERN, KERN_PROCARGS2, processIdentifier };

    /* the main informations */
    struct kinfo_proc proc;
    size_t oldlen = sizeof(struct kinfo_proc);
    if(sysctl(procMib, 4, &proc, &oldlen, NULL, 0) != 0)
    {
        return NULL;
    }

    size_t size = 0;
    int argMax = 0;
    size = sizeof(argMax);
    if(sysctl(argMaxMib, 2, &argMax, &size, NULL, 0) != 0)
    {
        return NULL;
    }

    char *procArgs = malloc(argMax);
    if(procArgs == NULL)
    {
        return NULL;
    }

    size = (size_t)argMax;
    if(sysctl(argsMib, 3, procArgs, &size, NULL, 0) == -1)
    {
        free(procArgs);
        goto skip_arg_copy;
    }

    /* now extracting the information like arguments */
    int argc = 0;
    memcpy(&argc, procArgs, sizeof(argc));
    char *cp = procArgs + sizeof(argc);

    /* NOTE: in this is the executable path! */
    executablePath = EFStringCreateWithCString(kEFAllocatorDefault, cp, kEFStringEncodingUTF8);
    for(; cp < &procArgs[size]; cp++)
    {
        if(*cp == '\0')
        {
            break;
        }
    }

    for(; cp < &procArgs[size]; cp++)
    {
        if(*cp != '\0')
        {
            break;
        }
    }

    if(cp >= &procArgs[size] || argc <= 0)
    {
        if(executablePath != NULL)
        {
            EFRelease(executablePath);
        }
        free(procArgs);
        return NULL;
    }

    mutableArguments = EFArrayCreateMutable(allocatorRef, kEFArrayCallbacksObjectCallbacks, argc);
    if(mutableArguments == NULL)
    {
        if(executablePath != NULL)
        {
            EFRelease(executablePath);
        }
        free(procArgs);
        return NULL;
    }

    int arg_count = 0;
    while(arg_count < argc && cp < &procArgs[size])
    {
        if(arg_count > 0)
        {
            EFStringRef argument = EFStringCreateWithCString(allocatorRef, cp, kEFStringEncodingUTF8);
            if(argument == NULL)
            {
                EFRelease(mutableArguments);
                if(executablePath != NULL)
                {
                    EFRelease(executablePath);
                }
                free(procArgs);
                return NULL;
            }

            Boolean success = EFArrayAppendValue(mutableArguments, argument);
            EFRelease(argument);
            if(!success)
            {
                EFRelease(mutableArguments);
                if(executablePath != NULL)
                {
                    EFRelease(executablePath);
                }
                free(procArgs);
                return NULL;
            }
        }

        cp += strlen(cp) + 1;
        arg_count++;
    }
    free(procArgs);

skip_arg_copy:
    {
        /* the command is already part of the KERN_PROC_PID thingy */
        EFStringRef commandRef = EFStringCreateWithCString(allocatorRef, proc.kp_proc.p_comm, kEFStringEncodingUTF8);
        if(commandRef == NULL)
        {
            if(executablePath != NULL)
            {
                    EFRelease(executablePath);
            }
            return NULL;
        }

        EFArrayRef arguments = NULL;
        if(mutableArguments != NULL)
        {
            arguments = EFArrayCreateCopy(allocatorRef, mutableArguments);
            EFRelease(mutableArguments);
            if(arguments == NULL)
            {
                /* this is a failure, because it usually would have been possible */
                if(executablePath != NULL)
                {
                    EFRelease(executablePath);
                }
                EFRelease(commandRef);
                return NULL;
            }
        }

        __EFProcess process = (__EFProcess)EFObjectAlloc(allocatorRef, EFProcessGetTypeID(), sizeof(struct __EFProcess));
        if(process == NULL)
        {
            if(executablePath != NULL)
            {
                EFRelease(executablePath);
            }
            if(arguments != NULL)
            {
                EFRelease(arguments);
            }
            EFRelease(commandRef);
            return NULL;
        }

        /* TODO: collecting info about processes comes later  */
        process->executablePath = executablePath;
        process->command = commandRef;
        process->arguments = arguments;
        process->processIdentifier = proc.kp_proc.p_pid;
        process->parentProcessIdentifier = proc.kp_eproc.e_ppid;
        process->userIdentifier = proc.kp_eproc.e_ucred.cr_uid;
        process->groupIdentifier = proc.kp_eproc.e_ucred.cr_gid;

        return (EFProcessRef)process;
    }
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
void EFProcessConstructor(void)
{
    EFProcessCurrent = EFProcessCreateWithProcessIdentifier(kEFAllocatorDefault, getpid());
    if(EFProcessCurrent == NULL)
    {
        fprintf(stderr, "EFProcessConstructor: failed to allocate current process\n");
        exit(1);
    }
}
