﻿using RetriX.Shared.Services;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.Storage.Pickers;
using Windows.System;
using Windows.UI.Core;
using Windows.UI.ViewManagement;
using System;
using Windows.ApplicationModel.Core;
using GalaSoft.MvvmLight.Messaging;
using RetriX.Shared.Messages;

namespace RetriX.UWP.Services
{
    public class PlatformService : IPlatformService
    {
        private ApplicationView AppView => ApplicationView.GetForCurrentView();
        private CoreWindow CoreWindow => CoreWindow.GetForCurrentThread();

        public bool IsFullScreenMode => AppView.IsFullScreenMode;

        private bool handleGameplayKeyShortcuts = false;
        public bool HandleGameplayKeyShortcuts
        {
            get { return handleGameplayKeyShortcuts; }
            set
            {
                handleGameplayKeyShortcuts = value;
                var window = CoreWindow.GetForCurrentThread();
                window.KeyDown -= OnKeyDown;
                if (handleGameplayKeyShortcuts)
                {               
                    window.KeyDown += OnKeyDown;
                }
            }
        }

        public event FullScreenChangeRequestedDelegate FullScreenChangeRequested;

        public event GameStateOperationRequestedDelegate GameStateOperationRequested;

        public bool ChangeFullScreenState(FullScreenChangeType changeType)
        {
            if ((changeType == FullScreenChangeType.Enter && IsFullScreenMode) || (changeType == FullScreenChangeType.Exit && !IsFullScreenMode))
            {
                return true;
            }

            if (changeType == FullScreenChangeType.Toggle)
            {
                changeType = IsFullScreenMode ? FullScreenChangeType.Exit : FullScreenChangeType.Enter;
            }

            switch (changeType)
            {
                case FullScreenChangeType.Enter:
                    return AppView.TryEnterFullScreenMode();
                case FullScreenChangeType.Exit:
                    AppView.ExitFullScreenMode();
                    return true;
            }

            throw new Exception("this should never happen");
        }

        public async Task<IPlatformFileWrapper> SelectFileAsync(IEnumerable<string> extensionsFilter)
        {
            var picker = new FileOpenPicker();
            picker.SuggestedStartLocation = PickerLocationId.ComputerFolder;
            foreach (var i in extensionsFilter)
            {
                picker.FileTypeFilter.Add(i);
            }

            var file = await picker.PickSingleFileAsync();
            return file == null ? null : new PlatformFileWrapper(file);
        }

        private void OnKeyDown(CoreWindow sender, KeyEventArgs args)
        {
            var shiftState = sender.GetKeyState(VirtualKey.Shift);
            var shiftIsDown = (shiftState & CoreVirtualKeyStates.Down) == CoreVirtualKeyStates.Down;

            var altState = sender.GetKeyState(VirtualKey.LeftMenu);
            var altIsDown = (altState & CoreVirtualKeyStates.Down) == CoreVirtualKeyStates.Down;

            switch (args.VirtualKey)
            {
                //By default the gamepad's B button is treated as a hardware back button.
                //Handling the KeyDown event disables this.
                //We want this to happen only in the game page and not in the rest of the UI
                case VirtualKey.GamepadB:
                    args.Handled = true;
                    break;

                //Alt+Enter: enter fullscreen
                case VirtualKey.Enter:
                    if (shiftIsDown)
                    {
                        FullScreenChangeRequested(this, new FullScreenChangeEventArgs(FullScreenChangeType.Toggle));
                        args.Handled = true;
                    }
                    break;

                case VirtualKey.Escape:
                    FullScreenChangeRequested(this, new FullScreenChangeEventArgs(FullScreenChangeType.Exit));
                    args.Handled = true;
                    break;

                case VirtualKey.F1:
                    HandleFunctionKeyPress(shiftIsDown, 1, args);
                    break;

                case VirtualKey.F2:
                    HandleFunctionKeyPress(shiftIsDown, 2, args);
                    break;

                case VirtualKey.F3:
                    HandleFunctionKeyPress(shiftIsDown, 3, args);
                    break;

                case VirtualKey.F4:
                    HandleFunctionKeyPress(shiftIsDown, 4, args);
                    break;

                case VirtualKey.F5:
                    HandleFunctionKeyPress(shiftIsDown, 5, args);
                    break;

                case VirtualKey.F6:
                    HandleFunctionKeyPress(shiftIsDown, 6, args);
                    break;
            }
        }

        public Task RunOnUIThreadAsync(Action action)
        {
            return CoreApplication.MainView.CoreWindow.Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () => action()).AsTask();
        }

        private void HandleFunctionKeyPress(bool shiftIsDown, uint slotID, KeyEventArgs args)
        {
            var eventArgs = new GameStateOperationEventArgs(shiftIsDown ? GameStateOperationEventArgs.GameStateOperationType.Save : GameStateOperationEventArgs.GameStateOperationType.Load, slotID);
            GameStateOperationRequested(this, eventArgs);

            args.Handled = true;
        }
    }
}
