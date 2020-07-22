/* Copyright (c) 2011-2014, 2016-2020 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef LOC_API_BASE_H
#define LOC_API_BASE_H

#include <stddef.h>
#include <ctype.h>
#include <gps_extended.h>
#include <LocationAPI.h>
#include <MsgTask.h>
#include <log_util.h>

namespace loc_core {

class ContextBase;
struct LocApiResponse;

int hexcode(char *hexstring, int string_size,
            const char *data, int data_size);
int decodeAddress(char *addr_string, int string_size,
                  const char *data, int data_size);

#define MAX_ADAPTERS          10
#define MAX_FEATURE_LENGTH    100

#define TO_ALL_ADAPTERS(adapters, call)                                \
    for (int i = 0; i < MAX_ADAPTERS && NULL != (adapters)[i]; i++) {  \
        call;                                                          \
    }

#define TO_1ST_HANDLING_ADAPTER(adapters, call)                              \
    for (int i = 0; i <MAX_ADAPTERS && NULL != (adapters)[i] && !(call); i++);

enum xtra_version_check {
    DISABLED,
    AUTO,
    XTRA2,
    XTRA3
};

class LocAdapterBase;
struct LocSsrMsg;
struct LocOpenMsg;

typedef struct
{
    uint32_t accumulatedDistance;
    uint32_t numOfBatchedPositions;
} LocApiBatchData;

typedef struct
{
    uint32_t hwId;
} LocApiGeofenceData;

struct LocApiMsg: LocMsg {
    private:
        std::function<void ()> mProcImpl;
        inline virtual void proc() const {
            mProcImpl();
        }
    public:
        inline LocApiMsg(std::function<void ()> procImpl ) :
                         mProcImpl(procImpl) {}
};

class LocApiProxyBase {
public:
    inline LocApiProxyBase() {}
    inline virtual ~LocApiProxyBase() {}
    inline virtual void* getSibling2() { return NULL; }
};

class LocApiBase {
    friend struct LocSsrMsg;
    //LocOpenMsg calls open() which makes it necessary to declare
    //it as a friend
    friend struct LocOpenMsg;
    friend struct LocCloseMsg;
    friend struct LocKillMsg;
    friend class ContextBase;
    static MsgTask* mMsgTask;
    LocAdapterBase* mLocAdapters[MAX_ADAPTERS];

protected:
    ContextBase *mContext;
    virtual enum loc_api_adapter_err
        open(LOC_API_ADAPTER_EVENT_MASK_T mask);
    virtual enum loc_api_adapter_err
        close();
    LOC_API_ADAPTER_EVENT_MASK_T getEvtMask();
    LOC_API_ADAPTER_EVENT_MASK_T mMask;
    uint32_t mNmeaMask;
    LocApiBase(LOC_API_ADAPTER_EVENT_MASK_T excludedMask,
               ContextBase* context = NULL);
    inline virtual ~LocApiBase() {
        if (nullptr != mMsgTask) {
            mMsgTask->destroy();
            mMsgTask = nullptr;
        }
    }
    bool isInSession();
    const LOC_API_ADAPTER_EVENT_MASK_T mExcludedMask;
    bool isMaster();

public:
    inline void sendMsg(const LocMsg* msg) const {
        mMsgTask->sendMsg(msg);
    }
    inline void destroy() {
        close();
        struct LocKillMsg : public LocMsg {
            LocApiBase* mLocApi;
            inline LocKillMsg(LocApiBase* locApi) : LocMsg(), mLocApi(locApi) {}
            inline virtual void proc() const {
                delete mLocApi;
            }
        };
        sendMsg(new LocKillMsg(this));
    }

    static bool needReport(const UlpLocation& ulpLocation,
                           enum loc_sess_status status,
                           LocPosTechMask techMask);

    void addAdapter(LocAdapterBase* adapter);
    void removeAdapter(LocAdapterBase* adapter);

    // upward calls
    void handleEngineUpEvent();
    void handleEngineDownEvent();
    void reportPosition(UlpLocation& location,
                        GpsLocationExtended& locationExtended,
                        enum loc_sess_status status,
                        LocPosTechMask loc_technology_mask =
                                  LOC_POS_TECH_MASK_DEFAULT,
                        GnssDataNotification* pDataNotify = nullptr,
                        int msInWeek = -1);
    void reportSv(GnssSvNotification& svNotify);
    void reportSvMeasurement(GnssSvMeasurementSet &svMeasurementSet);
    void reportSapInsParams(GnssSapInsParams &sapInsParams);
    void reportSvPolynomial(GnssSvPolynomial &svPolynomial);
    void reportSvEphemeris(GnssSvEphemerisReport &svEphemeris);
    void reportStatus(LocGpsStatusValue status);
    void reportNmea(const char* nmea, int length);
    void reportData(GnssDataNotification& dataNotify, int msInWeek);
    void reportXtraServer(const char* url1, const char* url2,
                          const char* url3, const int maxlength);
    void reportLocationSystemInfo(const LocationSystemInfo& locationSystemInfo);
    void requestXtraData();
    void requestTime();
    void requestLocation();
    void requestATL(int connHandle, LocAGpsType agps_type, LocApnTypeMask apn_type_mask);
    void releaseATL(int connHandle);
    void requestNiNotify(GnssNiNotification &notify, const void* data);
    void reportGnssMeasurementData(GnssMeasurementsNotification& measurements, int msInWeek);
    void reportWwanZppFix(LocGpsLocation &zppLoc);
    void reportZppBestAvailableFix(LocGpsLocation &zppLoc, GpsLocationExtended &location_extended,
            LocPosTechMask tech_mask);
    void reportGnssSvIdConfig(const GnssSvIdConfig& config);
    void reportGnssSvTypeConfig(const GnssSvTypeConfig& config);
    void requestOdcpi(OdcpiRequestInfo& request);
    void reportGnssEngEnergyConsumedEvent(uint64_t energyConsumedSinceFirstBoot);
    void reportDeleteAidingDataEvent(GnssAidingData& aidingData);
    void reportKlobucharIonoModel(GnssKlobucharIonoModel& ionoModel);
    void reportGnssAdditionalSystemInfo(GnssAdditionalSystemInfo& additionalSystemInfo);
    void reportGnssConfig(uint32_t sessionId, const GnssConfig& gnssConfig);

    // downward calls
    // All below functions are to be defined by adapter specific modules:
    // RPC, QMI, etc.  The default implementation is empty.

    virtual void* getSibling();
    virtual LocApiProxyBase* getLocApiProxy();
    virtual void startFix(const LocPosMode& fixCriteria, LocApiResponse* adapterResponse);
    virtual void
        stopFix(LocApiResponse* adapterResponse);
    virtual void
        deleteAidingData(const GnssAidingData& data, LocApiResponse* adapterResponse);

    virtual void
        injectPosition(double latitude, double longitude, float accuracy);

    virtual void
        injectPosition(const GnssLocationInfoNotification &locationInfo, bool onDemandCpi=false);

    virtual void
        injectPosition(const Location& location, bool onDemandCpi);
    virtual void
        setTime(LocGpsUtcTime time, int64_t timeReference, int uncertainty);

 //   // TODO:: called from izatapipds
    virtual enum loc_api_adapter_err
        setXtraData(char* data, int length);

    virtual void
        atlOpenStatus(int handle, int is_succ, char* apn, uint32_t apnLen,
                      AGpsBearerType bear, LocAGpsType agpsType,
                      LocApnTypeMask mask);
    virtual void
        atlCloseStatus(int handle, int is_succ);
    virtual void
        setPositionMode(const LocPosMode& posMode);
    virtual LocationError
        setServerSync(const char* url, int len, LocServerType type);
    virtual LocationError
        setServerSync(unsigned int ip, int port, LocServerType type);
    virtual void
        informNiResponse(GnssNiResponse userResponse, const void* passThroughData);
    virtual LocationError setSUPLVersionSync(GnssConfigSuplVersion version);
    virtual enum loc_api_adapter_err
        setNMEATypesSync(uint32_t typesMask);
    virtual LocationError setLPPConfigSync(GnssConfigLppProfile profile);
    virtual enum loc_api_adapter_err
        setSensorPropertiesSync(bool gyroBiasVarianceRandomWalk_valid,
                            float gyroBiasVarianceRandomWalk,
                            bool accelBiasVarianceRandomWalk_valid,
                            float accelBiasVarianceRandomWalk,
                            bool angleBiasVarianceRandomWalk_valid,
                            float angleBiasVarianceRandomWalk,
                            bool rateBiasVarianceRandomWalk_valid,
                            float rateBiasVarianceRandomWalk,
                            bool velocityBiasVarianceRandomWalk_valid,
                            float velocityBiasVarianceRandomWalk);
    virtual enum loc_api_adapter_err
        setSensorPerfControlConfigSync(int controlMode,
                               int accelSamplesPerBatch,
                               int accelBatchesPerSec,
                               int gyroSamplesPerBatch,
                               int gyroBatchesPerSec,
                               int accelSamplesPerBatchHigh,
                               int accelBatchesPerSecHigh,
                               int gyroSamplesPerBatchHigh,
                               int gyroBatchesPerSecHigh,
                               int algorithmConfig);
    virtual LocationError
        setAGLONASSProtocolSync(GnssConfigAGlonassPositionProtocolMask aGlonassProtocol);
    virtual LocationError setLPPeProtocolCpSync(GnssConfigLppeControlPlaneMask lppeCP);
    virtual LocationError setLPPeProtocolUpSync(GnssConfigLppeUserPlaneMask lppeUP);
    virtual GnssConfigSuplVersion convertSuplVersion(const uint32_t suplVersion);
    virtual GnssConfigLppProfile convertLppProfile(const uint32_t lppProfile);
    virtual GnssConfigLppeControlPlaneMask convertLppeCp(const uint32_t lppeControlPlaneMask);
    virtual GnssConfigLppeUserPlaneMask convertLppeUp(const uint32_t lppeUserPlaneMask);

    virtual void getWwanZppFix();
    virtual void getBestAvailableZppFix();
    virtual void installAGpsCert(const LocDerEncodedCertificate* pData,
                                 size_t length,
                                 uint32_t slotBitMask);
    inline virtual void setInSession(bool inSession) {

        (void)inSession;
    }


    void updateEvtMask();
    void updateNmeaMask(uint32_t mask);

    virtual LocationError setGpsLockSync(GnssConfigGpsLock lock);
    virtual void requestForAidingData(GnssAidingDataSvMask svDataMask);

    virtual LocationError setXtraVersionCheckSync(uint32_t check);

    /* Requests for SV/Constellation Control */
    virtual LocationError setBlacklistSvSync(const GnssSvIdConfig& config);
    virtual void setBlacklistSv(const GnssSvIdConfig& config);
    virtual void getBlacklistSv();
    virtual void setConstellationControl(const GnssSvTypeConfig& config,
                                         LocApiResponse *adapterResponse=nullptr);
    virtual void getConstellationControl();
    virtual void resetConstellationControl(LocApiResponse *adapterResponse=nullptr);

    virtual void setConstrainedTuncMode(bool enabled,
                                        float tuncConstraint,
                                        uint32_t energyBudget,
                                        LocApiResponse* adapterResponse=nullptr);
    virtual void setPositionAssistedClockEstimatorMode(bool enabled,
                                                       LocApiResponse* adapterResponse=nullptr);
    virtual LocationError getGnssEnergyConsumed();
    virtual void updatePowerState(PowerStateType powerState);
    virtual void configRobustLocation(bool enable, bool enableForE911,
                                      LocApiResponse* adapterResponse=nullptr);

    virtual void getRobustLocationConfig(uint32_t sessionId, LocApiResponse* adapterResponse);
};

typedef LocApiBase* (getLocApi_t)(LOC_API_ADAPTER_EVENT_MASK_T exMask,
                                  ContextBase *context);

} // namespace loc_core

#endif //LOC_API_BASE_H
