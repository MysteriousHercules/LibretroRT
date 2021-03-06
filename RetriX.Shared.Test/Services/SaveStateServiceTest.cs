﻿using Moq;
using RetriX.Shared.Services;
using System.Linq;
using System.Threading.Tasks;
using Xunit;

namespace RetriX.Shared.Test.Services
{
    public class SaveStateServiceTest : TestBase<SaveStateService>
    {
        private const int InitializationDelayMs = 50;

        private const string StateSavedToSlotMessageTitle = "Title";
        private const string StateSavedToSlotMessageBody = "Body {0}";

        private const string GameId = nameof(GameId);
        private const uint SlotID = 4;

        static readonly byte[] TestSavePayload = Enumerable.Range(0, byte.MaxValue).Select(d => (byte)d).ToArray();

        protected override SaveStateService InstanceTarget()
        {
            return new SaveStateService(NotificationServiceMock.Object, LocalizationServiceMock.Object) { GameId = GameId };
        }

        public SaveStateServiceTest()
        {
            LocalizationServiceMock.Setup(d => d.GetLocalizedString(SaveStateService.StateSavedToSlotMessageTitleKey)).Returns(StateSavedToSlotMessageTitle);
            LocalizationServiceMock.Setup(d => d.GetLocalizedString(SaveStateService.StateSavedToSlotMessageBodyKey)).Returns(StateSavedToSlotMessageBody);
        }

        [Theory]
        [InlineData(null)]
        [InlineData("")]
        [InlineData(" ")]
        public async Task InvalidIdIsHandledCorrectly(string gameId)
        {
            await Task.Delay(InitializationDelayMs);

            Target.GameId = gameId;
            var result = await Target.SlotHasDataAsync(SlotID);
            Assert.False(result);

            result = await Target.SaveStateAsync(SlotID, TestSavePayload);
            Assert.False(result);

            result = await Target.SlotHasDataAsync(SlotID);
            Assert.False(result);

            var loaded = await Target.LoadStateAsync(SlotID);
            Assert.Null(loaded);
        }

        [Fact]
        public async Task SaveLoadDeleteWorks()
        {
            await Task.Delay(InitializationDelayMs);

            var result = await Target.SlotHasDataAsync(SlotID);
            Assert.False(result);

            var loaded = await Target.LoadStateAsync(SlotID);
            Assert.Null(loaded);

            result = await Target.SaveStateAsync(SlotID, TestSavePayload);
            Assert.True(result);

            loaded = await Target.LoadStateAsync(SlotID);
            Assert.Equal(loaded, TestSavePayload);

            await Target.ClearSavesAsync();

            result = await Target.SlotHasDataAsync(SlotID);
            Assert.False(result);

            loaded = await Target.LoadStateAsync(SlotID);
            Assert.Null(loaded);
        }

        [Fact]
        public async Task DifferentSlotsAreIndependent()
        {
            await Task.Delay(InitializationDelayMs);

            var result = await Target.SaveStateAsync(SlotID, TestSavePayload);
            Assert.True(result);

            var otherSlotID = SlotID + 1;

            result = await Target.SlotHasDataAsync(otherSlotID);
            Assert.False(result);

            var loaded = await Target.LoadStateAsync(otherSlotID);
            Assert.Null(loaded);

            await Target.ClearSavesAsync();
        }

        [Fact]
        public async Task ConcurrentOperationsAreBlocked()
        {
            await Task.Delay(InitializationDelayMs);

            var otherSlotID = SlotID + 1;

            var result = await Target.SaveStateAsync(SlotID, TestSavePayload);
            Assert.True(result);
            result = await Target.SaveStateAsync(otherSlotID, TestSavePayload);
            Assert.True(result);

            var loadTasks = new Task<byte[]>[]
            {
                Target.LoadStateAsync(SlotID),
                Target.LoadStateAsync(otherSlotID)
            };

            await Task.WhenAll(loadTasks);
            Assert.NotNull(loadTasks[0].Result);
            Assert.Null(loadTasks[1].Result);

            await Target.ClearSavesAsync();
        }

        [Theory]
        [InlineData(true)]
        [InlineData(false)]
        public async Task NotificationsAreSentWhenAppropriate(bool saveSuccessful)
        {
            await Task.Delay(InitializationDelayMs);

            if(!saveSuccessful)
            {
                Target.GameId = null;
            }

            var result = await Target.SaveStateAsync(SlotID, TestSavePayload);
            Assert.Equal(saveSuccessful, result);

            var expectedBody = string.Format(StateSavedToSlotMessageBody, SlotID);
            var expectedTimes = saveSuccessful ? Times.Once() : Times.Never();
            NotificationServiceMock.Verify(d => d.Show(StateSavedToSlotMessageTitle, expectedBody, 0), expectedTimes);

            await Target.ClearSavesAsync();
        }
    }
}
