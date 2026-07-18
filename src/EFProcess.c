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
#include <string.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#ifdef __linux__
#include <limits.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/sysctl.h>
#ifdef __FreeBSD__
#include <sys/user.h>
#endif /* __FreeBSD__ */
#endif /* __linux__ || __APPLE__ || __FreeBSD__ */

/* ----------------------------------------------------------------------
 *  EmexFoundation Headers
 * -------------------------------------------------------------------- */
#include <EmexFoundation/runtime/EFRuntime.h>
#include <EmexFoundation/EFProcess.h>
#include <EmexFoundation/EFFileHandle.h>

typedef struct __EFProcess {
    EFObject header;
    Boolean weSpawnedThis;

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
    if(process->weSpawnedThis)
    {
        EFProcessForceKill(processRef);
    }

    EFReleaseTry(process->command);
    EFReleaseTry(process->executablePath);
    EFReleaseTry(process->arguments);
}

static EFStringRef __EFProcessCopyDescription(EFObjectRef processRef)
{
    __EFProcess process = (__EFProcess)processRef;
    EFAllocatorRef allocator = EFGetAllocator(processRef);
    return EFStringCreateWithFormat(allocator, EFSTR("<EFProcess %p>{processIdentifier = %ld, parentProcessIdentifier = %ld, userIdentifier = %ld, groupIdentifier = %ld, command = %@, executablePath = %@, arguments = %@, alive = %d}"), processRef, process->processIdentifier, process->parentProcessIdentifier, process->userIdentifier, process->groupIdentifier, process->command, process->executablePath, process->arguments, EFProcessIsAlive(processRef));
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

extern char *const *environ;

EFProcessRef EFProcessCreateWithCommand(EFAllocatorRef allocatorRef,
                                        EFStringRef commandRef,
                                        EFArrayRef arguments)
{
    if(commandRef == NULL || arguments == NULL)
    {
        return NULL;
    }

    const char *commandPtr = EFStringGetCStringPtr(commandRef, kEFStringEncodingUTF8);
    if(commandPtr == NULL)
    {
        return NULL;
    }

    EFIndex argumentsCount = EFArrayGetCount(arguments) + 1;
    const char *argv[argumentsCount + 1];
    argv[0] = commandPtr;
    for(EFIndex argumentsIndex = 0; argumentsIndex < (argumentsCount - 1); argumentsIndex++)
    {
        const char *cptr = EFStringGetCStringPtr(EFArrayGetValueAtIndex(arguments, argumentsIndex), kEFStringEncodingUTF8);
        if(cptr == NULL)
        {
            return NULL;
        }
        argv[argumentsIndex + 1] = cptr;
    }
    argv[argumentsCount] = NULL;

    pid_t pid = 0;
    if(posix_spawnp(&pid, commandPtr, NULL, NULL, (char *const *)argv, environ) != 0)
    {
        return NULL;
    }

    __EFProcess process = (__EFProcess)EFProcessCreateWithProcessIdentifier(allocatorRef, pid);
    process->weSpawnedThis = true;
    return (EFProcessRef)process;
}

EFProcessRef EFProcessCreateWithPath(EFAllocatorRef allocatorRef,
                                     EFStringRef pathRef,
                                     EFArrayRef arguments)
{
    if(pathRef == NULL || arguments == NULL)
    {
        return NULL;
    }

    const char *pathPtr = EFStringGetCStringPtr(pathRef, kEFStringEncodingUTF8);
    if(pathPtr == NULL)
    {
        return NULL;
    }

    EFIndex argumentsCount = EFArrayGetCount(arguments) + 1;
    const char *argv[argumentsCount + 1];
    argv[0] = pathPtr;
    for(EFIndex argumentsIndex = 0; argumentsIndex < (argumentsCount - 1); argumentsIndex++)
    {
        const char *cptr = EFStringGetCStringPtr(EFArrayGetValueAtIndex(arguments, argumentsIndex), kEFStringEncodingUTF8);
        if(cptr == NULL)
        {
            return NULL;
        }
        argv[argumentsIndex + 1] = cptr;
    }
    argv[argumentsCount] = NULL;

    pid_t pid = 0;
    if(posix_spawn(&pid, pathPtr, NULL, NULL, (char *const *)argv, environ) != 0)
    {
        return NULL;
    }

    __EFProcess process = (__EFProcess)EFProcessCreateWithProcessIdentifier(allocatorRef, pid);
    process->weSpawnedThis = true;
    return (EFProcessRef)process;
}

EFProcessRef EFProcessCreateWithProcessIdentifier(EFAllocatorRef allocatorRef,
                                                  SInt32 processIdentifier)
{
    if(processIdentifier < 0)
    {
        return NULL;
    }

    EFAUTOREL EFMutableArrayRef mutableArguments = NULL;
    EFAUTOREL EFStringRef executablePath = NULL;
    EFAUTOREL EFStringRef commandRef = NULL;

    SInt32 pid = 0;
    SInt32 ppid = 0;
    SInt32 uid = 0;
    SInt32 gid = 0;

    char const *commandCString = NULL;

#ifdef __APPLE__
    int procMib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, processIdentifier };
    struct kinfo_proc proc;
    size_t oldlen = sizeof(struct kinfo_proc);
    if(sysctl(procMib, 4, &proc, &oldlen, NULL, 0) != 0)
    {
        return NULL;
    }
    pid = proc.kp_proc.p_pid;
    ppid = proc.kp_eproc.e_ppid;
    uid = proc.kp_eproc.e_ucred.cr_uid;
    gid = proc.kp_eproc.e_ucred.cr_gid;
    commandCString = proc.kp_proc.p_comm;
#elifdef __FreeBSD__
    int procMib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, processIdentifier };
    struct kinfo_proc proc;
    size_t oldlen = sizeof(struct kinfo_proc);
    if(sysctl(procMib, 4, &proc, &oldlen, NULL, 0) != 0)
    {
        return NULL;
    }
    pid = proc.ki_pid;
    ppid = proc.ki_ppid;
    uid = proc.ki_uid;
    gid = proc.ki_groups[0];
    commandCString = proc.ki_comm;
