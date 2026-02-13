#ifndef PTPMANAGER_H
#define PTPMANAGER_H

#include <ProgramComponent.h>
#include <NetworkProcessor.h>
#include <t41-ptp.h>
#include <registers/SwMuxControlRegister.h>

class PTPManager final : public ProgramComponent,
                         public NetworkProcessor
{
public:
    explicit PTPManager(ClockRole role);

protected:
    void beginImpl() override;

public:
    void run() override;

    void connect() override;

    void resetPTP();

    void onPTPControllerUpdated(std::function<void(double adjust)> callback);

    void onPTPLock(std::function<void(bool isLocked, NanoTime compare, NanoTime now)> callback);

    size_t printTo(Print &p) const override;

private:
    static void interrupt1588Timer();

    static void ptpSyncInterrupt();

    static void ptpAnnounceInterrupt();

    void handle1588Interrupt();

    void handleSyncInterrupt();

    void handleAnnounceInterrupt();

    static inline PTPManager *sInstance{nullptr};

    l3PTP ptp;
    NanoTime interruptS{0};
    NanoTime interruptNS{0};
    NanoTime ppsS{0};
    NanoTime ppsNS{0};
    IntervalTimer ptpSyncTimer;
    IntervalTimer ptpAnnounceTimer;
    int noPPSCount{0};
    std::function<void(double)> ptpControllerUpdatedCallback{nullptr};
    std::function<void(bool, NanoTime, NanoTime)> ptpLockCallback{nullptr};
    Pin24SwMuxControlRegister pin24SwMuxControlRegister;
    bool shouldSendAnnouncePacket{false};
    bool shouldSendSyncPacket{false};
};


#endif //PTPMANAGER_H
