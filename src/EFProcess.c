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
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
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
#include <EmexFoundation/EFRuntime/EFRuntime.h>
#include <EmexFoundation/EFProcess.h>
#include <EmexFoundation/EFFileHandle.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/EFArray.h>

struct __EFProcess {
    EFObject super;
    Boolean weSpawnedThis;

    /* basic information */
    SInt32 processIdentifier;
    SInt32 parentProcessIdentifier;
    SInt32 userIdentifier;
    SInt32 groupIdentifier;
    SInt32 processGroupIdentifier;
    SInt32 sessionIdentifier;

    /* nice to have ^^ */
    EFStringRef command;
    EFStringRef executablePath;
    EFArrayRef arguments;
};

static void __EFProcessDeinit(EFObjectRef processRef)
{
    EFProcessRef process = (EFProcessRef)processRef;
    if(process->weSpawnedThis)
    {
        EFProcessForceKill(processRef);
    }

    EFReleaseTry(process->command);
    EFReleaseTry(process->executablePath);
    EFReleaseTry(process->arguments);
}

static EFStringRef __EFProcessDebugCopyDescription(EFObjectRef processRef)
{
    EFProcessRef process = (EFProcessRef)processRef;
    EFAllocatorRef allocator = EFGetAllocator(processRef);
    return EFStringCreateWithFormat(allocator, EFSTR("<EFProcess %p>{processIdentifier = %ld, parentProcessIdentifier = %ld, userIdentifier = %ld, groupIdentifier = %ld, processGroupIdentifier = %ld, sessionIdentifier = %ld, command = %@, executablePath = %@, arguments = %@, alive = %d}"), processRef, process->processIdentifier, process->parentProcessIdentifier, process->userIdentifier, process->groupIdentifier, process->processGroupIdentifier, process->sessionIdentifier, process->command, process->executablePath, process->arguments, EFProcessIsAlive(processRef));
}

EF_HIDDEN EFClassDefinitionNewest EFProcessClass = {
    .header = {
        .version = EFCLASS_NEWEST_VERSION,
        .typeID = kEFTypeIDProcess,
        .name = EFSTR_FILESCOPE("EFProcess"),
    },
    .init = NULL,
    .deinit = __EFProcessDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = __EFProcessDebugCopyDescription,
};

EFTypeID EFProcessGetTypeID(void)
{
    return kEFTypeIDNumber;
}

extern char *const *environ;

EFProcessRef EFProcessCreateWithCommand(EFAllocatorRef allocator,
                                        EFStringRef command,
                                        EFArrayRef arguments)
{
    if(command == NULL || arguments == NULL)
    {
        return NULL;
    }

    const char *commandPtr = EFStringGetCStringPtr(command, kEFStringEncodingUTF8);
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

    EFProcessRef process = (EFProcessRef)EFProcessCreateWithProcessIdentifier(allocator, pid);
    process->weSpawnedThis = true;
    return process;
}

EFProcessRef EFProcessCreateWithPath(EFAllocatorRef allocator,
                                     EFStringRef path,
                                     EFArrayRef arguments)
{
    if(path == NULL || arguments == NULL)
    {
        return NULL;
    }

    const char *pathPtr = EFStringGetCStringPtr(path, kEFStringEncodingUTF8);
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

    EFProcessRef process = (EFProcessRef)EFProcessCreateWithProcessIdentifier(allocator, pid);
    process->weSpawnedThis = true;
    return process;
}