#elifdef __linux__
    /* linus, seriously?? kill your self */
    pid = processIdentifier;
    char backupName[256] = {0};

    char statusPath[64];
    snprintf(statusPath, sizeof(statusPath), "/proc/%ld/status", (long)processIdentifier);
    FILE *statusFile = fopen(statusPath, "r");
    if(statusFile != NULL)
    {
        char line[256];
        while(fgets(line, sizeof(line), statusFile))
        {
            if(strncmp(line, "Name:", 5) == 0)
            {
                sscanf(line + 5, "%255s", backupName);
            }
            else if(strncmp(line, "PPid:", 5) == 0)
            {
                sscanf(line + 5, "%ld", (long *)&ppid);
            }
            else if(strncmp(line, "Uid:", 4) == 0)
            {
                sscanf(line + 4, "%ld", (long *)&uid);
            }
            else if(strncmp(line, "Gid:", 4) == 0)
            {
                sscanf(line + 4, "%ld", (long *)&gid);
            }
        }
        fclose(statusFile);
    }

    char exePath[64];
    snprintf(exePath, sizeof(exePath), "/proc/%ld/exe", (long)processIdentifier);
    char linkTarget[PATH_MAX];
    ssize_t linkLen = readlink(exePath, linkTarget, sizeof(linkTarget) - 1);
    if(linkLen != -1)
    {
        linkTarget[linkLen] = '\0';
        executablePath = EFStringCreateWithCString(allocatorRef, linkTarget, kEFStringEncodingUTF8);
    }

    char cmdlinePath[64];
    snprintf(cmdlinePath, sizeof(cmdlinePath), "/proc/%ld/cmdline", (long)processIdentifier);
    FILE *cmdFile = fopen(cmdlinePath, "rb");
    if(cmdFile != NULL)
    {
        size_t capacity = 4096;
        char *cmdBuffer = malloc(capacity);
        size_t readBytes = 0;

        if(cmdBuffer != NULL)
        {
            while(1)
            {
                size_t readNow = fread(cmdBuffer + readBytes, 1, capacity - readBytes - 1, cmdFile);
                if(readNow == 0)
                {
                    break;
                }
                readBytes += readNow;
                if(readBytes >= capacity - 1)
                {
                    capacity *= 2;
                    char *newBuf = realloc(cmdBuffer, capacity);
                    if(newBuf == NULL)
                    {
                        break;
                    }
                    cmdBuffer = newBuf;
                }
            }
            cmdBuffer[readBytes] = '\0';

            if(readBytes > 0)
            {
                int linuxArgc = 0;
                for(size_t i = 0; i < readBytes; i++)
                {
                    if(cmdBuffer[i] == '\0')
                    {
                        linuxArgc++;
                    }
                }
                if(cmdBuffer[readBytes - 1] != '\0')
                {
                    linuxArgc++;
                }

                if(linuxArgc > 0)
                {
                    commandCString = cmdBuffer;

                    mutableArguments = EFArrayCreateMutable(allocatorRef, kEFArrayCallbacksObjectCallbacks, linuxArgc);
                    if(mutableArguments != NULL)
                    {
                        char *cp = cmdBuffer;
                        int argIndex = 0;
                        while(argIndex < linuxArgc && cp < (cmdBuffer + readBytes))
                        {
                            if(argIndex > 0)
                            {
                                EFStringRef argument = EFStringCreateWithCString(allocatorRef, cp, kEFStringEncodingUTF8);
                                if(argument != NULL)
                                {
                                    EFArrayAppendValue(mutableArguments, argument);
                                    EFRelease(argument);
                                }
                            }
                            cp += strlen(cp) + 1;
                            argIndex++;
                        }
                    }
                }
                else
                {
                    free(cmdBuffer);
                }
            }
            else
            {
                free(cmdBuffer);
            }
        }
        fclose(cmdFile);
    }

    if(commandCString == NULL && backupName[0] != '\0')
    {
        commandCString = strdup(backupName);
    }
