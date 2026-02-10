#include "PTPManager.h"
#include <arm_math.h>
#include <QNEthernet.h>
#include <SystemUtils.h>
#include <registers/SwMuxControlRegister.h>

PTPManager::PTPManager(const ClockRole role) : ptp{role, DelayMode::E2E}
{
    sInstance = this;
}

void PTPManager::beginImpl()
{
    if (ptp.getClockRole() == ClockRole::Subscriber) {
        pinMode(13, OUTPUT);
    }

    pin24SwMuxControlRegister.begin();
    // Set up ENET PPS out on pin 24
    if (!pin24SwMuxControlRegister.setMuxMode(Pin24SwMuxControlRegister::MuxMode::ENET_1588_EVENT1_OUT) ||
        !qindesign::network::EthernetIEEE1588.setChannelCompareValue(1, NS_PER_S) ||
        !qindesign::network::EthernetIEEE1588.setChannelMode(1, qindesign::network::EthernetIEEE1588Class::TimerChannelModes::kPulseHighOnCompare) ||
        !qindesign::network::EthernetIEEE1588.setChannelOutputPulseWidth(1, 25) ||
        !qindesign::network::EthernetIEEE1588.setChannelInterruptEnable(1, true)) {
        Serial.printf("Failed to set up Ethernet IEEE 1588 timer. Restarting.");
        SystemUtils::reboot();
    }

    attachInterruptVector(IRQ_ENET_TIMER, interrupt1588Timer); //Configure Interrupt Handler
    NVIC_ENABLE_IRQ(IRQ_ENET_TIMER); //Enable Interrupt Handling

    switch (ptp.getClockRole()) {
        case ClockRole::Authority:
            Serial.println("Clock Authority");
            break;
        case ClockRole::Subscriber:
            Serial.println("Clock Subscriber");
            break;
        default:
            Serial.println("Unknown clock role. Abandoning.");
            while (true) {
            }
    }
}

void PTPManager::run()
{
    if (ptp.getClockRole() == ClockRole::Authority) {
        if (shouldSendAnnouncePacket) {
            shouldSendAnnouncePacket = false;
            ptp.announceMessage();
        }
        if (shouldSendSyncPacket) {
            shouldSendSyncPacket = false;
            ptp.syncMessage();
        }
    }

    ptp.update();

    if (ptp.getClockRole() == ClockRole::Subscriber) {
        // Six consecutive offsets below 100 ns sets pin 13 high to switch on the LED
        digitalWrite(13, ptp.getLockCount() > 5 ? HIGH : LOW);
    }
}

void PTPManager::connect()
{
    ptp.begin();

    if (ptp.getClockRole() == ClockRole::Authority) {
        ptpSyncTimer.begin(ptpSyncInterrupt, 1000000);
        ptpAnnounceTimer.begin(ptpAnnounceInterrupt, 1000000);
    }

    ptp.onControllerUpdated([this](const double adjust)
    {
        if (ptpControllerUpdatedCallback != nullptr)
            ptpControllerUpdatedCallback(adjust);
    });
}

void PTPManager::resetPTP()
{
    ptp.reset();
}

void PTPManager::onPTPControllerUpdated(std::function<void(double adjust)> callback)
{
    ptpControllerUpdatedCallback = std::move(callback);
}

void PTPManager::onPTPLock(std::function<void(bool isLocked, NanoTime compare, NanoTime now)> callback)
{
    ptpLockCallback = std::move(callback);
}

size_t PTPManager::printTo(Print &print) const
{
    // TODO: print something
    return 0;
}

void PTPManager::interrupt1588Timer()
{
    if (sInstance) {
        sInstance->handle1588Interrupt();
    }
}

void PTPManager::ptpSyncInterrupt()
{
    if (sInstance) {
        sInstance->handleSyncInterrupt();
    }
}

void PTPManager::ptpAnnounceInterrupt()
{
    if (sInstance) {
        sInstance->handleAnnounceInterrupt();
    }
}

void PTPManager::handle1588Interrupt()
{
    uint32_t t;
    if (!qindesign::network::EthernetIEEE1588.getAndClearChannelStatus(1)) {
        // Allow write to complete so the interrupt doesn't fire twice
        __DSB();
        return;
    }
    qindesign::network::EthernetIEEE1588.getChannelCompareValue(1, t);

    t %= NS_PER_S;

    timespec ts{};
    qindesign::network::EthernetIEEE1588.readTimer(ts);

    // The channel compare value may be close to, but not exactly, 1e9.
    // If it's just less than 1e9, the tv_sec part of the timespec read from the
    // timer will be 1 second too great.
    // What circumstances exist where t < 9e8 and tv_nsec > 1e8 I don't know;
    // perhaps when using an external PPS source.
    // The subscriber probably doesn't need to perform this check.
    if (ts.tv_nsec < 100 * 1000 * 1000 && t > 900 * 1000 * 1000) {
        interruptS = ts.tv_sec - 1;
        // Serial.printf("t (channel compare): %" PRIu32 ", ts.tv_nsec: %" PRId32 "\n", t, ts.tv_nsec);
        // Serial.printf("So interrupt_s = %" PRId64 " = %" PRId32 " - 1\n", interrupt_s, ts.tv_sec);
    } else {
        interruptS = ts.tv_sec;
        // Serial.printf("t (channel compare): %" PRIu32 ", ts.tv_nsec: %" PRId32 "\n", t, ts.tv_nsec);
    }

    interruptNS = t;

    // Start audio the first a PTP lock (offset < 100 ns) is reported.
    if (ptp.getClockRole() == ClockRole::Subscriber && ptpLockCallback != nullptr) {
        const NanoTime enetCompareTime{interruptS * NS_PER_S + interruptNS},
                now{ts.tv_sec * NS_PER_S + ts.tv_nsec};
        ptpLockCallback(ptp.getLockCount() > 0, enetCompareTime, now);
    }

    // Allow write to complete so the interrupt doesn't fire twice
    __DSB();
}

void PTPManager::handleSyncInterrupt()
{
    if (noPPSCount > 5) {
        shouldSendSyncPacket = true;
    } else {
        noPPSCount++;
    }
}

void PTPManager::handleAnnounceInterrupt()
{
    shouldSendAnnouncePacket = true;
}
