#ifndef PTPMANAGER_H
#define PTPMANAGER_H

#include <ProgramComponent.h>
#include <AudioSystemConfig.h>
#include <NetworkProcessor.h>
#include <t41-ptp.h>


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

    size_t printTo(Print &print) const override;

private:
    void handleInterrupt();

    static void interrupt1588Timer();

    static inline PTPManager *sInstance{nullptr};

    time_t interruptS;
    uint32_t interruptNS;
    l3PTP ptp;
    std::function<void(double)> ptpControllerUpdatedCallback{nullptr};
    std::function<void(bool, NanoTime, NanoTime)> ptpLockCallback{nullptr};
};


#endif //PTPMANAGER_H