EFProcessRef EFProcessCreateWithProcessIdentifier(EFAllocatorRef allocator,
                                                  SInt32 processIdentifier)
{
    if(processIdentifier <= 0)
    {
        return NULL;
    }

    EFAUTOREL EFMutableArrayRef mutableArguments = NULL;
    EFAUTOREL EFStringRef executablePath = NULL;
    EFAUTOREL EFStringRef command = NULL;

    SInt32 pid = 0;
    SInt32 ppid = 0;
    SInt32 uid = 0;
    SInt32 gid = 0;
    SInt32 pgid = getpgid(processIdentifier);
    SInt32 sid = getsid(processIdentifier);

    char const *commandCString = NULL;

#ifdef __APPLE__
    SInt32 procMib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, processIdentifier };
    struct kinfo_proc proc;
    EFSize oldlen = sizeof(struct kinfo_proc);
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
    SInt32 procMib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, processIdentifier };
    struct kinfo_proc proc;
    EFSize oldlen = sizeof(struct kinfo_proc);
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
    EFIndex linkLen = readlink(exePath, linkTarget, sizeof(linkTarget) - 1);
    if(linkLen != -1)
    {
        linkTarget[linkLen] = '\0';
        executablePath = EFStringCreateWithCString(allocator, linkTarget, kEFStringEncodingUTF8);
    }

    char cmdlinePath[64];
    snprintf(cmdlinePath, sizeof(cmdlinePath), "/proc/%ld/cmdline", (long)processIdentifier);
    FILE *cmdFile = fopen(cmdlinePath, "rb");
    if(cmdFile != NULL)
    {
        EFSize capacity = 4096;
        char *cmdBuffer = EFAllocatorAllocate(allocator, capacity, 0);
        EFSize readBytes = 0;

        if(cmdBuffer != NULL)
        {
            while(1)
            {
                EFSize readNow = fread(cmdBuffer + readBytes, 1, capacity - readBytes - 1, cmdFile);
                if(readNow == 0)
                {
                    break;
                }
                readBytes += readNow;
                if(readBytes >= capacity - 1)
                {
                    capacity *= 2;
                    char *newBuf = EFAllocatorReallocate(allocator, cmdBuffer, capacity, 0);
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
                SInt32 linuxArgc = 0;
                for(EFSize i = 0; i < readBytes; i++)
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

                    mutableArguments = EFArrayCreateMutable(allocator, kEFArrayCallbacksObjectCallbacks, linuxArgc);
                    if(mutableArguments != NULL)
                    {
                        char *cp = cmdBuffer;
                        SInt32 argIndex = 0;
                        while(argIndex < linuxArgc && cp < (cmdBuffer + readBytes))
                        {
                            if(argIndex > 0)
                            {
                                EFStringRef argument = EFStringCreateWithCString(allocator, cp, kEFStringEncodingUTF8);
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
                    EFAllocatorDeallocate(allocator, cmdBuffer);
                }
            }
            else
            {
                EFAllocatorDeallocate(allocator, cmdBuffer);
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
    SInt32 argMaxMib[2] = { CTL_KERN, KERN_ARGMAX };
    SInt32 argsMib[3] = { CTL_KERN, KERN_PROCARGS2, processIdentifier };

    SInt32 argMax = 0;
    EFSize size = sizeof(argMax);
    if(sysctl(argMaxMib, 2, &argMax, &size, NULL, 0) != 0)
    {
        return NULL;
    }

    char *procArgs = EFAllocatorAllocate(allocator, argMax, 0);
    if(procArgs == NULL)
    {
        return NULL;
    }

    size = (EFSize)argMax;
    if(sysctl(argsMib, 3, procArgs, &size, NULL, 0) == -1)
    {
        EFAllocatorDeallocate(allocator, procArgs);
        goto skip_arg_copy;
    }

    /* now extracting the information like arguments */
    SInt32 argc = 0;
    memcpy(&argc, procArgs, sizeof(argc));
    char *cp = procArgs + sizeof(argc);

    /* NOTE: in this is the executable path! */
    executablePath = EFStringCreateWithCString(allocator, cp, kEFStringEncodingUTF8);
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
        EFAllocatorDeallocate(allocator, procArgs);
        return NULL;
    }

    mutableArguments = EFArrayCreateMutable(allocator, kEFArrayCallbacksObjectCallbacks, argc);
    if(mutableArguments == NULL)
    {
        EFAllocatorDeallocate(allocator, procArgs);
        return NULL;
    }

    SInt32 arg_count = 0;
    while(arg_count < argc && cp < &procArgs[size])
    {
        if(arg_count > 0)
        {
            EFAUTOREL EFStringRef argument = EFStringCreateWithCString(allocator, cp, kEFStringEncodingUTF8);
            if(argument == NULL || !EFArrayAppendValue(mutableArguments, argument))
            {
                EFAllocatorDeallocate(allocator, procArgs);
                return NULL;
            }
        }
        cp += strlen(cp) + 1;
        arg_count++;
    }
    EFAllocatorDeallocate(allocator, procArgs);
#elifdef __FreeBSD__
    SInt32 pathMib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, processIdentifier };
    char pathBuf[1024];
    EFSize pathLen = sizeof(pathBuf);
    if(sysctl(pathMib, 4, pathBuf, &pathLen, NULL, 0) == 0 && pathLen > 0)
    {
        executablePath = EFStringCreateWithCString(allocator, pathBuf, kEFStringEncodingUTF8);
    }

    SInt32 argsMib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ARGS, processIdentifier };
    EFSize argsSize = 0;

    if(sysctl(argsMib, 4, NULL, &argsSize, NULL, 0) == 0 && argsSize > 0)
    {
        char *argsBuf = EFAllocatorAllocate(allocator, argsSize, 0);
        if(argsBuf != NULL)
        {
            if(sysctl(argsMib, 4, argsBuf, &argsSize, NULL, 0) == 0)
            {
                mutableArguments = EFArrayCreateMutable(allocator, kEFArrayCallbacksObjectCallbacks, 0);
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
                        EFAUTOREL EFStringRef argument = EFStringCreateWithCString(allocator, cp, kEFStringEncodingUTF8);
                        if(argument == NULL || !EFArrayAppendValue(mutableArguments, argument))
                        {
                            EFAllocatorDeallocate(allocator, argsBuf);
                            return NULL;
                        }
                        cp += strlen(cp) + 1;
                    }
                }
            }
            EFAllocatorDeallocate(allocator, argsBuf);
        }
    }
#endif /* __APPLE__ || __FreeBSD__ || __linux__ */

#ifdef __APPLE__
skip_arg_copy:
#endif /* __APPLE__ */

    if(mutableArguments == NULL && (mutableArguments = EFArrayCreate(allocator, kEFArrayCallbacksObjectCallbacks, NULL, 0)) == NULL)
    {
        return NULL;
    }

    command = EFStringCreateWithCString(allocator, commandCString, kEFStringEncodingUTF8);
    if(executablePath == NULL && (executablePath = EFRetainTry(command)) == NULL)
    {
        return NULL;
    }

#ifdef __linux__
    if(commandCString != NULL)
    {
        EFAllocatorDeallocate(allocatorRef, (void *)commandCString);
    }
#endif

    if(command == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFArrayRef arguments = NULL;
    if(mutableArguments != NULL)
    {
        arguments = EFArrayCreateCopy(allocator, mutableArguments);
        if(arguments == NULL)
        {
            return NULL;
        }
    }

    EFProcessRef process = (EFProcessRef)EFObjectCreate(allocator, EFProcessGetTypeID(), (EFIndex)sizeof(struct __EFProcess));
    if(process == NULL)
    {
        return NULL;
    }

    process->executablePath = EFAUTOTRANSFER(executablePath);
    process->command = EFAUTOTRANSFER(command);
    process->arguments = EFAUTOTRANSFER(arguments);
    process->processIdentifier = pid;
    process->parentProcessIdentifier = ppid;
    process->userIdentifier = uid;
    process->groupIdentifier = gid;
    process->processGroupIdentifier = pgid;
    process->sessionIdentifier = sid;
    process->weSpawnedThis = false;

    return process;
}

SInt32 EFProcessGetProcessIdentifier(EFProcessRef process)
{
    if(process == NULL)
    {
        return -1;
    }

    return process->processIdentifier;
}

SInt32 EFProcessGetParentProcessIdentifier(EFProcessRef process)
{
    if(process == NULL)
    {
        return -1;
    }

    return process->parentProcessIdentifier;
}

SInt32 EFProcessGetUserIdentifier(EFProcessRef process)
{
    if(process == NULL)
    {
        return -1;
    }

    return process->userIdentifier;
}

SInt32 EFProcessGetGroupIdentifier(EFProcessRef process)
{
    if(process == NULL)
    {
        return -1;
    }

    return process->groupIdentifier;
}

SInt32 EFProcessGetProcessGroupIdentifier(EFProcessRef process)
{
    if(process == NULL)
    {
        return -1;
    }

    return process->processGroupIdentifier;
}

SInt32 EFProcessGetSessionIdentifier(EFProcessRef process)
{
    if(process == NULL)
    {
        return -1;
    }

    return process->sessionIdentifier;
}

EFStringRef EFProcessCopyUserName(EFAllocatorRef allocator,
                                  EFProcessRef process)
{
    if(process == NULL)
    {
        return NULL;
    }

    struct passwd *pw = getpwuid(process->userIdentifier);
    const char *username = (pw != NULL) ? pw->pw_name : NULL;
    return EFStringCreateWithCString(allocator, username, kEFStringEncodingUTF8);
}

EFStringRef EFProcessCopyGroupName(EFAllocatorRef allocator,
                                   EFProcessRef process)
{
    if(process == NULL)
    {
        return NULL;
    }

    struct group *gr = getgrgid(process->groupIdentifier);
    const char *groupname = (gr != NULL) ? gr->gr_name : NULL;
    return EFStringCreateWithCString(allocator, groupname, kEFStringEncodingUTF8);
}

EFStringRef EFProcessGetCommand(EFProcessRef process)
{
    if(process == NULL)
    {
        return NULL;
    }

    return process->command;
}

EFStringRef EFProcessGetExecutablePath(EFProcessRef process)
{
    if(process == NULL)
    {
        return NULL;
    }

    return process->executablePath;
}

EFArrayRef EFProcessGetArguments(EFProcessRef process)
{
    if(process == NULL)
    {
        return NULL;
    }

    return process->arguments;
}

Boolean EFProcessSendSignal(EFProcessRef process,
                            SInt32 signal)
{
    if(process == NULL)
    {
        return false;
    }

    return kill(process->processIdentifier, signal) == 0;
}

Boolean EFProcessSuspend(EFProcessRef process)
{
    return EFProcessSendSignal(process, SIGSTOP);
}

Boolean EFProcessResume(EFProcessRef process)
{
    return EFProcessSendSignal(process, SIGCONT);
}

Boolean EFProcessTerminate(EFProcessRef process)
{
    return EFProcessSendSignal(process, SIGTERM);
}

Boolean EFProcessForceKill(EFProcessRef process)
{
    return EFProcessSendSignal(process, SIGKILL);
}

Boolean EFProcessIsAlive(EFProcessRef process)
{
    if(process == NULL)
    {
        return false;
    }

    return getpgid(process->processIdentifier) != -1;
}

extern EFProcessRef EFProcessCurrent;

extern EFProcessRef EFProcessGetCurrentProcess(void)
{
    return EFProcessCurrent;
}

SInt32 EFProcessWaitPID(EFProcessRef process,
                        SInt32 *status,
                        SInt32 options)
{
    if(process == NULL)
    {
        return -1;
    }

    return waitpid(process->processIdentifier, status, options);
}