#else
#error "EFProcess is not supported"
#endif /* __APPLE__ || __FreeBSD__ || __linux__ */

#ifdef __APPLE__
    int argMaxMib[2] = { CTL_KERN, KERN_ARGMAX };
    int argsMib[3] = { CTL_KERN, KERN_PROCARGS2, processIdentifier };

    int argMax = 0;
    size_t size = sizeof(argMax);
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
        free(procArgs);
        return NULL;
    }

    mutableArguments = EFArrayCreateMutable(allocatorRef, kEFArrayCallbacksObjectCallbacks, argc);
    if(mutableArguments == NULL)
    {
        free(procArgs);
        return NULL;
    }

    int arg_count = 0;
    while(arg_count < argc && cp < &procArgs[size])
    {
        if(arg_count > 0)
        {
            EFAUTOREL EFStringRef argument = EFStringCreateWithCString(allocatorRef, cp, kEFStringEncodingUTF8);
            if(argument == NULL || !EFArrayAppendValue(mutableArguments, argument))
            {
                free(procArgs);
                return NULL;
            }
        }
        cp += strlen(cp) + 1;
        arg_count++;
    }
    free(procArgs);
#elifdef __FreeBSD__
    int pathMib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, processIdentifier };
    char pathBuf[1024];
    size_t pathLen = sizeof(pathBuf);
    if(sysctl(pathMib, 4, pathBuf, &pathLen, NULL, 0) == 0 && pathLen > 0)
    {
        executablePath = EFStringCreateWithCString(allocatorRef, pathBuf, kEFStringEncodingUTF8);
    }

    int argsMib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ARGS, processIdentifier };
    size_t argsSize = 0;

    if(sysctl(argsMib, 4, NULL, &argsSize, NULL, 0) == 0 && argsSize > 0)
    {
        char *argsBuf = malloc(argsSize);
        if(argsBuf != NULL)
        {
            if(sysctl(argsMib, 4, argsBuf, &argsSize, NULL, 0) == 0)
            {
                mutableArguments = EFArrayCreateMutable(allocatorRef, kEFArrayCallbacksObjectCallbacks, 0);
                if(mutableArguments != NULL)
                {
                    char *cp = argsBuf;
                    char *end = argsBuf + argsSize;
                    if(cp < end)
                    {
                        cp += strlen(cp) + 1;
                    }
                    while(cp < end && *cp != '\0')
                    {
                        EFAUTOREL EFStringRef argument = EFStringCreateWithCString(allocatorRef, cp, kEFStringEncodingUTF8);
                        if(argument == NULL || !EFArrayAppendValue(mutableArguments, argument))
                        {
                            free(argsBuf);
                            return NULL;
                        }
                        cp += strlen(cp) + 1;
                    }
                }
            }
            free(argsBuf);
        }
    }
