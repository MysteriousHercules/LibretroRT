﻿using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace RetriX.Shared.Services
{
    public enum FullScreenChangeType { Enter, Exit, Toggle };

    public delegate void FullScreenChangeRequestedDelegate(IPlatformService sender, FullScreenChangeEventArgs args);

    public delegate void GameStateOperationRequestedDelegate(IPlatformService sender, GameStateOperationEventArgs args);

    public interface IPlatformService
    {
        bool IsFullScreenMode { get; }
        bool HandleGameplayKeyShortcuts { get; set; }

        bool ChangeFullScreenState(FullScreenChangeType changeType);

        Task<IPlatformFileWrapper> SelectFileAsync(IEnumerable<string> extensionsFilter);

        Task RunOnUIThreadAsync(Action action);

        event FullScreenChangeRequestedDelegate FullScreenChangeRequested;

        event GameStateOperationRequestedDelegate GameStateOperationRequested;
    }
}