#endif /* __APPLE__ || __FreeBSD__ || __linux__ */

#ifdef __APPLE__
skip_arg_copy:
#endif /* __APPLE__ */

    if(mutableArguments == NULL && (mutableArguments = EFArrayCreate(allocatorRef, kEFArrayCallbacksObjectCallbacks, NULL, 0)) == NULL)
    {
        return NULL;
    }

    commandRef = EFStringCreateWithCString(allocatorRef, commandCString, kEFStringEncodingUTF8);
    if(executablePath == NULL && (executablePath = EFRetainTry(commandRef)) == NULL)
    {
        return NULL;
    }

#ifdef __linux__
    if(commandCString != NULL)
    {
        free((void *)commandCString);
    }
#endif

    if(commandRef == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFArrayRef arguments = NULL;
    if(mutableArguments != NULL)
    {
        arguments = EFArrayCreateCopy(allocatorRef, mutableArguments);
        if(arguments == NULL)
        {
            return NULL;
        }
    }

    __EFProcess process = (__EFProcess)EFObjectCreate(allocatorRef, EFProcessGetTypeID(), (EFIndex)sizeof(struct __EFProcess));
    if(process == NULL)
    {
        return NULL;
    }

    process->executablePath = EFAUTOTRANSFER(executablePath);
    process->command = EFAUTOTRANSFER(commandRef);
    process->arguments = EFAUTOTRANSFER(arguments);
    process->processIdentifier = pid;
    process->parentProcessIdentifier = ppid;
    process->userIdentifier = uid;
    process->groupIdentifier = gid;
    process->weSpawnedThis = false;

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

EFStringRef EFProcessGetCommand(EFProcessRef processRef)
{
    __EFProcess process = (__EFProcess)processRef;
    if(process == NULL)
    {
        return NULL;
    }

    return process->command;
}

EFStringRef EFProcessGetExecutablePath(EFProcessRef processRef)
{
    __EFProcess process = (__EFProcess)processRef;
    if(process == NULL)
    {
        return NULL;
    }

    return process->executablePath;
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

Boolean EFProcessSendSignal(EFProcessRef processRef,
                            SInt32 signal)
{
    __EFProcess process = (__EFProcess)processRef;
    if(process == NULL)
    {
        return false;
    }

    return kill(process->processIdentifier, signal) == 0;
}

Boolean EFProcessSuspend(EFProcessRef processRef)
{
    return EFProcessSendSignal(processRef, SIGSTOP);
}

Boolean EFProcessResume(EFProcessRef processRef)
{
    return EFProcessSendSignal(processRef, SIGCONT);
}

Boolean EFProcessTerminate(EFProcessRef processRef)
{
    return EFProcessSendSignal(processRef, SIGTERM);
}

Boolean EFProcessForceKill(EFProcessRef processRef)
{
    return EFProcessSendSignal(processRef, SIGKILL);
}

Boolean EFProcessIsAlive(EFProcessRef processRef)
{
    __EFProcess process = (__EFProcess)processRef;
    if(process == NULL)
    {
        return false;
    }

    return getpgid(process->processIdentifier) != -1;
}

EFProcessRef EFProcessCurrent;

extern EFProcessRef EFProcessGetCurrentProcess(void)
{
    return EFProcessCurrent;
}

__attribute__((constructor(102)))
void EFProcessConstructor(void)
{
    EFProcessCurrent = EFProcessCreateWithProcessIdentifier(kEFAllocatorDefault, getpid());
    if(EFProcessCurrent == NULL)
    {
        fprintf(stderr, "EFProcessConstructor: failed to allocate current process\n");
        exit(1);
    }
}

SInt32 EFProcessWaitPID(EFProcessRef processRef,
                        int *status,
                        int options)
{
    __EFProcess process = (__EFProcess)processRef;
    if(process == NULL)
    {
        return -1;
    }

    return waitpid(process->processIdentifier, status, options);
}
